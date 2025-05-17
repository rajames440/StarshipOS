# starship-bootstrap

This module builds `bootstrap.jar`, the initial startup artifact for StarshipOS.
It contains early system services, configuration logic, and the entry point for the StarshipOS runtime.

---

## 🚀 Purpose

`bootstrap.jar` serves as the first Java process launched by `jvm_server`. It:

* Initializes core OSGi bundles
* Starts system message buses (e.g., ActiveMQ)
* Loads early system configuration
* Optionally starts the user shell or GUI layer

---

## 🔍 Contents

* `org.starship.OSGiManager`: primary bootstrap class
* Any support classes required to bootstrap the JVM runtime
* Future: provisioning logic, system profile loading, early bundle discovery

---

## 🌐 Deployment

This module is:

* Built as a plain Maven JAR
* Copied into `target/l4-deps/bootstrap.jar` by `starship-l4-deps`
* Later placed in ROMFS and launched by `jvm_server` under L4Re

---

## 📄 Building

```bash
mvn clean install
```

This will compile and install `bootstrap.jar` to your local `.m2` for downstream packaging.

---

## ⚙️ Notes

* This module is expected to grow into the StarshipOS system initialization layer
* It may evolve into a provisioning manager or embedded controller as StarshipOS expands
* It operates outside OSGi until the framework is initialized

---

## 🔐 License

Licensed under the Apache License 2.0.
See `LICENSE` for full terms.
