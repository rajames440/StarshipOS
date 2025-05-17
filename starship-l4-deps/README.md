# starship-l4-deps

This module defines and assembles all runtime and build-time dependencies needed to compile and boot the StarshipOS L4
system.
It acts as the **single, authoritative dependency bundle** for the `l4` module.

---

## 🚀 Purpose

* Collects the kernel (Fiasco), JDK, and runtime JARs (`bootstrap.jar`, `jvm_server.jar`)
* Unpacks all tarballs and JARs into a standardized directory: `target/l4-deps/`
* Prepares a final `.tar.gz` bundle for consumption by the `l4` module

---

## 🔧 Contents

After `mvn generate-resources`, the directory `target/l4-deps/` will contain:

```
bootstrap.jar
jvm_server.jar
fiasco/           # kernel headers, build-time artifacts
openjdk/          # full JDK layout for ROMFS runtime
```

---

## 🏠 Role in Build Flow

This module:

* Depends on: `openjdk`, `fiasco`, `starship-bootstrap`, `starship-jvm-server`
* Runs: `maven-dependency-plugin` to unpack/copy everything into `target/l4-deps`
* Optionally runs: Ant tasks to copy or transform layout
* Packages: `target/l4-deps` into `starship-l4-deps-<version>-x86_64.tar.gz`
* Installs the archive with a classifier to local Maven repo for use by `l4`

---

## 🚪 Design Principles

* The L4 build process must consume *only* this tarball
* Future additions (native `.so`, AI libs, extra jars) go here — never directly in `l4`
* Separation between "runtime" and "build-time" content is enforced by directory layout

---

## 📄 Building

```bash
mvn clean install
```

This will:

1. Unpack all dependencies
2. Run `build.xml` Ant logic (if defined)
3. Package everything into a `.tar.gz`
4. Install it as:

```
org.starship:starship-l4-deps:x86_64:tar.gz
```

---

## 🔐 License

Licensed under the Apache License 2.0.
See `LICENSE` for full details.
