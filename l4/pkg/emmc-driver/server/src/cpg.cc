/*
 * Copyright (C) 2021, 2023-2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <l4/vbus/vbus_pci>
#include <l4/util/util.h>

#include "cpg.h"
#include "debug.h"
#include "mmio.h"

constexpr unsigned Rcar3_cpg::rmstpcr[Nr_modules];
constexpr unsigned Rcar3_cpg::smstpcr[Nr_modules];
constexpr unsigned Rcar3_cpg::mstpsr[Nr_modules];

static Dbg warn(Dbg::Warn, "cpg");

Rcar3_cpg::Rcar3_cpg()
{
  L4vbus::Pci_dev dev;
  l4vbus_device_t di;

  auto vbus = L4Re::chkcap(L4Re::Env::env()->get_cap<L4vbus::Vbus>("vbus"),
                           "Get 'vbus' capability.", -L4_ENOENT);
  auto root = vbus->root();

  while (root.next_device(&dev, L4VBUS_MAX_DEPTH, &di) == L4_EOK)
    {
      printf("CPG: scanning '%s'\n", di.name);
      if (   dev.is_compatible("renesas,r8a7795-cpg-mssr") == 1
          || dev.is_compatible("renesas,r8a7796-cpg-mssr") == 1)
        {
          for (unsigned i = 0; i < di.num_resources; ++i)
            {
              l4vbus_resource_t res;
              L4Re::chksys(dev.get_resource(i, &res), "Get device resource");
              if (res.type == L4VBUS_RESOURCE_MEM)
                {
                  l4_uint64_t addr = res.start;
                  l4_uint64_t size = res.end - res.start + 1;
                  if (dev.is_compatible("renesas,r8a7796-cpg-mssr") == 1)
                    {
                      L4::Cap<L4Re::Mmio_space> mmio_space(dev.bus_cap().cap());
                      _regs = new Hw::Mmio_space_register_block<32>(
                                mmio_space, addr, size);
                    }
                  else
                    {
                      _regs = new Hw::Mmio_map_register_block<32>(
                                dev.bus_cap(), addr, size);
                    }
                  return;
                }
            }
        }
    }

  L4Re::throw_error(-L4_EINVAL, "No CPG device found");
}

int
Rcar3_cpg::enable_clock(unsigned n, unsigned bit)
{
  if (n > Nr_modules - 1)
    {
      warn.printf("rcar3_cpg: invalid module %u.\n", n);
      return -L4_EINVAL;
    }
  if (bit > 31)
    {
      warn.printf("rcar3_cpg: invalid bit %u.\n", bit);
      return -L4_EINVAL;
    }

  unsigned mask = 1 << bit;

  // assume Cpgwpcr.wpe=1
  unsigned val = _regs[smstpcr[n]];
  val &= ~mask;
  _regs[Cpgwpr] = ~val;
  _regs[smstpcr[n]] = val;

  // The MSTPSRn register shows the status of the corresponding module which
  // was enabled using the respective SMSTPCRn register.
  for (unsigned i = 20; i > 0; --i)
    {
      if (!(_regs[mstpsr[n]] & mask))
        return L4_EOK;
      l4_ipc_sleep_ms(5);
    }

  // Device not there or doesn't work.
  return -L4_ENXIO;
}

void
Rcar3_cpg::enable_register(unsigned reg, unsigned value)
{
  if (reg >= 0x1000)
    L4Re::throw_error(-L4_EINVAL, "Wrong CPG index");

  _regs[Cpgwpr] = ~value;
  _regs[reg] = value;
}
