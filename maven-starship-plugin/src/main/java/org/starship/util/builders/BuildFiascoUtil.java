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

package org.starship.util.builders;

import org.codehaus.plexus.util.FileUtils;
import org.starship.mojo.AbstractStarshipMojo;
import org.starship.mojo.InitializeMojo;
import org.starship.util.AbstractUtil;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;


/**
 * Utility class for building the Fiasco microkernel and L4Re userland components.
 * Handles the build process including configuration management and compilation
 * for different architectures.
 */
public class BuildFiascoUtil extends AbstractUtil {

    private static final String FIASCO_SRC_DIR = "src";

    /**
     * Constructs a new BuildFiascoUtil instance.
     *
     * @param mojo Maven calling mojo.
     */
    public BuildFiascoUtil(AbstractStarshipMojo mojo) {
        super(mojo);
    }

    /**
     * Builds the Fiasco microkernel for the specified architecture.
     * This includes compiling the kernel, copying architecture-specific configurations,
     * and building the L4Re userland components.
     *
     * @param architecture target architecture for the build
     * @throws IllegalStateException if the build process fails
     */
    public void buildFiasco(String architecture) {
        try {
            String fiascoBaseDir = getFiascoBaseDir((AbstractStarshipMojo) mojo); // Determine the base directory dynamically
            File fiascoDir = new File(fiascoBaseDir);
            File srcDir = new File(fiascoDir, FIASCO_SRC_DIR);
            File objDir = new File(fiascoDir, "target/" + architecture);

            validateDirectory(fiascoDir, "Fiasco Base");
            validateDirectory(srcDir, "Fiasco Source");

            runMakeCommand(fiascoDir, architecture);
            copyPrebuiltConfig(objDir, architecture);
            runOldConfig(objDir);
            buildFiascoUserland(objDir);

        } catch (Exception e) {
            throw new IllegalStateException("Fiasco build for architecture '" + architecture + "' failed: " + e.getMessage(), e);
        }
    }

    private String getFiascoBaseDir(AbstractStarshipMojo mojo) {
        if (mojo instanceof InitializeMojo) {
            return "StarshipOS/fiasco"; // Context for InitializeMojo
        } else {
            return "fiasco"; // Context for BuildCoreMojo
        }
    }

    /**
     * Executes the make command to build the Fiasco kernel.
     *
     * @param baseDir      base directory for the build
     * @param architecture target architecture
     * @throws Exception if the make command fails
     */
    private void runMakeCommand(File baseDir, String architecture) throws Exception {
        ProcessBuilder builder = new ProcessBuilder("make", "B=target/" + architecture)
                .directory(baseDir)
                .inheritIO();

        Process process = builder.start();
        int exitCode = process.waitFor();

        if (exitCode != 0) {
            throw new Exception("Building Fiasco kernel failed with exit code: " + exitCode);
        }
    }

    /**
     * Copies the prebuilt configuration file for the specified architecture.
     *
     * @param objDir target object directory
     * @param arch   target architecture
     * @throws IOException if copying the configuration fails
     */
    private void copyPrebuiltConfig(File objDir, String arch) throws IOException {
        String resourceName = "fiasco.globalconfig." + arch;
        try (InputStream in = getClass().getClassLoader().getResourceAsStream(resourceName)) {
            //                throw new IOException("Resource not found: " + resourceName);
            File outFile = new File(objDir, "globalconfig.out");
            FileUtils.copyStreamToFile(() -> in, outFile);
        } catch (IOException e) {
            throw new IOException("Failed to copy config for architecture '" + arch + "': " + e.getMessage(), e);
        }
    }

    /**
     * Runs the olddefconfig make target to update the configuration.
     *
     * @param objDir object directory containing the configuration
     * @throws Exception if the configuration update fails
     */
    private void runOldConfig(File objDir) throws Exception {
        ProcessBuilder builder = new ProcessBuilder("make", "olddefconfig")
                .directory(objDir)
                .inheritIO();

        Process process = builder.start();
        int exitCode = process.waitFor();

        if (exitCode != 0) {
            throw new Exception("make olddefconfig failed with exit code: " + exitCode);
        }
    }

    /**
     * Builds the L4Re userland components using parallel compilation.
     *
     * @param objDir object directory for the build
     * @throws Exception if the userland build fails
     */
    private void buildFiascoUserland(File objDir) throws Exception {
        ProcessBuilder builder = new ProcessBuilder("make", "-j" + Runtime.getRuntime().availableProcessors())
                .directory(objDir)
                .inheritIO();

        Process process = builder.start();
        int exitCode = process.waitFor();

        if (exitCode != 0) {
            throw new Exception("Building Fiasco.OC failed in " + objDir.getAbsolutePath() + " with exit code: " + exitCode);
        }
    }

//    /**
//     * Gets the absolute path to the Fiasco base directory.
//     *
//     * @return File object representing the Fiasco base directory
//     */
//    private File getAbsolutePath() {
//        return new File(System.getProperty("user.dir"), getFiascoBaseDir(mojo));
//    }

    /**
     * Validates that a directory exists and is actually a directory.
     *
     * @param dir         directory to validate
     * @param description descriptive name of the directory for error messages
     * @throws IOException if the directory is invalid or doesn't exist
     */
    private void validateDirectory(File dir, String description) throws IOException {
        if (!dir.exists() || !dir.isDirectory()) {
            throw new IOException(description + " directory does not exist or is invalid: " + dir.getAbsolutePath());
        }
    }
}
