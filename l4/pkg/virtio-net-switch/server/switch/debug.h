/*
 * (c) 2013-2014 Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <l4/re/util/debug>

struct Err : L4Re::Util::Err
{
  Err(Level l = Fatal) : L4Re::Util::Err(l, "VSwitch") {}
};

class Dbg : public L4Re::Util::Dbg
{
  enum
  {
    Verbosity_shift = 4, /// Bits per component for verbosity
    Verbosity_mask = (1UL << Verbosity_shift) - 1
  };

public:
  /// Verbosity level per component.
  enum Verbosity : unsigned long
  {
    Quiet = 0,
    Warn = 1,
    Info = 2,
    Debug = 4,
    Trace = 8,
    Max_verbosity = 8
  };

  /**
   * Different components for which the verbosity can be set independently.
   */
  enum Component
  {
    Core = 0,
    Virtio,
    Port,
    Request,
    Queue,
    Packet,
    Max_component
  };

#ifndef NDEBUG

  static_assert(Max_component * Verbosity_shift <= sizeof(level) * 8,
                "Too many components for level mask");
  static_assert((Max_verbosity & Verbosity_mask) == Max_verbosity,
                "Verbosity_shift to small for verbosity levels");

  /**
   * Set the verbosity for all components to the given levels.
   *
   * \param mask  Mask of verbosity levels.
   */
  static void set_verbosity(unsigned mask)
  {
    for (unsigned i = 0; i < Max_component; ++i)
      set_verbosity(i, mask);
  }

  /**
   * Set the verbosity of a single component to the given level.
   *
   * \param c     Component for which to set verbosity.
   * \param mask  Mask of verbosity levels.
   */
  static void set_verbosity(unsigned c, unsigned mask)
  {
    level &= ~(Verbosity_mask << (Verbosity_shift * c));
    level |= (mask & Verbosity_mask) << (Verbosity_shift * c);
  }

  /**
   * Check whether debugging is active for a component and verbosity level.
   *
   * \param  c     Component for which to check verbosity.
   * \param  mask  Mask of verbosity levels.
   *
   * \retval true   Debugging is active.
   * \retval false  Debugging is not active.
   */
  static bool is_active(unsigned c, unsigned mask)
  { return level & (mask & Verbosity_mask) << (Verbosity_shift * c); }

  /**
   * Check whether debugging is active for the current debug object.
   *
   * \retval true   Debugging is active.
   * \retval false  Debugging is not active.
   */
  using L4Re::Util::Dbg::is_active;

  Dbg(Component c = Core, Verbosity v = Warn, char const *subsys = "")
  : L4Re::Util::Dbg(v << (Verbosity_shift * c), "SWI", subsys)
  {}

#else

  static void set_verbosity(unsigned) {}
  static void set_verbosity(unsigned, unsigned) {}

  static bool is_active(unsigned, unsigned) { return false; }
  using L4Re::Util::Dbg::is_active;

  Dbg(Component c = Core, Verbosity v = Warn, char const *subsys = "")
  : L4Re::Util::Dbg(v << (Verbosity_shift * c), "", subsys)
  {}

#endif
};
