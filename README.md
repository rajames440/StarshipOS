# StarshipOS #
## Getting Started ##

1. Clone the repository

> `git clone https://github.com/starshipos/starship.githttps://github.com/rajames440/StarshipOS.git && cd StarshipOS`

2. Install your toolchain
> `sudo apt install git make binutils liburi-perl libgit-repository-perl libxml-parser-perl gcc g++ libc6-dev-i386 libncurses-dev qemu-system xorriso mtools flex bison pkg-config gawk device-tree-compiler dialog wget doxygen graphviz gdb gdb-multiarch gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf`

3. Create `~/.m2/settings.xml`
> ```xml
> <settings>
>     <pluginGroups>
>         <pluginGroup>org.starshipos</pluginGroup>
>     </pluginGroups>
> </settings>
> ```

4. Verify that `.starship/starship-dev.properties` has this content:

```
#
#  Copyright (c) 2025 R. A.  and contributors..
#  This file is part of StarshipOS, an experimental operating system.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#        https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
#

#Starship Development Updated Properties
#Thu May 08 15:26:42 EDT 2025
buildFiasco=true
buildFiasco.ARM=true
buildFiasco.x86_64=true
buildL4=true
buildL4.ARM=true
buildL4.x86_64=true
buildJDK=true
buildJDK.ARM=true
buildJDK.x86_64=true
installCodebase=false
installToolchain=false
runQEMU=false
runQEMU.ARM=false
runQEMU.x86_64=false
```

4. From the project root:

> `./mvnw clean install`
> ### Sit back and wait...
> Have a coffee & a smoke!
