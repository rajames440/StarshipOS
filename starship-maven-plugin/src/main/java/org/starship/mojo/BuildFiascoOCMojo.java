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
import org.starship.util.builders.BuildFiascoUtil;

import java.io.File;

@Mojo(
        name = "build-fiasco",
        defaultPhase = LifecyclePhase.COMPILE,
        aggregator = true,
        requiresProject = false
)
public class BuildFiascoOCMojo extends AbstractStarshipMojo {

    @Override
    protected final void doExecute() {

        File buildDir = new File(projectRoot, "fiasco/build");

        if (buildDir.exists()) {
            getLog().info("[Fiasco] Skipping build; build/ directory already exists.");
            return;
        }

        getLog().info("[Fiasco] build/ directory not found. Starting Fiasco build...");
        getLog().info("[Fiasco] build/ directory not found. Starting Fiasco build...");

        getLog().info("[Fiasco] build/ directory missing. Proceeding with rebuild.");
        getLog().warn("**********************************************************************");
        getLog().warn("*                         Building Fiasco.OC                         *");
        getLog().warn("**********************************************************************");

        if (buildFiasco) {
            BuildFiascoUtil util = new BuildFiascoUtil(this);

            if (buildFiasco_x86_64) {
                getLog().warn("**********************************************************************");
                getLog().warn("*                          Fiasco.OC x86_64                          *");
                getLog().warn("**********************************************************************");
                try {
                    util.buildFiasco(Architecture.X86_64.getClassifier());
                } catch (Exception e) {
                    getLog().error("FiascoOC build failed for x86_64", e);
                    throw new RuntimeException(e.getMessage(), e);
                }
            }

            if (buildFiasco_ARM) {
                getLog().warn("**********************************************************************");
                getLog().warn("*                           Fiasco.OC ARM                            *");
                getLog().warn("**********************************************************************");
                // todo Defer ARM until we can cross compile JDK for ARM
//                try {
//                    util.buildFiasco(Architecture.ARM.getClassifier());
//                } catch (Exception e) {
//                    getLog().error("FiascoOC build failed for ARM", e);
//                    failed = true;
//                }
            }

            getLog().warn("**********************************************************************");
            getLog().warn("*                          Fiasco.OC DONE                            *");
            getLog().warn("**********************************************************************");
        }
    }
}
