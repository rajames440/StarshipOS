# StarshipOS

StarshipOS is an experimental operating system that combines the robustness of the L4/Fiasco microkernel with a custom
JVM implementation. This project aims to create a secure, performant, and modular operating system for both x86\_64 and
ARM architectures.

## Features

* Microkernel-based architecture using L4/Fiasco
* Custom OpenJDK-based JVM runtime
* Cross-platform support (x86\_64 and ARM)
* Maven-based build system
* QEMU integration for testing and development

## Prerequisites

* Linux-based development environment (Ubuntu 22.04 LTS or later recommended)
* At least 16GB RAM
* 50GB free disk space
* Basic knowledge of OS development concepts
* Familiarity with Java and C++

## Getting Started

1. Clone the repository

   ```bash
   git clone https://github.com/rajames440/StarshipOS.git && cd StarshipOS
   ```

2. Install your toolchain

   ```bash
   sudo apt install git make binutils liburi-perl libgit-repository-perl libxml-parser-perl \
     gcc g++ libc6-dev-i386 libncurses-dev qemu-system xorriso mtools flex bison \
     pkg-config gawk device-tree-compiler dialog wget doxygen graphviz gdb gdb-multiarch \
     gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf autoconf libasound2-dev libcups2-dev \
     libfontconfig1-dev libx11-dev libxext-dev libxrender-dev libxrandr-dev libxtst-dev libxt-dev
   ```

3. Create `~/.m2/settings.xml`

   ```xml
   <settings>
       <pluginGroups>
           <pluginGroup>org.starshipos</pluginGroup>
       </pluginGroups>
   </settings>
   ```

4. Verify that `.starship/starship-dev.properties` exists and includes settings like:

   ```properties
   buildFiasco=true
   buildFiasco.ARM=true
   buildFiasco.x86_64=true
   buildL4=true
   buildL4.ARM=true
   buildL4.x86_64=true
   buildJDK=true
   buildJDK.ARM=true
   buildJDK.x86_64=true
   runQEMU=false
   runQEMU.ARM=false
   runQEMU.x86_64=false
   ```

5. Build the full system (must be run from the **project root**):

   ```bash
   ./mvnw clean install
   ```

   > 🧉 Sit back and wait... Have a coffee & a smoke!

## Project Structure

* `/fiasco` - L4/Fiasco microkernel (built via `starship:build-fiasco`)
* `/l4` - L4 Runtime and ROMFS image generator (via `starship:build-l4`)
* `/openjdk` - StarshipOS JDK, built and packaged into ROMFS
* `/starship-bootstrap` - Early system startup code (launches `OSGiManager`)
* `/starship-jvm-server` - Metadata for the native `jvm_server` loader
* `/starship-l4-deps` - Packs all dependencies needed to build and boot L4
* `/maven-starship-plugin` - Maven plugin providing all build goals

## Build Configuration

Build behavior is controlled by `.starship/starship-dev.properties`.
This file supports toggling build steps and architecture-specific flags:

* `buildFiasco` / `buildFiasco.ARM` / `buildFiasco.x86_64`
* `buildL4` / `buildL4.ARM` / `buildL4.x86_64`
* `buildJDK` / `buildJDK.ARM` / `buildJDK.x86_64`
* `runQEMU` / `runQEMU.ARM` / `runQEMU.x86_64`

## Running in QEMU

1. Set `runQEMU=true` in `.starship/starship-dev.properties`
2. Choose your architecture (`runQEMU.ARM` or `runQEMU.x86_64`)
3. Run the full build from the project root
4. QEMU will launch automatically on success

## Important Notes

* Some modules **require** being built from the project root to properly resolve plugin and parent POM relationships.
* Always run `./mvnw` from the **top-level StarshipOS project directory** unless you know the module is fully
  independent.

## Troubleshooting

### Build Fails with Missing Dependencies

* Ensure all toolchain packages are installed
* Check for correct architecture flags in `.starship/starship-dev.properties`

### QEMU Launch Fails

* Verify QEMU is installed
* Ensure built images exist in `target/` directories

### Maven Plugin Errors

* Check `.m2/settings.xml` for plugin group
* Ensure `maven-plugin-plugin` is version 3.13.1 or later

## Help & Community

* [Project Wiki](https://github.com/rajames440/StarshipOS/wiki)
* [Issue Tracker](https://github.com/rajames440/StarshipOS/issues)
* [Discord Server](https://discord.gg/starshipos)

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for workflow and coding standards.

## License

StarshipOS is licensed under the Apache License 2.0.
See the [LICENSE](LICENSE) file for details.
