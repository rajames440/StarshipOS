/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
/**
 * \file vbus_types.h
 * This header file contains descriptions of vbus related data types and
 * constants.
 */
#pragma once

#include <l4/sys/types.h>

/** Device handle for a device on the vbus */
typedef l4_mword_t l4vbus_device_handle_t;
/** Address of resources on the vbus */
typedef l4_addr_t l4vbus_paddr_t;

/** Description of a single vbus resource */
typedef struct {
  /** Resource type, see \ref l4vbus_resource_type_t */
  l4_uint16_t    type;
  /** Flags */
  l4_uint16_t    flags;
  /** Start of resource range */
  l4vbus_paddr_t start;
  /** End of resource range (inclusive) */
  l4vbus_paddr_t end;
  /** Device handle of the provider of the resource */
  l4vbus_device_handle_t provider;
  /** Resource ID (4 bytes), usually a 4 letter ASCII name is used */
  l4_uint32_t id;
} l4vbus_resource_t;

/** Description of vbus resource types. */
enum l4vbus_resource_type_t {
  L4VBUS_RESOURCE_INVALID = 0, /**< Invalid type */
  L4VBUS_RESOURCE_IRQ,         /**< Interrupt resource */
  L4VBUS_RESOURCE_MEM,         /**< I/O memory resource */
  L4VBUS_RESOURCE_PORT,        /**< I/O port resource (x86 only) */
  L4VBUS_RESOURCE_BUS,         /**< Bus resource */
  L4VBUS_RESOURCE_GPIO,        /**< Gpio resource */
  L4VBUS_RESOURCE_DMA_DOMAIN,  /**< DMA domain */
  L4VBUS_RESOURCE_MAX,         /**< Maximum resource id */
};

/** Description of vbus resource flags. */
enum l4vbus_resource_flags_t {
  /** Memory resource is readable. */
  L4VBUS_RESOURCE_F_MEM_R = 0x1,
  /** Memory resource is writeable. */
  L4VBUS_RESOURCE_F_MEM_W = 0x2,
  /**
   * Memory resource is prefetchable.
   * Clients may map it buffered or non-cached.
   */
  L4VBUS_RESOURCE_F_MEM_PREFETCHABLE = 0x10,
  /**
   * Memory resource is cacheable.
   * This implies that the memory resource is prefetchable. If not set,
   * clients must not map it cached. If the resource is neither cacheable
   * nor prefetchable, clients must map it non-cached!
   */
  L4VBUS_RESOURCE_F_MEM_CACHEABLE = 0x20,
  /** Reading needs to be performed using the MMIO space protocol. */
  L4VBUS_RESOURCE_F_MEM_MMIO_READ = 0x2000,
  /** Writing needs to be performed using the MMIO space protocol. */
  L4VBUS_RESOURCE_F_MEM_MMIO_WRITE = 0x4000,
};

enum l4vbus_consts_t {
  L4VBUS_DEV_NAME_LEN = 64,
  L4VBUS_MAX_DEPTH = 100,
};

/** Detailed information about a vbus device */
typedef struct {
  /** Bitfield of supported sub-interfaces, see \ref l4vbus_iface_type_t */
  l4_uint32_t   type;
  /** Name */
  char          name[L4VBUS_DEV_NAME_LEN];
  /** Number of resources for this device */
  unsigned      num_resources;
  /** Flags, see \ref l4vbus_device_flags_t */
  unsigned      flags;
} l4vbus_device_t;

/** Flags describing device properties, see l4vbus_device_t. */
enum l4vbus_device_flags_t {
  L4VBUS_DEVICE_F_CHILDREN = 0x10, /**< Device has child devices. */
};
