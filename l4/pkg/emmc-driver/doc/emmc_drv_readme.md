# eMMC driver {#l4re_servers_emmc_driver}

The eMMC driver is a driver for PCI Express eMMC controllers.


## Starting the service

The eMMC driver can be started with Lua like this:

    local emmc_bus = L4.default_loader:new_channel();
    L4.default_loader:start({
      caps = {
        vbus = vbus_emmc,
        svr = emmc_bus:svr(),
      },
    },
    "rom/emmc-drv");

First, an IPC gate (`emmc_bus`) is created which is used between the eMMC
driver and a client to request access to a particular disk or partition. The
server side is assigned to the mandatory `svr` capability of the eMMC driver.
See the sector below on how to configure access to a disk or partition.

The eMMC driver needs access to a virtual bus capability (`vbus`). On the
virtual bus the eMMC driver searches for eMMC compliant storage controllers.
Please see io's documentation about how to setup a virtual bus.


### Supported devices

The eMMC driver supports SDHCI and SDHI controllers, in particular
- SDHI interfaces found on RCar3 r8a7795 boards
- SDHCI interfaces found on RPI4
- uSDHCI interfaces found on i.MX8 boards
- uSDHCI interfaces found on the S32G SoC
- the QEMU SD card emulation (SDHCI, see `doc/pcie-ecam.io`),
- the QEMU eMMC emulation (provided by extending the QEMU SD card emulation by
  `doc/qemu-patch.diff`).


### Options

In the example above the eMMC driver is started in its default configuration.
To customize the configuration of the eMMC driver it accepts the following
command line options:

* `-v`

  Enable verbose mode. You can repeat this option up to three times to increase
  verbosity up to trace level.

* `-q`

  This option enables the quiet mode. All output is silenced.

* `--disable-mode <mode>`

  This option allows to disable certain eMMC/SD card modes from autodetection.
  The modes `hs26`, `hs52`, `hs52_ddr`, `hs200`, and `hs400` are determined for
  eMMC devices. The modes `sdr12`, `sdr25`, `sdr50`, `sdr104`, `ddr50` are
  determined for SD card devices.
  This option can be specified multiple times to disable multiple modes.

* `--max-seg <number>`

  Maximum number of segments per request. This number is announced to the virtio
  interface and is also relevant for the required bounce buffer size, see below.
  Default is 64.

* `--client <cap_name>`

  This option starts a new static client option context. The following
  `device`, `ds-max`, `readonly` and `dma-map-all` options belong to this
  context until a new client option context is created.

  The option parameter is the name of a local IPC gate capability with server
  rights.

* `--device <UUID>`

  This option denotes the partition UUID of the partition to be exported for
  the client specified in the preceding `client` option.

* `--ds-max <max>`

  This option sets the upper limit of the number of dataspaces the client is
  able to register with the eMMC driver for virtio DMA.

* `--readonly`

  This option sets the access to disks or partitions to read only for the
  preceding `client` option.

* `--dma-map-all`

  Map the entire client dataspace into the DMA space at the first I/O request
  and never unmap the dataspace until the client is destroyed. The default
  behavior is to map the relevant part of the dataspace before an I/O request
  and unmap it after the request.


## Connecting a client

Prior to connecting a client to a virtual block session it has to be created
using the following Lua function. It has to be called on the client side of the
IPC gate capability whose server side is bound to the eMMC driver.

    create(obj_type, "device=<UUID>", "ds-max=<max>")

* `obj_type`

  The type of object that should be created by the driver. The type must be a
  positive integer. Currently the following objects are supported:
  * `0`: Virtio block host

* `"device=<UUID>"`

  This string denotes a partition UUID the client wants to be exported via the
  Virtio block interface.

* `"ds-max=<max>"`

  Specifies the upper limit of the number of dataspaces the client is allowed
  to register with the eMMC driver for virtio DMA.

* `"readonly"`

  This option sets the access to disks or partitions to read only for this
  client connection.

* `"dma-map-all"`

  Map the entire client dataspace into the DMA space at the first I/O request
  and never unmap the dataspace until the client is destroyed. The default
  behavior is to map the relevant part of the dataspace before an I/O request
  and unmap it after the request.

If the `create()` call is successful a new capability which references an eMMC
virtio driver is returned. A client uses this capability to communicate with
the eMMC driver using the Virtio block protocol.


## Recognized capabilities

The driver makes use of certain capabilities:

* `vbus`

  Required for finding the device which should be driven by this driver.

* `bbds`

  **Only used by the SDHCI driver.**  
  Certain SDHCI devices cannot handle DMA requests with DMA buffers beyond 4GiB.
  The provided dataspace is used as bounce buffer if the driver detects that a
  certain request needs it.  
  **Note:** The bounce buffer needs to be able to hold the memory for an entire
  read/write request. That means that the buffer is divided into the number of
  maximum segments (see `--max-seg` parameter). 
  **Note:** The physical memory of the provided dataspace must be contiguous.

* `sdhci_adma_buf`

  **Only used by the SDHCI driver.**  
  Page (4096 bytes) for storing DMA descriptors for the SDHCI driver. If this
  capability is not provided, the driver will allocate an arbitrary page.

* `bcm2835_mbox_mem`

  **Only used by the SDHCI driver when attaching to an bcm2711-compatible device.**  
  Page (4096 bytes) for storing bcm2835 mbox messages. The firmware mbox is used
  to perform voltage switching for certain SD card configurations. If this
  capability is not provided, the driver will allocate an arbitrary page.


## Examples

A couple of examples on how to request different disks or partitions are listed
below.

* Request a partition with the given UUID

      vda1 = emmc_bus:create(0, "ds-max=5", "device=AFFA05B0-9379-480E-B9C6-5FF57FB1D194")

* A more elaborate example with a static client. The client uses the client
  side of the `emmc_cl1` capability to communicate with the eMMC driver.

      local emmc_cl1 = L4.default_loader:new_channel();
      local emmc_bus = L4.default_loader:new_channel();
      L4.default_loader:start({
        caps = {
          vbus = vbus_emmc,
          svr = emmc_bus:svr(),
          cl1 = emmc_cl1:svr(),
        },
      },
      "rom/emmc-drv --client cl1 --device 88E59675-4DC8-469A-98E4-B7B021DC7FBE --ds-max 5");

* Accessing a device from QEMU:

  The file `pcie-ecam.io` contains an IO config file which is able to use the
  QEMU PCI controller to search for attached eMMC devices.

* eMMC emulation with QEMU:

  The attached patch extends QEMU SD card emulation to emulate eMMC devices.
  After applying the patch and recompiling QEMU, attach the following parameters
  to your QEMU command line (assuming that `$HOME/foobar.img` is the eMMC medium):
```
-drive id=sd_disk,file=$(HOME)/foobar.img,if=none,format=raw \
-device sdhci-pci,id=sdhci \
-device sd-card,drive=sd_disk,spec_version=3,emmc=on
```
