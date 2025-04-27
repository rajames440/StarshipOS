-- vi:ft=lua

package.path = "rom/?.lua"

local L4  = require "L4"
local vmm = require "vmm"
local ldr = L4.default_loader
local ft  = L4.Env.user_factory

vmm.loader.log_fab = ldr:new_channel()
ldr.log_fab = vmm.loader.log_fab

local io_bus = ldr:new_channel()
local platform_ctl = ldr:new_channel()
local emmc_bus = ldr:new_channel()
local emmc_drv = ldr:new_channel()

local mem_flags = L4.Mem_alloc_flags.Continuous
                | L4.Mem_alloc_flags.Pinned
                | L4.Mem_alloc_flags.Super_pages

-- bounce buffer
-- WARNING: There will be weird -5 errors if the virtio client uses a request
--          with a buffer size bigger than the bounce buffer. The driver
--          announces max_size() = sizeof("bbds" dataspace) so the client
--          has to follow the virtio-constraints.
local bbds = ft:create(L4.Proto.Dataspace, 128 << 10, mem_flags, 21):m("rws")

ldr:start(
{
  log  = L4.Env.log,
  caps =
  {
    jdb  = L4.Env.jdb,
    cons = vmm.loader.log_fab:svr()
  },
}, "rom/cons -a")

ldr:start(
{
  caps =
  {
    jdb    = L4.Env.jdb,
    emmc   = io_bus:svr(),
    icu    = L4.Env.icu,
    iommu  = L4.Env.iommu,
    sigma0 = L4.Proto.Sigma0,
    platform_ctl = platform_ctl:svr()
  },
  log = { "io", "red" },
}, "rom/io -v rom/io.cfg")

ldr:start(
{
  caps =
  {
    jdb  = L4.Env.jdb,
    vbus = io_bus,
    svr  = emmc_bus:svr(),
    cl1  = emmc_drv:svr(),
    bbds = bbds,
  },
  log = { "emmc", "cyan" },
}, "rom/emmc-drv -v --client cl1"..
   " --device AFFA05B0-9379-480E-B9C6-5FF57FB1D194 --ds-max 5")
-- the UIID depends on the actual GPT partition table

ldr:start(
{
  caps =
  {
    jdb = L4.Env.jdb,
    dsk = emmc_drv
  },
  log = { "client", "green" },
}, "rom/virtio-block-chksum-sync -m -v")
