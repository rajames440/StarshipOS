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

import org.apache.maven.plugin.AbstractMojo;
import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;
import org.apache.maven.plugins.annotations.Parameter;
import org.starship.util.mavenizers.ModuleScaffolder;
import org.starship.util.mavenizers.PomUtils;

import java.io.File;

@Mojo(name = "add-module", defaultPhase = LifecyclePhase.NONE)
public class AddModuleMojo extends AbstractMojo {

    @Parameter(property = "name", required = true)
    private String moduleName;

    @Parameter(property = "bundle", defaultValue = "false")
    private boolean isBundle;

    @Parameter(defaultValue = "${project.basedir}", readonly = true)
    private File baseDir;

    public void execute() throws MojoExecutionException {
        try {
            File moduleDir = new File(baseDir, moduleName);
            if (!moduleDir.exists()) {
                getLog().info("Creating module directory: " + moduleDir);
                moduleDir.mkdirs();
            }

            File rootPom = new File(baseDir, "pom.xml");
            PomUtils.insertModuleEntry(rootPom, moduleName, getLog());

            ModuleScaffolder scaffold = new ModuleScaffolder(getLog());
            scaffold.createStandardStructure(moduleDir);
            scaffold.writePomFile(moduleDir, moduleName, isBundle);

            getLog().info("Module '" + moduleName + "' added successfully.");
        } catch (Exception e) {
            throw new MojoExecutionException("Failed to add module: " + moduleName, e);
        }
    }
}
