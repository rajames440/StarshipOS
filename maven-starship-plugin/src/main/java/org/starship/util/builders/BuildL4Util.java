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


/**
 * Utility class for building L4Re operating system components.
 * Handles the build process including configuration setup and compilation
 * for different architectures.
 */
public class BuildL4Util {

//    private static final String L4_SRC_DIR = "src";
    private final AbstractStarshipMojo mojo;

    /**
     * Constructs a new BuildL4Util instance.
     *
     * @param mojo Maven calling mojo.
     */
    public BuildL4Util(AbstractStarshipMojo mojo) {
        this.mojo = mojo;
    }

    private String getL4BaseDir(AbstractStarshipMojo mojo) {
        if (mojo instanceof InitializeMojo) {
            return "StarshipOS/l4"; // Context for InitializeMojo
        } else {
            return "l4"; // Context for BuildCoreMojo
        }
    }

    /**
     * Builds L4Re for the specified architecture.
     * This method handles the complete build process, including directory validation,
     * build setup, configuration copying, and final compilation.
     *
     * @param architecture the target architecture for the build
     * @throws IllegalStateException if the build process fails
     */
    public final void buildL4Re(String architecture) {
        try {
            String fiascoBaseDir = getL4BaseDir(mojo); // Determine the base directory dynamically
            File l4Dir = new File(fiascoBaseDir);
            File objDir = new File(l4Dir, "build/" + architecture);

            validateDirectory(l4Dir);

            setupBuild(l4Dir, architecture);
            copyPrebuiltConfig(l4Dir, architecture);
            buildL4Re(objDir, architecture);
            copyFiascoKernel(architecture);
        } catch (Exception e) {
            throw new IllegalStateException("L4Re build for architecture '" + architecture + "' failed: " + e.getMessage(), e);
        }
    }

    /**
     * Executes the L4Re build process in the specified directory.
     *
     * @param baseDir      the base directory for the build
     * @param architecture the target architecture
     * @throws Exception if the build process fails
     */
    private void buildL4Re(File baseDir, String architecture) throws Exception {
        ProcessBuilder builder = new ProcessBuilder("make")
                .directory(baseDir)
                .inheritIO();

        Process process = builder.start();
        int exitCode = process.waitFor();

        if (exitCode != 0) {
            throw new Exception("make B=build/" + architecture + " failed with exit code: " + exitCode);
        }
    }

    /**
     * Copies the prebuilt configuration file for the specified architecture.
     *
     * @param objDir the target directory for the configuration file
     * @param arch   the target architecture
     * @throws IOException if the configuration file cannot be copied
     */
    private void copyPrebuiltConfig(File objDir, String arch) throws IOException {
        String resourceName = "l4.config." + arch;
        try (InputStream in = getClass().getClassLoader().getResourceAsStream(resourceName)) {
            if (in == null) {
                throw new IOException("Resource not found: " + resourceName);
            }
            File outFile = new File(objDir, ".config");
            FileUtils.copyStreamToFile(() -> in, outFile);
        } catch (IOException e) {
            throw new IOException("Failed to copy config for architecture '" + arch + "': " + e.getMessage(), e);
        }
    }

    /**
     * Validates that the specified directory exists and is valid.
     *
     * @param dir the directory to validate
     * @throws IOException if the directory doesn't exist or is invalid
     */
    private void validateDirectory(File dir) throws IOException {
        if (!dir.exists() || !dir.isDirectory()) {
            throw new IOException("L4Re Base" + " directory does not exist or is invalid: " + dir.getAbsolutePath());
        }
    }

    /**
     * Sets up the build environment for the specified architecture.
     *
     * @param objDir       the build directory
     * @param architecture the target architecture
     * @throws Exception if the build setup fails
     */
    private void setupBuild(File objDir, String architecture) throws Exception {
        ProcessBuilder builder = new ProcessBuilder("make", "B=build/" + architecture)
                .directory(objDir)
                .inheritIO();

        Process process = builder.start();
        int exitCode = process.waitFor();

        if (exitCode != 0) {
            throw new Exception("make olddefconfig failed with exit code: " + exitCode);
        }
    }

    /**
     * Copies the compiled Fiasco kernel from the build directory to the L4Re system directory.
     * This method ensures the Fiasco kernel is properly integrated into the L4Re system structure.
     *
     * @param arch the target architecture for which the kernel was built
     * @throws IOException if the source kernel file doesn't exist or if the copy operation fails
     */
    private void copyFiascoKernel(String arch) throws IOException {
        File source = new File(
                new File(new File(new File(mojo.baseDir, "fiasco"), "build"), arch),
                "fiasco"
        );

        File target = new File(
                new File(
                        new File(
                                new File(
                                        new File(new File(mojo.baseDir, "l4"), "build"),
                                        arch
                                ),
                                "bin"
                        ),
                        "amd64_gen"
                ),
                new File("l4f", "fiasco").getPath()
        );

        if (!source.exists()) {
            throw new IOException("Fiasco kernel not found at: " + source.getAbsolutePath());
        }

        if (!target.getParentFile().exists() && !target.getParentFile().mkdirs()) {
            throw new IOException("Failed to create target directory: " + target.getParent());
        }

        FileUtils.copyFile(source, target);
        mojo.getLog().info("[BuildL4Util] Copied Fiasco kernel to: " + target.getAbsolutePath());
    }

}
