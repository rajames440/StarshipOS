# Fiasco.OC Microkernel (L4Re) — StarshipOS Kernel Module

This module integrates the [Fiasco.OC microkernel](https://l4re.org/fiasco) into the StarshipOS build system.
It provides a Maven-driven interface to build, package, and distribute the kernel as a classified tarball.

---

## 📌 About Fiasco.OC

Fiasco.OC is a modern, small, fast, and secure L4-family microkernel designed for real-time, virtualization, and
time-sharing systems.
It scales from embedded devices to complex multi-core platforms.

### Supported Architectures

| Architecture | 32-bit | 64-bit |       Status       |
|:------------:|:------:|:------:|:------------------:|
|     x86      |   ✅    |   ✅    |  ![x86 Build][3]   |
|     ARM      |   ✅    |   ✅    |  ![ARM Build][4]   |
|     MIPS     |   ✅    |   ✅    |  ![MIPS Build][5]  |
|    RISC-V    |   ✅    |   ✅    | ![RISC-V Build][6] |

For a full list of platforms and features, see the [Fiasco feature list][1].

---

## 🔧 Building (Maven-based in StarshipOS)

This module replaces traditional Makefile workflows with a Maven-based build pipeline. It:

* Builds Fiasco using `starship:build-fiasco`
* Packages the result via `maven-assembly-plugin`
* Installs a classified artifact to `.m2` as:

  org.starship\:fiasco.tar.gz\:x86\_64

### To Build

```
mvn clean install
```

This will:

1. Run `make O=build` internally
2. Package the `build/` output
3. Produce: `target/fiasco-1.0.0-SNAPSHOT-x86_64.tar.gz`
4. Install the tarball for use by `starship-l4-deps`

---

## 🛠 Upstream Build Instructions (if building manually)

To build outside Maven (for reference or testing):

```
make BUILDDIR=/path/to/build
cd /path/to/build
make menuconfig  # Optional configuration
make -j$(nproc)  # Build with parallel jobs
```

The resulting kernel will be: `build/fiasco`

See [Upstream Fiasco Build Instructions](https://l4re.org/fiasco/build.html) for details.

---

## 🔐 License

Fiasco.OC is licensed under **GPLv2**.
For alternate licensing, contact: [info@kernkonzept.com](mailto:info@kernkonzept.com)

---

## 📬 Vulnerability Reporting

Please disclose vulnerabilities responsibly via:
**[security@kernkonzept.com](mailto:security@kernkonzept.com)**

---

## 🧑‍🤝‍🧑 Contributing

See [How to Contribute](https://kernkonzept.com/L4Re/contributing/fiasco) for contribution guidelines.

---

[1]: https://l4re.org/fiasco/features.html
[2]: https://kernkonzept.com/L4Re/contributing/fiasco
[3]: https://github.com/kernkonzept/fiasco/actions/workflows/check_build_x86.yml/badge.svg?branch=master
[4]: https://github.com/kernkonzept/fiasco/actions/workflows/check_build_arm.yml/badge.svg?branch=master
[5]: https://github.com/kernkonzept/fiasco/actions/workflows/check_build_mips.yml/badge.svg?branch=master
[6]: https://github.com/kernkonzept/fiasco/actions/workflows/check_build_riscv.yml/badge.svg?branch=master
