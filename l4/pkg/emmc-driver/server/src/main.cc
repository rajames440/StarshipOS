/*
 * Copyright (C) 2020-2021, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *            Sarah Hoffmann <sarah.hoffmann@kernkonzept.com>
 *            Jakub Jermar <jakub.jermar@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <getopt.h>

#include <l4/sys/factory>
#include <l4/vbus/vbus>
#include <l4/vbus/vbus_pci>
#include <l4/libblock-device/block_device_mgr.h>
#include <terminate_handler-l4>

#include "device.h"
#include "debug.h"
#include "factory.h"
#include "mmc.h"
#include "util.h"

namespace {

static Dbg warn(Dbg::Warn, "main");
static Dbg info(Dbg::Info, "main");
static Dbg trace(Dbg::Trace, "main");

static Emmc::Device_type_disable device_type_disable;
static int max_seg = 64;

// Don't specify the partition number when creating a client. The partition is
// already specified by setting `device` to the GUID of the corresponding GPT
// partition. To access the entire device, use the PSN (product serial number)
// of the device.
//
// See Device::match_hid() for matching the whole device. This function is
// invoked if libblock-device couldn't match the device name to any GUID.
//
// Specifying PSN:partition would work as well.
enum { No_partno = -1 };

static char const *usage_str =
"Usage: %s [-vq] --client CAP <client parameters>\n"
"\n"
"Options:\n"
" -v                   Verbose mode\n"
" -q                   Be quiet\n"
" --disable-mode MODE  Disable a certain eMMC mode (can be used more than once)\n"
"                      (MODE is hs26|hs52|hs200|hs400)\n"
" --client CAP         Add a static client via the CAP capability\n"
" --ds-max NUM         Specify maximum number of dataspaces the client can register\n"
" --max-seg NUM        Specify maximum number of segments one vio request can have\n"
" --readonly           Only allow read-only access to the device\n"
" --dma-map-all        Map the entire client dataspace permanently\n";

class Blk_mgr
: public Emmc::Base_device_mgr,
  public L4::Epiface_t<Blk_mgr, L4::Factory>
{
  class Deletion_irq : public L4::Irqep_t<Deletion_irq>
  {
  public:
    void handle_irq()
    { _parent->check_clients(); }
    Deletion_irq(Blk_mgr *parent) : _parent{parent} {}
  private:
    Blk_mgr *_parent;
  };

public:
  Blk_mgr(L4Re::Util::Object_registry *registry)
  : Emmc::Base_device_mgr(registry),
    _del_irq(this)
  {
    auto c = L4Re::chkcap(registry->register_irq_obj(&_del_irq),
                          "Creating IRQ for IPC gate deletion notifications.");
    L4Re::chksys(L4Re::Env::env()->main_thread()->register_del_irq(c),
                 "Registering deletion IRQ at the thread.");
  }

  long op_create(L4::Factory::Rights, L4::Ipc::Cap<void> &res, l4_umword_t,
                 L4::Ipc::Varg_list_ref valist)
  {
    trace.printf("Client requests connection.\n");

    // default values
    std::string device;
    int num_ds = 2;
    bool readonly = false;
    bool dma_map_all = false;

    for (L4::Ipc::Varg p: valist)
      {
        if (!p.is_of<char const *>())
          {
            warn.printf("String parameter expected.\n");
            return -L4_EINVAL;
          }

        std::string device_param;
        if (parse_string_param(p, "device=", &device_param))
          {
            long ret = parse_device_name(device_param, device);
            if (ret < 0)
              return ret;
          }
        else if (parse_int_param(p, "ds-max=", &num_ds))
          {
            if (num_ds <= 0 || num_ds > 256) // sanity check with arbitrary limit
              {
                warn.printf("Invalid range for parameter 'ds-max'. "
                            "Number must be between 1 and 256.\n");
                return -L4_EINVAL;
              }
          }
        else if (strncmp(p.value<char const *>(), "readonly", p.length()) == 0)
          readonly = true;
        else if (strncmp(p.value<char const *>(), "dma-map-all", p.length()) == 0)
          dma_map_all = true;
      }

    if (device.empty())
      {
        warn.printf("Parameter 'device=' not specified. Device label or UUID required.\n");
        return -L4_EINVAL;
      }

    L4::Cap<void> cap;
    int ret = create_dynamic_client(device, No_partno, num_ds, &cap, readonly,
                                    [dma_map_all, device](Emmc::Base_device *b)
      {
        Dbg(Dbg::Warn).printf("%s for device '%s'.\033[m\n",
                              dma_map_all ? "\033[31;1mDMA-map-all enabled"
                                          : "\033[32mDMA-map-all disabled",
                              device.c_str());
        if (auto *pd = dynamic_cast<Emmc::Part_device *>(b))
          pd->set_dma_map_all(dma_map_all);
        else
          b->set_dma_map_all(dma_map_all);
      });
    if (ret >= 0)
      {
        res = L4::Ipc::make_cap(cap, L4_CAP_FPAGE_RWSD);
        L4::cap_cast<L4::Kobject>(cap)->dec_refcnt(1);
      }

    return (ret == -L4_ENODEV && _scan_in_progress) ? -L4_EAGAIN : ret;
  }

  void scan_finished()
  { _scan_in_progress = false; }


private:
  static bool parse_string_param(L4::Ipc::Varg const &param, char const *prefix,
                                 std::string *out)
  {
    l4_size_t headlen = strlen(prefix);

    if (param.length() < headlen)
      return false;

    char const *pstr = param.value<char const *>();

    if (strncmp(pstr, prefix, headlen) != 0)
      return false;

    *out = std::string(pstr + headlen, strnlen(pstr, param.length()) - headlen);

    return true;
  }

  static bool parse_int_param(L4::Ipc::Varg const &param, char const *prefix,
                              int *out)
  {
    l4_size_t headlen = strlen(prefix);

    if (param.length() < headlen)
      return false;

    char const *pstr = param.value<char const *>();

    if (strncmp(pstr, prefix, headlen) != 0)
      return false;

    std::string tail(pstr + headlen, param.length() - headlen);

    char *endp;
    long num = strtol(tail.c_str(), &endp, 10);

    if (num < INT_MIN || num > INT_MAX || *endp != '\0')
      {
        warn.printf("Bad parameter '%s'. Number required.\n", prefix);
        L4Re::throw_error(-L4_EINVAL, "Parsing integer");
      }

    *out = num;

    return true;
  }

  Deletion_irq _del_irq;
  bool _scan_in_progress = true;
};

struct Client_opts
{
  bool add_client(Blk_mgr *blk_mgr)
  {
    if (capname)
      {
        if (device.empty())
          {
            Err().printf("No device for client '%s' given. "
                         "Please specify a device.\n", capname);
            return false;
          }

        auto cap = L4Re::Env::env()->get_cap<L4::Rcv_endpoint>(capname);
        if (!cap.is_valid())
          {
            Err().printf("Client capability '%s' no found.\n", capname);
            return false;
          }

        // Copy parameters for lambda capture. The object itself is ephemeral!
        std::string dev = device;
        bool map_all = dma_map_all;
        blk_mgr->add_static_client(cap, dev.c_str(), No_partno, ds_max, readonly,
                                   [dev, map_all](Emmc::Base_device *b)
         {
           Dbg(Dbg::Warn).printf("%s for device '%s'\033[m\n",
                                 map_all ? "\033[31;1mDMA-map-all enabled"
                                         : "\033[32mDMA-map-all disabled",
                                 dev.c_str());
           if (auto *pd = dynamic_cast<Emmc::Part_device *>(b))
             pd->set_dma_map_all(map_all);
           else
             b->set_dma_map_all(map_all);
         });
      }

    return true;
  }

  char const *capname = nullptr;
  std::string device;
  int ds_max = 2;
  bool readonly = false;
  bool dma_map_all = false;
};

static Block_device::Errand::Errand_server server;
static Blk_mgr drv(server.registry());
static unsigned devices_in_scan = 0;
static unsigned devices_found = 0;

static int
parse_args(int argc, char *const *argv)
{
  int debug_level = 1;

  enum
  {
    OPT_MAX_SEG,

    OPT_CLIENT,
    OPT_DEVICE,
    OPT_DS_MAX,
    OPT_READONLY,
    OPT_DMA_MAP_ALL,
    OPT_DISABLE_MODE,
  };

  static struct option const loptions[] =
  {
    // global options
    { "verbose",        no_argument,            NULL,   'v' },
    { "quiet",          no_argument,            NULL,   'q' },
    { "disable-mode",   required_argument,      NULL,   OPT_DISABLE_MODE },
    { "max-seg",        required_argument,      NULL,   OPT_MAX_SEG },

    // per-client options
    { "client",         required_argument,      NULL,   OPT_CLIENT },
    { "device",         required_argument,      NULL,   OPT_DEVICE },
    { "ds-max",         required_argument,      NULL,   OPT_DS_MAX },
    { "readonly",       no_argument,            NULL,   OPT_READONLY },
    { "dma-map-all",    no_argument,            NULL,   OPT_DMA_MAP_ALL },
    { 0,                0,                      NULL,   0, },
  };

  Client_opts opts;
  for (;;)
    {
      int opt = getopt_long(argc, argv, "vq", loptions, NULL);
      if (opt == -1)
        {
          if (optind < argc)
            {
              warn.printf("Unknown parameter '%s'\n", argv[optind]);
              warn.printf(usage_str, argv[0]);
              return -1;
            }
          break;
        }

      switch (opt)
        {
        case 'v':
          debug_level <<= 1;
          ++debug_level;
          break;
        case 'q':
          debug_level = 0;
          break;
        case OPT_DISABLE_MODE:
          // ==================
          // === eMMC modes ===
          // ==================
          if (!strcmp(optarg, "hs26"))
            device_type_disable.mmc.hs26() = 1;
          else if (!strcmp(optarg, "hs52"))
            device_type_disable.mmc.hs52() = 1;
          else if (!strcmp(optarg, "hs52_ddr"))
            {
              device_type_disable.mmc.hs52_ddr_18() = 1;
              device_type_disable.mmc.hs52_ddr_12() = 1;
            }
          else if (!strcmp(optarg, "hs200"))
            {
              device_type_disable.mmc.hs200_sdr_18() = 1;
              device_type_disable.mmc.hs200_sdr_12() = 1;
            }
          else if (!strcmp(optarg, "hs400"))
            {
              device_type_disable.mmc.hs400_ddr_18() = 1;
              device_type_disable.mmc.hs400_ddr_12() = 1;
            }
          // =====================
          // === SD card modes ===
          // =====================
          else if (!strcmp(optarg, "sdr12"))
            device_type_disable.sd |= Emmc::Mmc::Timing::Uhs_sdr12;
          else if (!strcmp(optarg, "sdr25"))
            device_type_disable.sd |= Emmc::Mmc::Timing::Uhs_sdr25;
          else if (!strcmp(optarg, "sdr50"))
            device_type_disable.sd |= Emmc::Mmc::Timing::Uhs_sdr50;
          else if (!strcmp(optarg, "sdr104"))
            device_type_disable.sd |= Emmc::Mmc::Timing::Uhs_sdr104;
          else if (!strcmp(optarg, "ddr50"))
            device_type_disable.sd |= Emmc::Mmc::Timing::Uhs_ddr50;
          else
            {
              warn.printf("Invalid parameter\n\n");
              warn.printf(usage_str, argv[0]);
            }
          break;
        case OPT_MAX_SEG:
          max_seg = atoi(optarg);
          break;

        case OPT_CLIENT:
          if (!opts.add_client(&drv))
            return 1;
          opts = Client_opts();
          opts.capname = optarg;
          break;
        case OPT_DEVICE:
          if (Blk_mgr::parse_device_name(optarg, opts.device) < 0)
            {
              warn.printf("Invalid device name parameter\n");
              return -1;
            }
          break;
        case OPT_DS_MAX:
          opts.ds_max = atoi(optarg);
          break;
        case OPT_READONLY:
          opts.readonly = true;
          break;
        case OPT_DMA_MAP_ALL:
          opts.dma_map_all = true;
          break;
        default:
          warn.printf(usage_str, argv[0]);
          return -1;
        }
    }

  if (!opts.add_client(&drv))
    return -1;

  Dbg::set_level(debug_level);
  return optind;
}

static void
device_scan_finished()
{
  if (--devices_in_scan > 0)
    return;

  drv.scan_finished();
  if (!server.registry()->register_obj(&drv, "svr").is_valid())
    warn.printf("Capability 'svr' not found. No dynamic clients accepted.\n");
  else
    trace.printf("Device now accepts new clients.\n");
}

static void
device_discovery(L4::Cap<L4vbus::Vbus> bus, L4::Cap<L4::Icu> icu)
{
  info.printf("Starting device discovery.\n");

  L4vbus::Pci_dev child;
  l4vbus_device_t di;
  auto root = bus->root();

  // make sure that we don't finish device scan before the loop is done
  ++devices_in_scan;

  while (root.next_device(&child, L4VBUS_MAX_DEPTH, &di) == L4_EOK)
    {
      trace.printf("Scanning child 0x%lx (%s).\n", child.dev_handle(), di.name);
      auto dev = Emmc::Factory::create_dev(child, di, bus, icu,
                                           server.registry(), max_seg,
                                           device_type_disable);
      if (dev)
        {
          ++devices_found;
          ++devices_in_scan;
          drv.add_disk(std::move(dev), device_scan_finished);
        }
    }

  // marks the end of the device detection loop
  device_scan_finished();

  if (devices_found)
    info.printf("All devices scanned. Found %u suitable.\n", devices_found);
  else
    info.printf("All devices scanned. No suitable found!\n");
}

static void
setup_hardware()
{
  auto vbus = L4Re::chkcap(L4Re::Env::env()->get_cap<L4vbus::Vbus>("vbus"),
                           "Get 'vbus' capability.", -L4_ENOENT);

  L4vbus::Icu icudev;
  L4Re::chksys(vbus->root().device_by_hid(&icudev, "L40009"),
               "Look for ICU device.");
  auto icu = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Icu>(),
                          "Allocate ICU capability.");
  L4Re::chksys(icudev.vicu(icu), "Request ICU capability.");

  device_discovery(vbus, icu);
}

} // namespace

int
main(int argc, char *const *argv)
{
  Dbg::set_level(3);

  if (int arg_idx = parse_args(argc, argv) < 0)
    return arg_idx;

  info.printf("Emmc driver says hello.\n");

  Util::tsc_init();

  if (Util::tsc_available())
    info.printf("TSC frequency of %s.\n",
                Util::readable_freq(Util::freq_tsc_hz()).c_str());
  else
    info.printf("Fine-grained clock not available!\n");

  Block_device::Errand::set_server_iface(&server);
  setup_hardware();

  trace.printf("Entering server loop.\n");
  server.loop();
}
