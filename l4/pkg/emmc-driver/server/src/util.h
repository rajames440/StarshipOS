/*
 * Copyright (C) 2020, 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Miscellaneous utility functions (fine-grained clock, logging convenience
 * functions).
 */

#pragma once

#include <string>
#include <functional>

struct Util
{
  /// Return descriptive string like '5.6MiB' or similar.
  static std::string readable_size(l4_uint64_t size);

  /// Return descriptive string like '6.7MHz' or similar.
  static std::string readable_freq(l4_uint32_t freq);

  static char printable(char c) { return c >= ' ' ? c : ' '; }

  typedef std::function<bool ()> Poll_timeout_handler;

  /**
   * Poll `handler` for a certain amount of time.
   *
   * \param us       Polling interval in microseconds with granularity of the
   *                 kernel clock (usually milliseconds).
   * \param handler  The function to be called while polling. If this function
   *                 returns true, the polling loop is aborted.
   *
   * \retval false The handler did not return true within the polling interval.
   *               Actually this cannot happen: If the polling interval is
   *               exceeded, an exception is triggered.
   * \retval true The handler returned true within the polling interval.
   */
  static bool poll(l4_cpu_time_t us, Poll_timeout_handler handler,
                   char const *s);

  /**
   * Determine if fine-grained clock available.
   *
   * The fine-grained clock is usually only used for tracing but certain drivers
   * might require it for other purposes. For example, the iproc SDHCI driver
   * uses it for delayed writing.
   *
   * \retval false Fine-grained clock not available.
   * \retval true Fine-grained clock available.
   */
  static bool tsc_available()
  {
    return tsc_init_success;
  }

  /**
   * Initialize fine-grained clock.
   */
  static void tsc_init();

  /**
   * Busy wait for a short amount of time.
   *
   * \param us  The amount of time to wait.
   *
   * \note This requires that tsc_init() succeded.
   */
  static void busy_wait_us(l4_uint64_t us);

  /**
   * Busy wait for a short amount of time.
   *
   * \param until  The absolve end time for the waiting period. Function will
   *               return immediately if this is in the past.
   */
  static void busy_wait_until(l4_uint64_t until);

  /**
   * Returns always 0 if no fine-grained clock available.
   *
   * This is no problem for tracing but if accurate time stamps are required for
   * polling, tsc_init() will fail early.
   */
  static l4_uint64_t read_tsc()
  {
#if defined(ARCH_arm)
    l4_uint64_t v;
    asm volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (v));
    return v;
#elif defined(ARCH_arm64)
    l4_uint64_t v;
    asm volatile("mrs %0, CNTVCT_EL0" : "=r" (v));
    return v;
#elif defined(ARCH_x86) || defined(ARCH_amd64)
    l4_umword_t lo, hi;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return (l4_uint64_t{hi} << 32) | lo;
#else
    return 0;
#endif
  }

  /// Returns 0 if no fine-grained clock available.
  static l4_uint64_t freq_tsc_hz()
  {
#if defined(ARCH_arm)
    return generic_timer_freq;
#elif defined(ARCH_arm64)
    return generic_timer_freq;
#elif defined(ARCH_x86) || defined(ARCH_amd64)
    return cpu_freq_khz * 1000;
#else
    return 0;
#endif
  }

  /// Returns 0 if no fine-grained clock available.
  static l4_uint64_t tsc_to_us(l4_uint64_t tsc)
  {
#if defined(ARCH_arm) || defined(ARCH_arm64)
    l4_uint64_t freq = freq_tsc_hz();
    return freq ? tsc * 1000000 / freq : 0;
#elif defined(ARCH_x86)
    l4_uint32_t dummy;
    l4_uint64_t us;
    asm ("movl  %%edx, %%ecx \n\t"
         "mull  %3           \n\t"
         "movl  %%ecx, %%eax \n\t"
         "movl  %%edx, %%ecx \n\t"
         "mull  %3           \n\t"
         "addl  %%ecx, %%eax \n\t"
         "adcl  $0, %%edx    \n\t"
        :"=A" (us), "=&c" (dummy)
        :"0" (tsc), "g" (scaler_tsc_to_us));
    return us;
#elif defined(ARCH_amd64)
    l4_uint64_t us, dummy;
    asm ("mulq %3; shrd $32, %%rdx, %%rax"
         :"=a"(us), "=d"(dummy)
         :"a"(tsc), "r"(static_cast<l4_uint64_t>(scaler_tsc_to_us)));
    return us;
#else
    static_cast<void>(tsc);
    return 0;
#endif
  }

  static l4_uint64_t tsc_to_ms(l4_uint64_t tsc)
  {
#if defined(ARCH_arm) || defined(ARCH_arm64)
    l4_uint64_t freq = freq_tsc_hz();
    return freq ? tsc * 1000 / freq : 0;
#elif defined(ARCH_x86) || defined(ARCH_amd64)
    return tsc_to_us(tsc) / 1000;
#else
    static_cast<void>(tsc);
    return 0;
#endif
  }

  static bool tsc_init_success;
#if defined(ARCH_x86) || defined(ARCH_amd64)
  static l4_uint32_t scaler_tsc_to_us;
  static l4_umword_t cpu_freq_khz;
#elif defined(ARCH_arm) || defined(ARCH_arm64)
  static l4_uint32_t generic_timer_freq;
#endif
};
