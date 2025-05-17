# starship-maven-plugin

This module defines the custom Maven plugin that powers the entire StarshipOS build process.
It provides goals for building each system component (Fiasco, L4Re, OpenJDK, ROMFS, ISO), staging artifacts,
and wiring up cross-architecture toolchains.

---

## 🚀 Purpose

This plugin acts as the glue between Maven and external native build systems (Make, Ant, etc).
Each goal corresponds to a specific system layer:

* `build-fiasco` — compiles the Fiasco microkernel
* `build-jdk` — compiles OpenJDK
* `build-l4` — builds the L4Re runtime
* `build-iso` — generates a bootable ISO
* `run-qemu` — boots the system under QEMU
* `initialize` — sets up developer toolchains and project metadata

More goals are expected to be added over time for tasks such as cleaning, packaging, validation, and deployment.

---

## ⚖️ Mojo Architecture

Each goal is implemented as a `Mojo` class under `org.starship.mojo`.
Mojos inherit from `AbstractStarshipMojo`, which provides:

* Project root resolution
* Logging integration
* `.starship` properties file handling
* Common build hooks

Utility classes support Mojos with reusable logic and resource handling.

---

## 🔍 Key Features

* Plugin descriptor generated via `maven-plugin-plugin`
* Built as a true Maven plugin (packaging: `maven-plugin`)
* Available for local `mvn` invocation in any StarshipOS build environment

---

## 📄 Build & Use

To build the plugin:

```bash
mvn clean install
```

To invoke a goal:

```bash
mvn starship:<goal>
```

For example:

```bash
mvn starship:build-fiasco
```

---

## 🔐 License

Licensed under the Apache License 2.0.
See `LICENSE` for full details.
