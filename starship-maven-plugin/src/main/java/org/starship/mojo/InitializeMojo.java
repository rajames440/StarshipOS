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
import org.starship.util.downloaders.InstallToolchainUtil;

import java.io.File;
import java.io.IOException;

@SuppressWarnings("unused")
@Mojo(
        name = "initialize",
        defaultPhase = LifecyclePhase.INITIALIZE,
        aggregator = true,
        requiresProject = false
)
public class InitializeMojo extends AbstractStarshipMojo {


    @Override
    public final void doExecute() throws MojoExecutionException {


        try {
            getLog().info("*******************************************************");
            getLog().info("  Installing toolchains for ARM and x86_64");
            getLog().info("*******************************************************");
            InstallToolchainUtil installToolchainUtil = new InstallToolchainUtil(this);
            installToolchainUtil.installToolchain();
        } catch (Exception e) {
            throw new MojoExecutionException("Failed to initialize StarshipOS: " + e.getMessage(), e);
        }
    }

    private File returnToProjectDirectory(File projectDir) throws IOException {
        if (!projectDir.exists() && !projectDir.mkdirs()) {
            throw new IOException("Failed to access or create project directory: " + projectDir.getAbsolutePath());
        }

        if (!projectDir.setWritable(true)) {
            getLog().warn("Could not set project directory writable: " + projectDir.getAbsolutePath());
        }

        return projectDir;
    }
}
