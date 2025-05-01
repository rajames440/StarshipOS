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

package org.starship.util.misc;

import java.io.File;


/**
 * Utility class for running Hello demo applications in QEMU emulator with Fiasco.OC microkernel
 * and L4Re runtime environment. This class provides functionality to execute and test
 * Hello configuration demos in a virtualized environment.
 */
public class RunHelloQemuUtil {

    private static final String BASE_FIASCO_DIR = "StarshipOS/fiasco";
    private static final String BASE_L4RE_DIR = "StarshipOS/l4";

    /**
     * Runs a Hello demo configuration in QEMU for the specified architecture.
     * This method sets up the necessary kernel and L4Re directories, configures
     * the module search path, and executes the demo in QEMU.
     *
     * @param arch the target architecture for running the demo (e.g., "x86", "arm")
     * @throws Exception if the QEMU process fails to execute or returns a non-zero exit code
     */
    public void runHelloDemo(String arch) throws Exception {
        File kernelObjDir = new File(System.getProperty("user.dir"), BASE_FIASCO_DIR + "/target/" + arch);
        File l4reSrcDir = new File(System.getProperty("user.dir"), BASE_L4RE_DIR);
        File l4reObjDir = new File(System.getProperty("user.dir"), BASE_L4RE_DIR + "/target/" + arch);

        String modulePath = kernelObjDir.getAbsolutePath() + ":" + new File(l4reSrcDir, "conf/examples").getAbsolutePath();

        ProcessBuilder builder = new ProcessBuilder(
                "make",
                "E=hello-cfg",
                "qemu",
                "MODULE_SEARCH_PATH=" + modulePath
        ).directory(l4reObjDir)
                .inheritIO();

        Process process = builder.start();
        int exit = process.waitFor();

        if (exit != 0) {
            throw new Exception("Running hello-cfg in QEMU for " + arch + " failed with exit code: " + exit);
        }
    }
}
