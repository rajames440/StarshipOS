/*
 * Copyright (c) 2025 R. A.  and contributors..
 * /This file is part of StarshipOS, an experimental operating system.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       https://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 *  either express or implied. See the License for the specific
 *  language governing permissions and limitations under the License.
 *
 */

package org.starship.mojo;

import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;
import org.starship.util.builders.BuildFiascoUtil;
import org.starship.util.builders.BuildJDKUtil;
import org.starship.util.builders.BuildL4Util;

@Mojo(name = "build-core", defaultPhase = LifecyclePhase.COMPILE)
public class BuildMojo extends AbstractStarshipMojo {

    @Override
    public void doExecute() {
        System.out.println("BuildMojo.doExecute");
        try {
            if (buildFiasco) {
                getLog().info("*******************************************************");
                getLog().info("  Building Fiasco");
                getLog().info("*******************************************************");
                BuildFiascoUtil util = new BuildFiascoUtil();
                if (buildFiasco_x86_64) util.buildFiasco("x86_64");
                if (buildFiasco_ARM) util.buildFiasco("arm");
            }

            if (buildL4) {
                getLog().info("*******************************************************");
                getLog().info("  Building L4");
                getLog().info("*******************************************************");
                BuildL4Util util = new BuildL4Util();
                if (buildL4_x86_64) util.buildL4("x86_64");
                if (buildL4_ARM) util.buildL4("arm");
            }

            if (buildJDK) {
                getLog().info("*******************************************************");
                getLog().info("  Building OpenJDK jdk-21-ga");
                getLog().info("*******************************************************");
                BuildJDKUtil util = new BuildJDKUtil();
                if (buildJDK_x86_64) util.buildJDK("x86_64");
                if (buildJDK_ARM) util.buildJDK("arm");
            }
        } catch (Exception e) {
            getLog().error("Failed to build StarshipOS: " + e.getMessage());
            throw new RuntimeException(e);
        }


//        if (runQEMU) {
//            getLog().info("*******************************************************");
//            getLog().info("  Running Hello World Demo in QEMU");
//            getLog().info("*******************************************************");
//            RunHelloQemuUtil util = new RunHelloQemuUtil();
//            if (runQEMU_x86_64) util.runHelloDemo("x86_64");
//            if (runQEMU_ARM) util.runHelloDemo("arm");
//        }

    }
}
