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

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

@Mojo(name = "build-iso", defaultPhase = LifecyclePhase.PACKAGE)
public class BuildIsoMojo extends AbstractStarshipMojo {

    @Override
    protected final void doExecute() {
        // Convert once at the top
        Path projectRootPath = projectRoot.toPath();

        Path romfsDir = projectRootPath
                .resolve("target")
                .resolve("l4re-modules");

        Path isoPath = projectRootPath
                .resolve("target")
                .resolve("starship-x86_64.iso");

        getLog().info("[ISO] Building ISO from: " + romfsDir);
        getLog().info("[ISO] Output ISO: " + isoPath);

        if (!Files.exists(romfsDir) || !Files.isDirectory(romfsDir)) {
            throw new RuntimeException("ROMFS directory does not exist or is not a directory: " + romfsDir);
        }

        List<String> command = new ArrayList<>();
        command.add("mkisofs");
        command.add("-quiet");
        command.add("-R");
        command.add("-o");
        command.add(isoPath.toString());
        command.add(romfsDir.toString());

        ProcessBuilder pb = new ProcessBuilder(command);
        pb.directory(projectRoot);  // ✅ FIXED: projectRoot is already File
        pb.inheritIO();

        try {
            Process mkiso = pb.start();
            int result = mkiso.waitFor();
            if (result != 0) {
                throw new RuntimeException("mkisofs failed with exit code: " + result);
            }
        } catch (IOException | InterruptedException e) {
            throw new RuntimeException("Failed to create ISO image", e);
        }

        getLog().info("[ISO] ISO created successfully.");
    }
}
