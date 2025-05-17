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
import org.starship.Architecture;
import org.starship.mojo.AbstractStarshipMojo;
import org.starship.mojo.InitializeMojo;
import org.starship.util.AbstractUtil;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

public class BuildFiascoUtil extends AbstractUtil {

    private static final String FIASCO_SRC_DIR = "src";

    public BuildFiascoUtil(AbstractStarshipMojo mojo) {
        super(mojo);
    }

    public final void buildFiasco(String architecture) {
        try {
            String fiascoBaseDir = getFiascoBaseDir((AbstractStarshipMojo) getMojo());
            File fiascoDir = new File(fiascoBaseDir);
            File srcDir = new File(fiascoDir, FIASCO_SRC_DIR);
            File objDir = new File(fiascoDir, "build/" + architecture);

            validateDirectory(fiascoDir, "Fiasco Base");
            validateDirectory(srcDir, "Fiasco Source");

            runMakeCommand(fiascoDir, architecture);
            copyPrebuiltConfig(objDir, Architecture.from(architecture).getClassifier());
            runOldConfig(objDir);
            buildFiascoOC(objDir);

        } catch (Exception e) {
            throw new IllegalStateException("Fiasco build for architecture '" + architecture + "' failed: " + e.getMessage(), e);
        }
    }

    private String getFiascoBaseDir(AbstractStarshipMojo mojo) {
        return mojo instanceof InitializeMojo ? "StarshipOS/fiasco" : "fiasco";
    }

    private void runMakeCommand(File baseDir, String architecture) throws Exception {
        ProcessBuilder builder = new ProcessBuilder("make", "B=build/" + architecture)
                .directory(baseDir)
                .inheritIO();
        if (builder.start().waitFor() != 0) {
            throw new Exception("Building Fiasco kernel failed.");
        }
    }

    private void copyPrebuiltConfig(File objDir, String arch) throws IOException {
        String resourceName = "fiasco.globalconfig." + arch;
        try (InputStream in = getClass().getClassLoader().getResourceAsStream(resourceName)) {
            if (in == null) throw new IOException("Missing resource: " + resourceName);
            FileUtils.copyStreamToFile(() -> in, new File(objDir, "globalconfig.out"));
        }
    }

    private void runOldConfig(File objDir) throws Exception {
        ProcessBuilder builder = new ProcessBuilder("make", "olddefconfig")
                .directory(objDir)
                .inheritIO();
        if (builder.start().waitFor() != 0) {
            throw new Exception("make olddefconfig failed.");
        }
    }

    private void buildFiascoOC(File objDir) throws Exception {
        //noinspection StringConcatenationMissingWhitespace intentional!
        ProcessBuilder builder = new ProcessBuilder("make", "-j" + Runtime.getRuntime().availableProcessors())
                .directory(objDir)
                .inheritIO();
        if (builder.start().waitFor() != 0) {
            throw new Exception("Fiasco.OC build failed.");
        }
    }

    private void validateDirectory(File dir, String description) throws IOException {
        if (!dir.exists() || !dir.isDirectory()) {
            throw new IOException(description + " directory does not exist: " + dir.getAbsolutePath());
        }
    }
}
