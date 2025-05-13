# StarshipOS #

StarshipOS is an experimental operating system that combines the robustness of L4/Fiasco microkernel with a custom JVM
implementation. This project aims to create a secure, performant, and modular operating system for both x86_64 and ARM
architectures.

## Features ##

- Microkernel-based architecture using L4/Fiasco
- Custom OpenJDK-based JVM implementation
- Cross-platform support (x86_64 and ARM)
- Maven-based build system
- QEMU integration for testing and development

## Prerequisites ##

- Linux-based development environment (Ubuntu 22.04 LTS or later recommended)
- At least 16GB RAM
- 50GB free disk space
- Basic knowledge of OS development concepts
- Familiarity with Java and C++

## Getting Started ##

1. Clone the repository
> `git clone https://github.com/rajames440/StarshipOS.git && cd StarshipOS`

2. Install your toolchain

>
`sudo apt install git make binutils liburi-perl libgit-repository-perl libxml-parser-perl gcc g++ libc6-dev-i386 libncurses-dev qemu-system xorriso mtools flex bison pkg-config gawk device-tree-compiler dialog wget doxygen graphviz gdb gdb-multiarch gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf autoconf libasound2-dev libcups2-dev libfontconfig1-dev libx11-dev libxext-dev libxrender-dev libxrandr-dev libxtst-dev libxt-dev`

3. Create `~/.m2/settings.xml`
> ```xml
> <settings>
>     <pluginGroups>
>         <pluginGroup>org.starshipos</pluginGroup>
>     </pluginGroups>
> </settings>
> ```

4. Verify that `.starship/starship-dev.properties` has this content:
> ```properties
> #
> #  Copyright (c) 2025 R. A.  and contributors..
> #  This file is part of StarshipOS, an experimental operating system.
> #
> #  Licensed under the Apache License, Version 2.0 (the "License");
> #  you may not use this file except in compliance with the License.
> #  You may obtain a copy of the License at
> #
> #        https://www.apache.org/licenses/LICENSE-2.0
> #
> # Unless required by applicable law or agreed to in writing,
> # software distributed under the License is distributed on an
> # "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
> # either express or implied. See the License for the specific
> # language governing permissions and limitations under the License.
> #
> #
> 
> #Starship Development Updated Properties
> #Thu May 08 15:26:42 EDT 2025
> buildFiasco=true
> buildFiasco.ARM=true
> buildFiasco.x86_64=true
> buildL4=true
> buildL4.ARM=true
> buildL4.x86_64=true
> buildJDK=true
> buildJDK.ARM=true
> buildJDK.x86_64=true
> installCodebase=false
> installToolchain=false
> runQEMU=false
> runQEMU.ARM=false
> runQEMU.x86_64=false
> ```

5. From the project root:
> `./mvnw clean install`
> ### Sit back and wait...
> Have a coffee & a smoke!

## Project Structure ##

- `/fiasco` - L4/Fiasco microkernel implementation
- `/l4` - L4 Runtime Environment
- `/maven-starship-plugin` - Custom Maven plugin for build automation
- `/openjdk` - Modified OpenJDK for StarshipOS
- `/starship-bootstrap` - System bootstrap components
- `/starship-jvm-server` - JVM server implementation
- `/starship-romfs` - Read-only filesystem components

## Build Configuration ##

The build process can be customized through `.starship/starship-dev.properties`. Key options include:

- `buildFiasco` - Build the Fiasco microkernel
- `buildL4` - Build the L4 runtime
- `buildJDK` - Build the custom JDK
- `runQEMU` - Launch QEMU after successful build

Each option has architecture-specific variants (`.ARM` and `.x86_64`).

## Running in QEMU ##

To test StarshipOS in QEMU:

1. Set `runQEMU=true` in your properties file
2. Choose your architecture (`runQEMU.ARM` or `runQEMU.x86_64`)
3. Run the build process
4. QEMU will launch automatically upon successful build

## Troubleshooting ##

### Common Issues ###

1. **Build Fails with Missing Dependencies**
    - Verify all required packages are installed
    - Check system architecture compatibility

2. **QEMU Launch Fails**
    - Ensure QEMU is properly installed
    - Verify built images exist in target directory

3. **Maven Plugin Issues**
    - Check `settings.xml` configuration
    - Verify plugin version compatibility

### Getting Help ###

- Check our [Wiki](https://github.com/rajames440/StarshipOS/wiki)
- Submit issues on our [Issue Tracker](https://github.com/rajames440/StarshipOS/issues)
- Join our [Discord Community](https://discord.gg/starshipos)

## Contributing ##

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

## License ##

StarshipOS is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.