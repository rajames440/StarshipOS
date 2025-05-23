#!/bin/bash

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

# build-fiasco.sh

###########################################################################################################
#  IMPORTANT! ANYTHING that needs to be in romfs must be built BEFORE starship-l4-deps and l4. IMPORTANT! #
#  IMPORTANT! ANYTHING that needs to be in romfs must be built BEFORE starship-l4-deps and l4. IMPORTANT! #
#  IMPORTANT! ANYTHING that needs to be in romfs must be built BEFORE starship-l4-deps and l4. IMPORTANT! #
###########################################################################################################
# See the root pom.

echo "[Fiasco.OC] mvn clean install"
rm -rf ./fiasco/build
./mvnw clean install 2>&1 | tee build.log


