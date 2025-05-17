/*
 *  Copyright (c) 2025 R. A.  and contributors..
 *  This file is part of StarshipOS, an experimental operating system.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 */

package org.starship.mojo;

import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;
import org.starship.Architecture;
import org.starship.util.builders.BuildJDKUtil;

import java.io.File;

@Mojo(
        name = "build-jdk",
        defaultPhase = LifecyclePhase.COMPILE,
        aggregator = true,
        requiresProject = false
)
public class BuildJdkMojo extends AbstractStarshipMojo {

    @Override
    protected final void doExecute() {

        File buildDir = new File(projectRoot, "openjdk/build");

        if (buildDir.exists()) {
            getLog().info("[OpenJDK] Skipping build; build/ directory already exists.");
            return;
        }

        if (buildJDK) {
            getLog().warn("**********************************************************************");
            getLog().warn("*                         Building OpenJDK 21                        *");
            getLog().warn("**********************************************************************");
            BuildJDKUtil util = new BuildJDKUtil(this);

            if (buildJDK_x86_64) {
                getLog().warn("**********************************************************************");
                getLog().warn("*                        OpenJDK 21 - x86_64                         *");
                getLog().warn("**********************************************************************");

                try {
                    util.buildJDK(Architecture.X86_64.getClassifier());
                } catch (Exception e) {
                    getLog().error("JDK build failed for x86_64", e);
                    throw new RuntimeException(e.getMessage(), e);
                }
            }

            if (buildJDK_ARM) {
                getLog().warn("**********************************************************************");
                getLog().warn("*                           OpenJDK 21 ARM                           *");
                getLog().warn("**********************************************************************");

                // todo Must figure out a way to build ARM JDK
//                try {
//                    util.buildJDK(Architecture.ARM.getClassifier());
//                } catch (Exception e) {
//                    getLog().error("JDK build failed for ARM", e);
//                    failed = true;
//                }
            }

            getLog().warn("**********************************************************************");
            getLog().warn("*                          OpenJDK 21 DONE                           *");
            getLog().warn("**********************************************************************");
        }
    }
}
