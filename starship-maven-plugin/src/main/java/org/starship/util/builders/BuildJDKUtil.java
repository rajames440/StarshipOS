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

import org.starship.Architecture;
import org.starship.mojo.AbstractStarshipMojo;
import org.starship.mojo.InitializeMojo;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.attribute.PosixFilePermission;
import java.util.Set;

@SuppressWarnings("AccessOfSystemProperties")
public class BuildJDKUtil {

    private final AbstractStarshipMojo mojo;

    public BuildJDKUtil(AbstractStarshipMojo mojo) {
        this.mojo = mojo;
    }

    private String getJdkBaseDir(AbstractStarshipMojo mojo) {
        return mojo instanceof InitializeMojo ? "StarshipOS/openjdk" : "openjdk";
    }

    public final void buildJDK(String arch) throws Exception {
        configure(arch);
        clean();
        build(arch);
    }

    public final void configure(String arch) throws Exception {
        File jdkSrcDir = new File(System.getProperty("user.dir"), getJdkBaseDir(mojo));
        if (!jdkSrcDir.exists()) throw new Exception("JDK source dir missing: " + jdkSrcDir);

        File configureScript = new File(jdkSrcDir, "configure");
        Files.setPosixFilePermissions(configureScript.toPath(), Set.of(
                PosixFilePermission.OWNER_READ, PosixFilePermission.OWNER_WRITE, PosixFilePermission.OWNER_EXECUTE,
                PosixFilePermission.GROUP_READ, PosixFilePermission.GROUP_EXECUTE,
                PosixFilePermission.OTHERS_READ, PosixFilePermission.OTHERS_EXECUTE
        ));

        String triplet = Architecture.ARM.getClassifier().equals(arch) ? "arm-linux-gnueabihf" : "x86_64-linux-gnu";

        ProcessBuilder builder = new ProcessBuilder(
                "bash", "-c", "./configure " +
                "--openjdk-target=" + triplet + " " +
                "--with-debug-level=fastdebug " +
                "--enable-option-checking=fatal " +
                "--with-native-debug-symbols=internal " +
                "--with-jvm-variants=client " +
                "--with-version-pre=starship " +
                "--with-version-build=1 " +
                "--with-version-opt=reloc " +
                "--with-toolchain-type=gcc " +
                "--with-target-bits=64 " +
                "--disable-warnings-as-errors"
        ).directory(jdkSrcDir).inheritIO();

        if (builder.start().waitFor() != 0) throw new Exception("OpenJDK configure failed.");
    }

    public final void clean() throws Exception {
        File jdkSrcDir = new File(System.getProperty("user.dir"), getJdkBaseDir(mojo));
        if (new ProcessBuilder("bash", "-c", "make clean").directory(jdkSrcDir).inheritIO().start().waitFor() != 0) {
            throw new Exception("OpenJDK clean failed.");
        }
    }

    public final void build(String arch) throws Exception {
        File jdkSrcDir = new File(System.getProperty("user.dir"), getJdkBaseDir(mojo));
        File imageDir = new File(jdkSrcDir, "build/linux-" + arch + "-server-release/images/jdk");

        if (new ProcessBuilder("bash", "-c", "make images").directory(jdkSrcDir).inheritIO().start().waitFor() != 0) {
            throw new Exception("OpenJDK build failed.");
        }
    }
}
