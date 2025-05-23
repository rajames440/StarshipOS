/**
 * \internal
 * \file
 * \brief  IPC system call for x86
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Jork Löser <jork@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#pragma once

#include <l4/sys/consts.h>

L4_INLINE l4_msgtag_t
l4_ipc(l4_cap_idx_t dest, l4_utcb_t *u,
       l4_umword_t flags,
       l4_umword_t slabel,
       l4_msgtag_t tag,
       l4_umword_t *rlabel,
       l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy, dummy1, dummy2;

  (void)u;

  __asm__ __volatile__
    (L4_ENTER_KERNEL
     :
     "=d" (dummy2),
     "=S" (slabel),
     "=c" (dummy1),
     "=D" (dummy),
     "=a" (tag.raw)
     :
     "S" (slabel),
     "c" (timeout),
     "a" (tag.raw),
     "d" (dest | flags)
     L4S_PIC_SYSCALL
     :
     "memory", "cc" L4S_PIC_CLOBBER
     );

  if (rlabel)
    *rlabel = slabel;

  return tag;
}
