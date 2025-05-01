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

import java.io.File;
import java.io.IOException;

/**
 * Maven Mojo to execute the Starship Configuration UI JAR.
 */
@Mojo(
        name = "configure",
        defaultPhase = LifecyclePhase.NONE,
        aggregator = true,
        requiresProject = false
)
public class ConfigUIMojo extends AbstractStarshipMojo {

    @Override
    protected void doExecute() throws MojoExecutionException {
        // Define the `.starship` directory and files
        File jarFile = new File(".starship/starshipConfig.jar");
        File propsFile = new File(".starship/starship.properties");
        File pomFile = new File("pom.xml");

        // Construct the FULL command as ONE string
        String command = "java -jar " + jarFile.getAbsolutePath() + " " + propsFile.getAbsolutePath() + " " + pomFile.getAbsolutePath();
        // Execute the command
        try {
            Process process = new ProcessBuilder("bash", "-c", command) // Use bash for full string as a command
                    .inheritIO()                                        // Inherit I/O for visibility
                    .start();
            int exitCode = process.waitFor();
            if (exitCode != 0) {
                getLog().error("Starship Config JAR exited with code: " + exitCode);
                throw new MojoExecutionException("Failed to execute the Starship Config JAR. Exit code: " + exitCode);
            }
        } catch (IOException | InterruptedException e) {
            getLog().error("Error while executing the Starship Config JAR: ", e);
            throw new MojoExecutionException("Error while executing the Starship Config JAR: " + e.getMessage(), e);
        }
    }
}
