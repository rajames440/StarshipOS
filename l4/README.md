# L4Re Build System — StarshipOS Integration

This module wraps the upstream L4Re source tree and build system within a Maven-based structure
for StarshipOS.

It serves as the entry point for building the user-level runtime, boot environment, and ROMFS image
based on inputs provided by `starship-l4-deps`.

---

## 🔧 What This Module Does

* Extracts `starship-l4-deps` into a build staging area
* Uses Maven plugin `starship:build-l4` to invoke the L4Re native `make` system
* Links against prebuilt kernel and JDK assets
* Generates a bootable image (e.g., ISO or QEMU-ready) with a `romfs`

### Build Output (Post-Compile)

* ROMFS-ready binaries and JARs in `build/`
* ISO or ELF images in `build/x86_64/`
* Log and dependency trace artifacts

---

## 📄 Upstream README Summary

This package contains the build system for the L4Re operating system.
It also provides the basic directory infrastructure for L4Re.

### Documentation

This package is part of the L4Re operating system. For documentation and
build instructions see the [L4Re wiki](https://kernkonzept.com/L4Re/guides/l4re).

Packages for additional parts of the L4Re operating system are located
in `pkg/` and `test/`. Please refer to the READMEs in each directory for
additional information about each package.

### Contributions

We welcome contributions. Please see our contributors guide on
[how to contribute](https://kernkonzept.com/L4Re/contributing/l4re).

### License

Detailed licensing and copyright information can be found in the
[LICENSE](LICENSE.spdx) file. Licensing information for packages
in `pkg/` and `test/` are located in each subdirectory.

---

## 🚫 Note

This module does **not** contain Fiasco or OpenJDK. These are injected via the `starship-l4-deps` archive.
The `generate-resources` phase prepares the environment before invoking native L4Re compilation.
