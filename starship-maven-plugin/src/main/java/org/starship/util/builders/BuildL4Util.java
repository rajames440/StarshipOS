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

package org.starship.util.builders;

import org.codehaus.plexus.util.FileUtils;
import org.starship.mojo.AbstractStarshipMojo;
import org.starship.mojo.InitializeMojo;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

public class BuildL4Util {

    private final AbstractStarshipMojo mojo;

    public BuildL4Util(AbstractStarshipMojo mojo) {
        this.mojo = mojo;
    }

    private String getL4BaseDir(AbstractStarshipMojo mojo) {
        return mojo instanceof InitializeMojo ? "StarshipOS/l4" : "l4";
    }

    public final void buildL4Re(String architecture) {
        try {
            File l4Dir = new File(getL4BaseDir(mojo));
            validateDirectory(l4Dir);

            log(architecture, "Setting up build directory...");
            setupBuild(l4Dir, architecture);

            log(architecture, "Copying prebuilt config...");
            copyPrebuiltConfig(l4Dir, architecture);

            log(architecture, "Running olddefconfig...");
            runOldDefconfig(l4Dir, architecture);

            log(architecture, "Starting build...");
            buildL4Re(l4Dir, architecture);

            log(architecture, "Building ISO image...");
            buildIsoImage(architecture, "jvm_server.cfg", new File(mojo.projectRoot, "fiasco/build/" + architecture));


            log(architecture, "Build completed successfully.");
        } catch (Exception e) {
            throw new IllegalStateException("L4Re build for architecture '" + architecture + "' failed: " + e.getMessage(), e);
        }
    }

    public final void buildIsoImage(String architecture, String configName, File kernelDir) {
        try {
            File l4BuildDir = new File(getL4BaseDir(mojo), "build/" + architecture);
            File l4SrcDir = new File(getL4BaseDir(mojo));

            validateDirectory(l4BuildDir);
            validateDirectory(kernelDir);
            validateDirectory(l4SrcDir);

            log(architecture, "Creating ISO image...");
            ProcessBuilder pb = new ProcessBuilder(
                    "make",
                    "E=" + configName,
                    "isoimage",
                    "MODULE_SEARCH_PATH=" + new File(mojo.projectRoot, "starship-l4-deps/target/l4-deps").getAbsolutePath() + ":" +
                            kernelDir.getAbsolutePath() + ":" +
                            new File(l4SrcDir, "conf/examples").getAbsolutePath()
            );
            pb.directory(l4BuildDir);
            pb.inheritIO();
            int exit = pb.start().waitFor();
            if (exit != 0) {
                throw new IllegalStateException("ISO image creation failed with exit code " + exit);
            }
        } catch (Exception e) {
            throw new IllegalStateException("ISO creation failed for architecture '" + architecture + "': " + e.getMessage(), e);
        }
    }

    private void setupBuild(File dir, String architecture) throws Exception {
        ProcessBuilder pb = new ProcessBuilder("make", "B=build/" + architecture);
        pb.directory(dir);
        pb.inheritIO();
        int exit = pb.start().waitFor();
        if (exit != 0) {
            throw new Exception("Setup build failed with exit code " + exit);
        }
    }

    private void buildL4Re(File dir, String architecture) throws Exception {
        File buildDir = new File(dir, "build/" + architecture);
        ProcessBuilder pb = new ProcessBuilder("make", "O=.");
        pb.directory(buildDir);
        pb.inheritIO();
        int exit = pb.start().waitFor();
        if (exit != 0) {
            throw new Exception("make failed for L4Re in build/" + architecture);
        }
    }

    private void runOldDefconfig(File dir, String arch) throws Exception {
        File buildDir = new File(dir, "build/" + arch);
        ProcessBuilder pb = new ProcessBuilder("make", "olddefconfig");
        pb.directory(buildDir);
        pb.inheritIO();
        int exit = pb.start().waitFor();
        if (exit != 0) {
            throw new Exception("make olddefconfig failed.");
        }
    }

    private void copyPrebuiltConfig(File objDir, String arch) throws IOException {
        String resourceName = "l4.config." + arch;
        try (InputStream in = getClass().getClassLoader().getResourceAsStream(resourceName)) {
            if (in == null) throw new IOException("Missing config: " + resourceName);
            File buildConfig = new File(objDir, "build/" + arch + "/.config");
            FileUtils.copyStreamToFile(() -> in, buildConfig);
        }
    }

    private void validateDirectory(File dir) throws IOException {
        if (!dir.exists() || !dir.isDirectory()) {
            throw new IOException("Directory invalid: " + dir.getAbsolutePath());
        }
    }

    private void log(String arch, String message) {
        System.out.println("[L4Re:" + arch + "] " + message);
    }
}
