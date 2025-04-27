/*
 * Copyright (C) 2025 Kernkonzept GmbH.
 * Author(s): Martin Kuettler martin.kuettler@kernkonzept.com
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file   The DS3231 rtc. Is expected to always live on a I2C-Bus.
 */

// Documentation:
// https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf

#include "rtc.h"
#include "bcd.h"

#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/i2c-driver/i2c_device_if.h>

#include <cstdio>
#include <cstring>
#include <time.h>

class DS3231_rtc : public Rtc
{
private:
  using raw_t = l4_uint8_t[7];

  struct Reg_addr
  {
    enum
      {
        Seconds           = 0x00,
        Minutes           = 0x01,
        Hours             = 0x02,
        Wday              = 0x03,
        Mday              = 0x04,
        Month_and_century = 0x05,
        Year              = 0x06,
      };
  };

  // seconds in range 0..59
  static int seconds(raw_t data)
  {
    return bcd2bin(data[Reg_addr::Seconds]);
  }

  // minutes in range 0..59
  static int minutes(raw_t data)
  {
    return bcd2bin(data[Reg_addr::Minutes]);
  }

  // hours in range 0..23
  static int hours(raw_t data)
  {
    if (data[Reg_addr::Hours] & 0x40) // am/pm format
      {
        int const value = data[Reg_addr::Hours];
        return (value & 0x0f)
          + ((value & 0x10) ? 10 : 0)
          + ((value & 0x20) ? 12 : 0);
      }
    else // 24h format
      {
        int const value = data[Reg_addr::Hours];
        return (value & 0x0f)
          + ((value & 0x10) ? 10 : 0)
          + ((value & 0x20) ? 20 : 0);
      }
  }

  // month day in range 0..31
  static int mday(raw_t data)
  {
    int const value = data[Reg_addr::Mday] & 0x3f;
    return bcd2bin(value);
  }

  // month in range 0..11
  static int month(raw_t data)
  {
    return bcd2bin(data[Reg_addr::Month_and_century] & 0x1f) - 1;
  }

  // year in range 100..299, representing 2000..2199 (offset 1900)
  static int year(raw_t data)
  {
    return bcd2bin(data[Reg_addr::Year])
      + ((data[Reg_addr::Month_and_century] & 0x80) ? 100 : 0)
      + 100;
  }

  // set the seconds value in data
  static void set_seconds(int sec, raw_t data)
  {
    data[Reg_addr::Seconds] = bin2bcd(sec);
  }

  // set the minutes value in data
  static void set_minutes(int min, raw_t data)
  {
    data[Reg_addr::Minutes] = bin2bcd(min);
  }

  // set the hours value in data
  static void set_hours(int hours, raw_t data)
  {
    // this sets the 24h format.
    l4_uint8_t val = hours % 10;
    if (hours >= 10 && hours <= 19)
      val |= 0x10;
    else if (hours >= 20)
      val |= 0x20;
    data[Reg_addr::Hours] = val;
  }

  // set the week day value in data
  static void set_wday(int wday, raw_t data)
  {
    data[Reg_addr::Wday] = wday;
  }

  // set the month day value in data
  static void set_mday(int mday, raw_t data)
  {
    data[Reg_addr::Mday] = bin2bcd(mday);
  }

  // set the month value in data
  // the value passed here is in range 0 .. 11
  static void set_month(int month, raw_t data)
  {
    data[Reg_addr::Month_and_century] &= 0xc0;
    data[Reg_addr::Month_and_century] |= bin2bcd(month + 1);
  }

  // set the year value in data
  // due to internal encoding this needs to touch two elements of data
  static void set_year(int year, raw_t data)
  {
    // we expect the year as stored in struct tm, i.e., relative to 1900
    year = year - 100;
    data[Reg_addr::Year] = bin2bcd(year % 100);
    if (year > 100)
      data[Reg_addr::Month_and_century] |= 0x80;
    else
      data[Reg_addr::Month_and_century] &= 0x7f;
  }

public:
  bool probe()
  {
    _ds3231 = L4Re::Env::env()->get_cap<I2c_device_ops>("ds3231");
    if (_ds3231.is_valid())
      {
        l4_uint64_t time;
        if (int err = get_time(&time); err != L4_EOK)
          {
            printf("get_time() in probe returned %d\n", err);
            return false;
          }
        l4_uint64_t ns = time + l4_kip_clock_ns(l4re_kip());
        time_t secs = ns / 1'000'000'000;
        char buf[26];
        ctime_r(&secs, buf);
        printf("Found DS3231 RTC. Current time: %s", buf);
        return true;
      }

    // TODO: check for alternative vbus containing i2c

    return false;
  }

  int set_time(l4_uint64_t offset_nsec)
  {
    l4_uint64_t const ns = offset_nsec + l4_kip_clock_ns(l4re_kip());
    time_t const sec = ns / 1'000'000'000ull;
    struct tm time;
    gmtime_r(&sec, &time);
    raw_t data = {0, 0, 0, 0, 0, 0, 0};
    set_year(time.tm_year, data);
    set_month(time.tm_mon, data);
    set_mday(time.tm_mday, data);
    set_wday(time.tm_wday, data);
    set_hours(time.tm_hour, data);
    set_minutes(time.tm_min, data);
    set_seconds(time.tm_sec, data);
    if (_ds3231.is_valid())
      {
        l4_uint8_t send_data[1 + sizeof(data)];
        send_data[0] = 0; // reg-addr to write to
        memcpy(&send_data[1], data, sizeof(data));
        L4::Ipc::Array<l4_uint8_t const> send_buffer{sizeof(send_data),
                                                     send_data};
        if (int err = _ds3231->write(send_buffer); err != L4_EOK)
          {
            printf("write time data returned error code %d\n", err);
            return err;
          }
      }
    else
      {
        printf("Direct device access is needed for now\n");
        return -L4_ENODEV;
      }
    return 0;
  }

  int get_time(l4_uint64_t *offset_nsec)
  {
    enum : unsigned int
      {
        Size = 7
      };
    l4_uint8_t data[Size];

    if (_ds3231.is_valid())
      {
        l4_uint8_t addr = 0;
        L4::Ipc::Array<l4_uint8_t const> send_buffer{sizeof(addr), &addr};
        L4::Ipc::Array<l4_uint8_t> buffer{Size, data};

        if (int err = _ds3231->write_read(send_buffer, buffer.length, buffer);
            err != L4_EOK)
          {
            printf("ERROR: writing and reading register returned %d\n", err);
            return err;
          }
      }
    else
      {
        printf("Direct device access is needed for now\n");
        return -L4_ENODEV;
      }

    struct tm time;
    time.tm_sec = seconds(data);
    time.tm_min = minutes(data);
    time.tm_hour = hours(data);
    time.tm_mday = mday(data);
    time.tm_mon = month(data);
    time.tm_year = year(data);
    time.tm_wday = 0; // this is ignored
    time.tm_yday = 0;
    time.tm_isdst = 0;
    time.tm_gmtoff = 0;
    time.tm_zone = nullptr;
    l4_uint64_t ns = timegm(&time) * 1'000'000'000ull;
    *offset_nsec = ns - l4_kip_clock_ns(l4re_kip());
    return 0;
  }

private:
  L4::Cap<I2c_device_ops> _ds3231;
};

static DS3231_rtc _ds3231;
