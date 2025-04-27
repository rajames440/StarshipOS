# l4vio_switch, a virtual network switch   {#l4re_servers_vio_switch}

The virtual network switch connects multiple clients with a virtual network
connection. It uses Virtio as the transport mechanism. Each virtual switch port
implements the host-side of a Virtio network device (virtio-net).

The virtual network switch can be setup to feature exactly one monitor port.
All traffic passing through the switch is mirrored to the monitor port. The
monitor port is read-only, and has no TX capability.
An optional packet filter can be configured and implemented to filter data
sent to the monitor port.

## Configuration

Certain features of the virtual network switch are configurable at
compile-time. Configuration is done through the build-time configuration of
the L4Re build tree.

## Starting the service

The virtual network switch can be started in Ned like this:

    local switch = L4.default_loader:new_channel();
    L4.default_loader:start(
    {
      caps = {
        svr = switch:svr(),
      },
    },
    "rom/l4vio_switch");

First a communication channel (`switch`) is created which is used to create
virtual network ports. It is connected to the switch component via its
mandatory `svr` capability. See the section below on how to create a new
virtual port and connect a client to it.

### Options

In the example above the virtual network switch is started in its default
configuration with a maximum of 5 virtual ports. To customize the configuration
the virtual network switch accepts the following command line options:

* `-D <component=level>`, `--debug <component=level>`

  Configure individual debug levels per component. Allowed components are:
  > `core`, `virtio`, `port`, `request`, `queue`, `packet`

  Possible debug levels with increasing verbosity are:
  > `quiet`, `warn`, `info`, `debug`, `trace`

* `-m`, `--mac`

  Enable the switch to set the MAC address for each client. An explicitly set
  MAC address of a port is always forwarded to a client.

* `-p <num>`, `--ports <num>`

  Set the maximum number of virtual ports. The default is 5.

* `-q`, `--quiet`

  Silence all output except for error messages.

* `-s <num>`, `--size <num>`

  Set the maximum queue size for the device-side Virtio queues.
  Must be a power of 2 in the range of 1 to 32768 inclusive.

* `-v`, `--verbose`

  Increase the global verbosity level. Individual levels per component can be
  set using the `-D` option.

* `-d <cap_name>`, `--register-ds <cap_name>`
  Register a trusted dataspace capability. If this option gets used, it is not
  possible to communicate with the server via dataspaces other than the
  registered ones. Can be used multiple times for multiple dataspaces.

  The option parameter is the name of a dataspace capability.

### Hardware devices
To plug hardware devices into the switch, provide a Vbus capability with the
name `vbus` when starting the switch.
To use this feature, you have to enable the `VNS_IXL` config option.

## Connecting a client

First, a virtual network port has to be created using the following Ned-Lua
function. It has to be called on the communication channel called `switch`,
which has been created earlier.

    create(obj_type, ["ds-max=<max>", "name=<name>", "type=<port type>",
                      "vlan=<options>", "mac=<mac_address>"])

* `obj_type`

  The type of object that should be created by the switch. The type must be a
  positive integer. Currently the following objects are supported:
  * `0`: Virtual switch port

* `ds-max=<max>`

  Specifies the upper limit of the number of dataspaces the client is allowed
  to register with the virtual network switch for Virtio DMA.

* `name=<name>`

  Sets the name of port in debug messages to `<name>`.  A name may consist of
  at most 19 characters, all other characters are dropped. If there is enough
  space left, the name will get a postfix of "[<port number>]", e.g. "name=foo"
  -> foo[1].

* `type=<port type>`

  Optionally specify the port type, either `normal` or a `monitor` port. Valid
  types are `[monitor|normal]`. The default is `type=normal` (if no type is
  given).

* `vlan=(access=<vlan id>|trunk=[<vlan id>[,<vlan id>]*])`

  Configure the port to participate in an IEEE 802.1Q compatible VLAN.
  Fundamentally there are two types of ports: access ports and trunk ports:

  * `vlan=access=<vlan id>`

    Configures the port as access port for VLAN `<vlan id>` where the id must
    be a decimal number greater than 0 and less than 4095 in accordance to the
    standard. Packets on an access port belong to the configured VLAN and are
    only forwarded to ports that belong to the same VLAN or trunk ports that
    participate in the particular VLAN. The packets on this port will not have
    a VLAN tag attached to them so that a guest connected to this port does not
    see that the port is part of a VLAN.

    An optional monitor port will see packets from an access port as VLAN
    tagged packets with the `<vlan id>` given for the port.

  * `vlan=trunk=all|[<vlan id>[,<vlan id>]*]`

    Configures the port as trunk port. It participates either in all VLANs, if
    specified by the keyword 'all', or in the list of VLANs given as comma
    separated list. There must be no whitespace in the list. Each id must be a
    decimal number greater than 0 and less than 4095 in accordance to the
    standard. Outgoing packets on this port will be tagged with an IEEE 802.1Q
    compatible tag. Incoming packets must be tagged with a VLAN tag from the
    given list. Packets that have no tag or a tag not in the vlan id list are
    dropped silently. They are not forwarded to the monitor port either.

    Currently there is no support for IEEE 802.1p. The PCP and DEI sub-fields
    in the TCI field will be set to zero on outgoing packets and are ignored
    for incoming packets.

* `mac=xx:xx:xx:xx:xx:xx`

  Explicitly sets the MAC address of the port. It will be checked that no other
  port on the switch has the same address. It is the responsibility of the user
  to ensure the validity of the address and its global uniqueness, though.

If the `create()` call is successful a new capability which references a
virtual switch port is returned. A client uses this capability to talk to the
virtual network switch using the Virtio network protocol.

Here are couple of examples on how to create ports with different properties:

    -- normal port with at most 4 data spaces
    net0 = switch:create(0, "ds-max=4")
    -- like the previous but with name foo
    net0 = switch:create(0, "ds-max=4", "name=foo")
    -- like the previous but the port is a monitor port
    net0 = switch:create(0, "ds-max=4", "name=foo", "type=monitor")
    -- normal port with 4 data spaces as access port to VLAN 1
    net0 = switch:create(0, "ds-max=4", "name=vl1", "vlan=access=1")
    -- normal port with 4 data spaces as trunk port participating in VLAN 1 & 2
    net0 = switch:create(0, "ds-max=4", "name=vl1", "vlan=trunk=1,2")
