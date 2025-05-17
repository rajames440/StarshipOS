# starship-jvm-server

This module builds `jvm_server.jar`, which defines metadata and resources associated with the native `jvm_server` binary
used in the StarshipOS boot process.

It is a placeholder for:

* Startup configuration metadata
* Manifest declarations for classpath construction
* Future service discovery hooks

---

## 🚀 Purpose

Although it contains no Java code (yet), this JAR exists to:

* Act as a logical counterpart to the native `jvm_server` binary
* Enable Maven-based dependency management for Java-side components
* Provide a staging ground for future `jvm_server` runtime behavior (e.g., logging, tracing, or self-registration)

---

## 🏠 Deployment

* Packaged as `jvm_server.jar`
* Copied by `starship-l4-deps` to `target/l4-deps/`
* Later placed in ROMFS alongside `bootstrap.jar` and the OpenJDK runtime

---

## 📄 Build Instructions

```bash
mvn clean install
```

This builds and installs `jvm_server.jar` to your local `.m2` for reuse during L4 assembly.

---

## ⚙️ Future Scope

This module may be expanded to include:

* Classpath bootstrap logic for native ↔ JVM handoff
* Manifest-based bundle wiring for early startup
* A Java-facing mirror of native event and system state

---

## 🔐 License

Licensed under the Apache License 2.0.
See `LICENSE` for details.
