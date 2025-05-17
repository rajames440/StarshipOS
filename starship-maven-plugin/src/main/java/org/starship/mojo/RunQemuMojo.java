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

import org.apache.maven.plugins.annotations.Mojo;
import org.apache.maven.plugins.annotations.ResolutionScope;

import java.io.IOException;
import java.nio.file.Path;

@Mojo(name = "run-qemu", requiresDependencyResolution = ResolutionScope.NONE)
public class RunQemuMojo extends AbstractStarshipMojo {

    @Override
    protected final void doExecute() {
        Path projectRootPath = projectRoot.toPath();

        Path fiascoBin = projectRootPath
                .resolve("fiasco")
                .resolve("build")
                .resolve("x86_64")
                .resolve("fiasco");

        Path isoPath = projectRootPath
                .resolve("target")
                .resolve("starship-x86_64.iso");

        Path workingDir = projectRootPath
                .resolve("l4")
                .resolve("build")
                .resolve("x86_64");

        getLog().info("[QEMU] Launching QEMU for x86_64");
        getLog().info("[QEMU] Kernel: " + fiascoBin);
        getLog().info("[QEMU] ISO: " + isoPath);

        ProcessBuilder pb = new ProcessBuilder(
                "qemu-system-x86_64",
                "-kernel", fiascoBin.toString(),
                "-cdrom", isoPath.toString(),
                "-serial", "mon:stdio",
                "-m", "512",
                "-no-reboot"
        );

        pb.directory(workingDir.toFile());
        pb.inheritIO();

        try {
            Process qemu = pb.start();
            qemu.waitFor();
        } catch (IOException | InterruptedException e) {
            throw new RuntimeException("Failed to launch QEMU", e);
        }
    }
}
