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

import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
import java.util.Properties;

@Mojo(name = "smart-clean", defaultPhase = LifecyclePhase.CLEAN)
public class SmartCleanMojo extends AbstractStarshipMojo {

    @Override
    protected final void doExecute() throws MojoExecutionException {
        boolean didAnything = false;

        try {
            if (cleanFiasco) {
                cleanDir("fiasco");
                didAnything = true;
            }

            if (cleanL4) {
                cleanDir("l4");
                didAnything = true;
            }

            if (cleanJDK) {
                cleanDir("openjdk");
                didAnything = true;
            }

            if (!didAnything) {
                getLog().info("[Smart Clean] No clean actions requested. All clean flags are false.");
                return;
            }

            resetCleanFlags();

        } catch (IOException e) {
            String message = "[Smart Clean] Failed to clean build directories: " + e.getMessage();
            getLog().error(message);
            throw new MojoExecutionException(message, e);
        }
    }

    private void cleanDir(String moduleName) throws IOException {
        File moduleDir = new File(projectRoot, moduleName);
        File buildDir = new File(moduleDir, "build");

        if (buildDir.exists()) {
            getLog().info("[Smart Clean] Deleting: " + buildDir.getAbsolutePath());
            deleteRecursively(buildDir.toPath());
        } else {
            getLog().info("[Smart Clean] Skipping: " + buildDir.getAbsolutePath() + " (does not exist)");
        }
    }

    private void deleteRecursively(Path path) throws IOException {
        Files.walk(path)
                .sorted(Comparator.reverseOrder())
                .map(Path::toFile)
                .forEach(file -> {
                    if (!file.delete()) {
                        throw new UncheckedIOException(new IOException("Failed to delete: " + file.getAbsolutePath()));
                    }
                });
    }

    private void resetCleanFlags() throws IOException {
        File propertiesFile = new File(configDir, "starship-dev.properties");

        if (!propertiesFile.exists()) {
            throw new FileNotFoundException("Missing properties file: " + propertiesFile.getAbsolutePath());
        }

        Properties props = new Properties();
        try (FileInputStream in = new FileInputStream(propertiesFile)) {
            props.load(in);
        }

        props.setProperty("cleanFiasco", "false");
        props.setProperty("cleanL4", "false");
        props.setProperty("cleanJDK", "false");

        try (FileOutputStream out = new FileOutputStream(propertiesFile)) {
            props.store(out, "Starship Development Updated Properties");
            getLog().info("[Smart Clean] Reset clean flags to false.");
        }
    }
}
