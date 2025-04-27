# Atkins

Atkins provides tools for testing L4Re applications.

libgtest and libgmock are provided as the basic test libraries. They can be
used in both host and target mode, making it possible to write both functional
tests and basic target-independent unit tests.  The host versions are currently
not built but can be enabled in `.gmock/Makefile` and `.gtest/Makefile`,
respectively.

In addition, the `./atkins` directory contains utilities and fixtures
specifically for the L4Re environment.

# Documentation

This package is part of the L4Re Operating System Framework. For documentation
and build instructions see the [L4Re
wiki](https://kernkonzept.com/L4Re/guides/l4re).

# Contributions

We welcome contributions. Please see our contributors guide on
[how to contribute](https://kernkonzept.com/L4Re/contributing/l4re).

# License

Detailed licensing and copyright information can be found in
the [LICENSE](LICENSE.spdx) file.
