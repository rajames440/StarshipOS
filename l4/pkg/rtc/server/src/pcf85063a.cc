/*
 * Copyright (C) 2025 Kernkonzept GmbH.
 * Author(s): Martin Kuettler <martin.kuettler@kernkonzept.com>
 *            Andreas Wiese <andreas.wiese@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file   The PCF85063A rtc. Is expected to always live on the I2C-Bus.
 */

// Documentation:
// https://www.nxp.com/docs/en/data-sheet/PCF85063A.pdf

#include "rtc.h"
#include "bcd.h"

#include <l4/re/env>
#include <l4/util/util.h>
#include <l4/re/error_helper>
#include <l4/i2c-driver/i2c_device_if.h>

#include <cstdio>
#include <cstring>
#include <time.h>

class PCF85063A_rtc : public Rtc
{
private:
  struct Reg_addr
  {
    enum
      {
        Control_1 = 0x00,
        Control_2 = 0x01,
        Offset    = 0x02,
        RAM_byte  = 0x03,
        Seconds   = 0x04,
        Minutes   = 0x05,
        Hours     = 0x06,
        Mday      = 0x07,
        Wday      = 0x08,
        Month     = 0x09,
        Year      = 0x0a,

        Reg_size  = 11,
      };
  };

  using raw_t = l4_uint8_t[Reg_addr::Reg_size];

  /* convert register file to struct tm */
  static void raw2tm(raw_t data, struct tm &tm)
  {
    memset(&tm, 0, sizeof(tm));

    tm.tm_sec  = bcd2bin(data[Reg_addr::Seconds] & 0x7f);
    tm.tm_min  = bcd2bin(data[Reg_addr::Minutes] & 0x7f);
    tm.tm_hour =
      data[Reg_addr::Control_1] & 0x02 // 12hr mode?
      ? (bcd2bin(data[Reg_addr::Hours] & 0x1f) % 12
         + (data[Reg_addr::Hours] & 0x20 ? 12 : 0))
      : bcd2bin(data[Reg_addr::Hours] & 0x3f);
    tm.tm_mday = bcd2bin(data[Reg_addr::Mday] & 0x3f);
    tm.tm_mon  = bcd2bin(data[Reg_addr::Month] & 0x1f) - 1;
    tm.tm_year = bcd2bin(data[Reg_addr::Year]) + 100;
    tm.tm_wday = bcd2bin(data[Reg_addr::Wday] & 0x07);

    // missing, hence 0: tm_yday, tm_isdst, tm_gmtoff, tm_zone
  }

  /* convert struct tm to register file, touching only those bits which
     actually are date and time, leaving the others intact */
  static void tm2raw(struct tm &tm, raw_t &data)
  {
    /* don't mask out OS bit, but explicitly clear it */
    data[Reg_addr::Seconds] =   0;
    data[Reg_addr::Seconds] |=  0x7f & bin2bcd(tm.tm_sec);
    data[Reg_addr::Minutes] &= ~0x7f;
    data[Reg_addr::Minutes] |=  0x7f & bin2bcd(tm.tm_min);

    data[Reg_addr::Hours]   &= ~0x3f;
    if (data[Reg_addr::Control_1] & 0x02) /* 12hr mode */
      {
        /* Linux ignores the existence of this, maybe we should, too. */
        l4_uint8_t hr = tm.tm_hour;
        l4_uint8_t pmbit = hr / 12 ? 0x20 : 0;
        if ((hr %= 12) == 0) hr = 12;

        data[Reg_addr::Hours] |= 0x3f & (pmbit | bin2bcd(hr));
      }
    else
      {
        data[Reg_addr::Hours] |= 0x3f & bin2bcd(tm.tm_hour);
      }

    data[Reg_addr::Mday]  &= ~0x3f;
    data[Reg_addr::Mday]  |=  0x3f & bin2bcd(tm.tm_mday);
    data[Reg_addr::Wday]  &= ~0x07;
    data[Reg_addr::Wday]  |=  0x07 & bin2bcd(tm.tm_wday);
    data[Reg_addr::Month] &= ~0x1f;
    data[Reg_addr::Month] |=  0x1f & bin2bcd(tm.tm_mon + 1);
    data[Reg_addr::Year] = bin2bcd(tm.tm_year - 100);
  }

private:
  L4::Cap<I2c_device_ops> _pcf85063a;

  /* read register file */
  inline int read_data(raw_t &data)
  {
    l4_uint8_t addr = 0;
    L4::Ipc::Array<l4_uint8_t const> send_buffer{sizeof(addr), &addr};
    L4::Ipc::Array<l4_uint8_t> buffer{sizeof(data), data};

    if (int err = _pcf85063a->write_read(send_buffer, buffer.length, buffer);
        err != L4_EOK)
      {
        printf("ERROR: writing and reading register returned %d\n", err);
        return err;
      }

    return L4_EOK;
  }

  /* write register file */
  inline int write_data(raw_t &data)
  {
    l4_uint8_t buf[sizeof(data) + 1] = { 0, };
    memcpy(buf+1, data, sizeof(data));
    L4::Ipc::Array<l4_uint8_t const> send_buffer{sizeof(buf), buf};

    if (int err = _pcf85063a->write(send_buffer); err != L4_EOK)
      {
        printf("ERROR: writing registers returned %d\n", err);
        return err;
      }

    return L4_EOK;
  }

public:
  bool probe()
  {
    _pcf85063a = L4Re::Env::env()->get_cap<I2c_device_ops>("pcf85063a");
    if (!_pcf85063a.is_valid())
      return false;

    raw_t data;
    if (int err = read_data(data); err != L4_EOK)
      return false;

    if (data[Reg_addr::Seconds] & 0x80) {
      printf("Found PCF85063A RTC, but it experienced power-loss; time will be bogus until re-set.");
      return true;
    };

    l4_uint64_t nsecs;
    if (int err = get_time(&nsecs); err != L4_EOK)
      return false;
    nsecs += l4_kip_clock_ns(l4re_kip());

    {
      time_t secs = nsecs / 1'000'000'000ull;
      char buf[26];
      ctime_r(&secs, buf); /* time + '\n' */
      printf("Found PCF85063A RTC reports time is %s", buf);
    };

    return true;
  }

  int get_time(l4_uint64_t *offset_nsec)
  {
    raw_t data;

    if (!_pcf85063a.is_valid())
      return -L4_ENODEV;

    if (int err = read_data(data); err != L4_EOK)
      return err;

    if (data[Reg_addr::Seconds] & 0x80)
      {
        printf("WARNING: PCF85063A power loss detected, set_time() needed\n");
        return -1;
      }

    struct tm stime;
    raw2tm(data, stime);

    l4_uint64_t ns = timegm(&stime) * 1'000'000'000ull;
    *offset_nsec = ns - l4_kip_clock_ns(l4re_kip());

    return 0;
  }

  int set_time(l4_uint64_t offset_nsec)
  {
    raw_t data;
    struct tm stime;
    if (!_pcf85063a.is_valid())
      return -L4_ENODEV;

    if (int err = read_data(data); err != L4_EOK)
      return err;

    l4_uint64_t ns = l4_kip_clock_ns(l4re_kip()) + offset_nsec;
    time_t const secs = ns / 1'000'000'000ull;

    gmtime_r(&secs, &stime);
    tm2raw(stime, data);

    if (int err = write_data(data); err != L4_EOK)
      return err;

    /* TODO: double-check whether OS flag cleared? */

    return 0;
  }
};

static PCF85063A_rtc _pcf85063a;
