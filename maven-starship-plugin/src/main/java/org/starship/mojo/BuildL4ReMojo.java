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
import org.starship.util.builders.BuildL4Util;

@Mojo(
        name = "build-l4",
        defaultPhase = LifecyclePhase.COMPILE,
        aggregator = true,
        requiresProject = false
)
public class BuildL4ReMojo extends AbstractStarshipMojo {

    @Override
    protected final void doExecute() {
        getLog().warn("**********************************************************************");
        getLog().warn("*                           Building L4Re                            *");
        getLog().warn("**********************************************************************");
        boolean failed = false;

        if (buildL4) {
            BuildL4Util util = new BuildL4Util(this);

            if (buildL4_x86_64) {
                getLog().warn("**********************************************************************");
                getLog().warn("*                            L4Re x86_64                             *");
                getLog().warn("**********************************************************************");
                if (buildJDK_ARM || buildFiasco_ARM || buildL4_ARM) {
                    getLog().warn("[StarshipOS] ARM builds are currently disabled. See TODOs for re-enablement.");
                }

                try {
                    util.buildL4Re("x86_64");
                } catch (Exception e) {
                    getLog().error("L4Re build failed for x86_64", e);
                    failed = true;
                }
            }

            if (buildL4_ARM) {
                getLog().warn("**********************************************************************");
                getLog().warn("*                             L4Re ARM                               *");
                getLog().warn("**********************************************************************");
                // todo Defer ARM until we can cross compile JDK for ARM
//                try {
//                    util.buildL4Re("arm");
//                } catch (Exception e) {
//                    getLog().error("L4Re build failed for ARM", e);
//                    failed = true;
//                }
            }

            if (failed) {
                setCleanFlag("cleanL4");
                getLog().warn("One or more L4Re builds failed. cleanL4=true.");
            } else {
                getLog().info("Built L4Re successfully.");
            }
        }
    }
}
