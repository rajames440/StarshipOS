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

package org.starship.mojo;

import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;
import org.starship.util.builders.BuildFiascoUtil;
import org.starship.util.builders.BuildJDKUtil;
import org.starship.util.builders.BuildL4Util;
import org.starship.util.downloaders.DownloadOpenJdkUtil;
import org.starship.util.downloaders.InstallHamUtil;
import org.starship.util.downloaders.InstallToolchainUtil;
import org.starship.util.mavenizers.MavenizeUtil;
import org.starship.util.misc.RunHelloQemuUtil;
import org.starship.util.misc.CleanupUtil;

import java.io.File;
import java.io.IOException;

@SuppressWarnings("unused")
@Mojo(
        name = "initialize",
        defaultPhase = LifecyclePhase.NONE,
        aggregator = true,
        requiresProject = false
)
public class InitializeMojo extends AbstractStarshipMojo {


    @Override
    public void doExecute() throws MojoExecutionException {


        try {
            if (installToolchainFlag) {
                getLog().info("*******************************************************");
                getLog().info("  Installing toolchains for ARM and x86_64");
                getLog().info("*******************************************************");
                InstallToolchainUtil installToolchainUtil = new InstallToolchainUtil(this);
                installToolchainUtil.installToolchain();
            }

            if (installCodebase) {
                getLog().info("*******************************************************");
                getLog().info("  Cloning Repositories and Setting up StarshipOS");
                getLog().info("*******************************************************");
                new InstallHamUtil(this).cloneRepositoriesAndSetupStarshipOS();
                new DownloadOpenJdkUtil(this).cloneOpenJdk("jdk-21-ga");
            }

            if (buildFiasco) {
                getLog().info("*******************************************************");
                getLog().info("  Building Fiasco");
                getLog().info("*******************************************************");
                BuildFiascoUtil util = new BuildFiascoUtil();
                if (buildFiasco_x86_64) util.buildFiasco("x86_64");
                if (buildFiasco_ARM) util.buildFiasco("arm");
            }

            if (buildL4) {
                getLog().info("*******************************************************");
                getLog().info("  Building L4");
                getLog().info("*******************************************************");
                BuildL4Util util = new BuildL4Util();
                if (buildL4_x86_64) util.buildL4("x86_64");
                if (buildL4_ARM) util.buildL4("arm");
            }

            if (buildJDK) {
                getLog().info("*******************************************************");
                getLog().info("  Building OpenJDK jdk-21-ga");
                getLog().info("*******************************************************");
                BuildJDKUtil util = new BuildJDKUtil();
                if (buildJDK_x86_64) util.buildJDK("x86_64");
                if (buildJDK_ARM) util.buildJDK("arm");
            }

            if (runQEMU) {
                getLog().info("*******************************************************");
                getLog().info("  Running Hello World Demo in QEMU");
                getLog().info("*******************************************************");
                RunHelloQemuUtil util = new RunHelloQemuUtil();
                if (runQEMU_x86_64) util.runHelloDemo("x86_64");
                if (runQEMU_ARM) util.runHelloDemo("arm");
            }

            getLog().info("*******************************************************");
            getLog().info("  Creating Apache Maven project: StarshipOS");
            getLog().info("*******************************************************");
            new MavenizeUtil(this).generateModulePoms(this);

            getLog().info("*******************************************************");
            getLog().info("  Cleanup. Almost done");
            getLog().info("*******************************************************");
            new CleanupUtil().scrubAndInit();

        } catch (Exception e) {
            throw new MojoExecutionException("Failed to initialize StarshipOS: " + e.getMessage(), e);
        }
    }



    public void warnAndPrompt() {
        getLog().warn("===============================================================");
        getLog().warn("        ⚠️  Some Operations May Require Sudo Privileges ⚠️");
        getLog().warn("===============================================================");
        getLog().warn("This process may request your sudo password when setting up ");
        getLog().warn("toolchains, installing dependencies, or performing privileged tasks.");
        getLog().warn("");
        getLog().warn("        Do NOT run: sudo mvn starship:initialize");
        getLog().warn("        Maven should remain under your user account.");
        getLog().warn("");
        getLog().warn("        Press ENTER to continue or Ctrl+C to abort...");
        getLog().warn("        (Continuing automatically in 5 seconds)");
        getLog().warn("===============================================================");
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
