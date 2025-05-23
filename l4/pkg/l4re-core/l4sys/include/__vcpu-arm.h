/*
 * (c) 2017 Alexander Warg <alexander.warg@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

typedef struct l4_arm_vcpu_e_info_t
{
  l4_uint8_t  version;     // must be 0
  l4_uint8_t  gic_version;
  l4_uint8_t  _rsvd0[2];
  l4_uint32_t features;
  l4_uint32_t _rsvd1[14];
  l4_umword_t user[8];
} l4_arm_vcpu_e_info_t;

L4_INLINE void *l4_vcpu_e_ptr(void const *vcpu, unsigned id) L4_NOTHROW;

enum L4_vcpu_e_consts
{
  L4_VCPU_E_NUM_LR = 4, /**< Number of list registers (LRs) */
};

L4_INLINE l4_arm_vcpu_e_info_t const *
l4_vcpu_e_info(void const *vcpu) L4_NOTHROW;

L4_INLINE l4_umword_t *
l4_vcpu_e_info_user(void *vcpu) L4_NOTHROW;

L4_INLINE l4_umword_t *
l4_vcpu_e_info_user(void *vcpu) L4_NOTHROW
{
  return ((l4_arm_vcpu_e_info_t *)l4_vcpu_e_info(vcpu))->user;
}


/**
 * Read a 32bit field from the extended vCPU state.
 *
 * \param vcpu  Pointer to the vCPU memory.
 * \param id    Field ID as defined in L4_vcpu_e_field_ids.
 * \returns The value stored in the field.
 */
L4_INLINE l4_uint32_t
l4_vcpu_e_read_32(void const *vcpu, unsigned id) L4_NOTHROW;

L4_INLINE l4_uint32_t
l4_vcpu_e_read_32(void const *vcpu, unsigned id) L4_NOTHROW
{ return *(l4_uint32_t const *)l4_vcpu_e_ptr(vcpu, id); }

/**
 * Write a 32bit field to the extended vCPU state.
 *
 * \param vcpu  Pointer to the vCPU memory.
 * \param id    Field ID as defined in L4_vcpu_e_field_ids.
 * \param val   The value to be written.
 */
L4_INLINE void
l4_vcpu_e_write_32(void *vcpu, unsigned id, l4_uint32_t val) L4_NOTHROW;

L4_INLINE void
l4_vcpu_e_write_32(void *vcpu, unsigned id, l4_uint32_t val) L4_NOTHROW
{ *((l4_uint32_t *)l4_vcpu_e_ptr(vcpu, + id)) = val; }

/**
 * Read a 64bit field from the extended vCPU state.
 *
 * \param vcpu  Pointer to the vCPU memory.
 * \param id    Field ID as defined in L4_vcpu_e_field_ids.
 * \returns The value stored in the field.
 */
L4_INLINE l4_uint64_t
l4_vcpu_e_read_64(void const *vcpu, unsigned id) L4_NOTHROW;

L4_INLINE l4_uint64_t
l4_vcpu_e_read_64(void const *vcpu, unsigned id) L4_NOTHROW
{ return *(l4_uint64_t const *)l4_vcpu_e_ptr(vcpu, id); }

/**
 * Write a 64bit field to the extended vCPU state.
 *
 * \param vcpu  Pointer to the vCPU memory.
 * \param id    Field ID as defined in L4_vcpu_e_field_ids.
 * \param val   The value to be written.
 */
L4_INLINE void
l4_vcpu_e_write_64(void *vcpu, unsigned id, l4_uint64_t val) L4_NOTHROW;

L4_INLINE void
l4_vcpu_e_write_64(void *vcpu, unsigned id, l4_uint64_t val) L4_NOTHROW
{ *((l4_uint64_t *)l4_vcpu_e_ptr(vcpu, id)) = val; }

/**
 * Read a natural register field from the extended vCPU state.
 *
 * \param vcpu  Pointer to the vCPU memory.
 * \param id    Field ID as defined in L4_vcpu_e_field_ids.
 * \returns The value stored in the field.
 */
L4_INLINE l4_umword_t
l4_vcpu_e_read(void const *vcpu, unsigned id) L4_NOTHROW;

L4_INLINE l4_umword_t
l4_vcpu_e_read(void const *vcpu, unsigned id) L4_NOTHROW
{ return *(l4_umword_t const *)l4_vcpu_e_ptr(vcpu, id); }

/**
 * Write a natural register field to the extended vCPU state.
 *
 * \param vcpu  Pointer to the vCPU memory.
 * \param id    Field ID as defined in L4_vcpu_e_field_ids.
 * \param val   The value to be written.
 */
L4_INLINE void
l4_vcpu_e_write(void *vcpu, unsigned id, l4_umword_t val) L4_NOTHROW;

L4_INLINE void
l4_vcpu_e_write(void *vcpu, unsigned id, l4_umword_t val) L4_NOTHROW
{ *((l4_umword_t *)l4_vcpu_e_ptr(vcpu, id)) = val; }
