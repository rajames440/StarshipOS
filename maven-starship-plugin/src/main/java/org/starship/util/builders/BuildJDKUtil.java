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

import org.starship.mojo.AbstractStarshipMojo;
import org.starship.mojo.InitializeMojo;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.attribute.PosixFilePermission;
import java.util.HashSet;
import java.util.Set;

/**
 * Utility class for building OpenJDK from source code.
 * This class provides functionality to configure and build OpenJDK
 * for different architectures within the StarshipOS project.
 */
@SuppressWarnings({"HardcodedFileSeparator", "Annotation", "AccessOfSystemProperties", "UseOfProcessBuilder"})
public class BuildJDKUtil {

    private final AbstractStarshipMojo mojo;

    public BuildJDKUtil(AbstractStarshipMojo mojo) {
        this.mojo = mojo;
    }

    private String getJdkBaseDir(AbstractStarshipMojo mojo) {
        if (mojo instanceof InitializeMojo) {
            return "StarshipOS/openjdk"; // Context for InitializeMojo
        } else {
            return "openjdk"; // Context for BuildCoreMojo
        }
    }

    /**
     * Builds OpenJDK for the specified architecture by executing the configure,
     * clean, and build steps sequentially.
     *
     * @param arch the target architecture (e.g., "arm" or "x86_64")
     * @throws Exception if the configuration or build process fails
     */
    public final void buildJDK(String arch) throws Exception {
        configure(arch);
        clean();
        build(arch);
    }

    /**
     * Configures the OpenJDK build environment for the specified architecture.
     *
     * @param architecture the target architecture (e.g., "arm" or "x86_64")
     * @throws Exception if the configuration process fails
     */
    public final void configure(String architecture) throws Exception {
        File jdkSrcDir = new File(System.getProperty("user.dir"), getJdkBaseDir(mojo));
        if (!jdkSrcDir.exists()) {
            throw new Exception("JDK source directory does not exist: " + jdkSrcDir.getAbsolutePath());
        }

        File configureScript = new File(jdkSrcDir, "configure");
        Path configurePath = configureScript.toPath();
        Set<PosixFilePermission> perms = new HashSet<>();
        perms.add(PosixFilePermission.OWNER_READ);
        perms.add(PosixFilePermission.OWNER_WRITE);
        perms.add(PosixFilePermission.OWNER_EXECUTE);
        perms.add(PosixFilePermission.GROUP_READ);
        perms.add(PosixFilePermission.GROUP_EXECUTE);
        perms.add(PosixFilePermission.OTHERS_READ);
        perms.add(PosixFilePermission.OTHERS_EXECUTE);
        Files.setPosixFilePermissions(configurePath, perms);

        String targetTriplet = architecture.equals("arm") ? "arm-linux-gnueabihf" : "x86_64-linux-gnu";

        ProcessBuilder configureBuilder = new ProcessBuilder(
                "bash", "-c", "./configure " +
                "--openjdk-target=" + targetTriplet + " " +
                "--with-debug-level=fastdebug " +    // Changed from release to fastdebug
                "--enable-option-checking=fatal " +
                "--with-native-debug-symbols=internal " +  // Changed from none to internal
                "--with-jvm-variants=minimal " +     // Changed from server to minimal
                "--with-version-pre=starship " +
                "--with-version-build=1 " +
                "--with-version-opt=reloc " +
                "--with-toolchain-type=gcc " +
                "--disable-warnings-as-errors"
        ).directory(jdkSrcDir).inheritIO();

        Process process = configureBuilder.start();
        if (process.waitFor() != 0) {
            throw new Exception("OpenJDK configure failed for: " + architecture);
        }
    }

    /**
     * Cleans the build using `make clean`.
     *
     * @throws Exception if the clean process fails
     */
    public final void clean() throws Exception {
        File jdkSrcDir = new File(System.getProperty("user.dir"), getJdkBaseDir(mojo));

        ProcessBuilder cleanBuilder = new ProcessBuilder("bash", "-c", "make clean")
                .directory(jdkSrcDir).inheritIO();

        Process process = cleanBuilder.start();
        if (process.waitFor() != 0) {
            throw new Exception("OpenJDK clean failed.");
        }
    }

    /**
     * Builds OpenJDK for the specified architecture using `make images`.
     * Verifies the standard output directory for correctness.
     *
     * @param architecture the target architecture (e.g., "arm" or "x86_64")
     * @throws Exception if the build process fails or output image is missing
     */
    public final void build(String architecture) throws Exception {
        File jdkSrcDir = new File(System.getProperty("user.dir"), getJdkBaseDir(mojo));
        String outputDirName = "linux-" + architecture + "-server-release";
        File imageDir = new File(jdkSrcDir, "build/" + outputDirName + "/images/jdk");

        ProcessBuilder buildBuilder = new ProcessBuilder("bash", "-c", "make images")
                .directory(jdkSrcDir).inheritIO();

        Process process = buildBuilder.start();
        if (process.waitFor() != 0) {
            throw new Exception("OpenJDK build failed for: " + architecture);
        }

        if (!imageDir.exists()) {
            throw new Exception("Expected JDK image not found at: " + imageDir.getAbsolutePath());
        }

        // Artifact handling is done downstream (e.g., in starship-romfs)
    }
}
