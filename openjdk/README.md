# OpenJDK Runtime — StarshipOS Integration

This module builds and packages OpenJDK 21 as part of the StarshipOS toolchain.
It integrates the upstream OpenJDK source tree into a Maven workflow,
and packages the resulting JDK runtime for inclusion in the L4 ROMFS.

---

## 🎓 Upstream README Summary

Welcome to the JDK!

For build instructions please see the
[online documentation](https://openjdk.org/groups/build/doc/building.html),
or either of these files:

* [doc/building.html](doc/building.html) (HTML version)
* [doc/building.md](doc/building.md) (Markdown version)

See [https://openjdk.org/](https://openjdk.org/) for more information about the OpenJDK
Community and the JDK, and see [https://bugs.openjdk.org](https://bugs.openjdk.org) for JDK issue
tracking.

---

## 🚀 StarshipOS Role

This module is responsible for:

* Compiling OpenJDK from source using `starship:build-jdk`
* Producing a complete JDK tree in `build/jdk/`
* Packaging the result as `openjdk-<version>-x86_64.tar.gz`
* Making the JDK runtime available to `starship-l4-deps` and ultimately the `l4` ROMFS

---

## 🔧 Future Scope

This module will evolve into a core part of the **StarshipOS SDK**,
providing both the target runtime (ROMFS) and the host SDK
(for building system bundles and user apps).

In the future, it may:

* Export a Starship-customized `jlink` image
* Include JavaFX, Panama, and other platform libraries
* Contribute to system entropy and GC instrumentation models

---

## 📄 Building via Maven

To build and install the OpenJDK tarball:

```bash
mvn clean install
```

This will:

1. Compile OpenJDK using the custom plugin
2. Package the runtime image via `maven-assembly-plugin`
3. Install the resulting tarball to `.m2` for downstream use
