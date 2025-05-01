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

import org.apache.maven.execution.MavenSession;
import org.apache.maven.plugin.AbstractMojo;
import org.apache.maven.plugin.BuildPluginManager;
import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.plugins.annotations.Component;
import org.apache.maven.plugins.annotations.Parameter;
import org.apache.maven.project.MavenProject;
import org.codehaus.plexus.util.FileUtils;

import java.io.*;
import java.util.Properties;

public abstract class AbstractStarshipMojo extends AbstractMojo {

    public File baseDir;
    public File projectRoot;
    public File configDir;

    public boolean installToolchain;
    public boolean installCodebase;
    public boolean buildFiasco;
    public boolean buildFiasco_ARM;
    public boolean buildFiasco_x86_64;
    public boolean buildL4;
    public boolean buildL4_ARM;
    public boolean buildL4_x86_64;
    public boolean buildJDK;
    public boolean buildJDK_ARM;
    public boolean buildJDK_x86_64;
    public boolean runQEMU;
    public boolean runQEMU_ARM;
    public boolean runQEMU_x86_64;


    @Component
    protected BuildPluginManager pluginManager;

    @Parameter(defaultValue = "${project}", readonly = true)
    protected MavenProject project;

    @Parameter(defaultValue = "${session}", readonly = true)
    protected MavenSession session;

    @Override
    public void execute() throws MojoExecutionException {
        try {
            initializeProjectRoot();
            processPropertiesFile();
        } catch (IOException e) {
            getLog().error("Failed to initialize StarshipOS: " + e.getMessage());
            throw new RuntimeException(e);
        }

        doExecute();
    }

    protected abstract void doExecute() throws MojoExecutionException;

    protected void initializeProjectRoot() throws IOException {
        String projectName = System.getProperty("starship.project-name", "StarshipOS");
        baseDir = new File(System.getProperty("user.dir"));

        if (baseDir.getName().equals(projectName)) {
            // If the base directory's name matches the project name, set it as the project root
            projectRoot = baseDir;
        } else {
            // Otherwise, create the project root directory inside the base directory
            projectRoot = new File(baseDir, projectName);

            if (!projectRoot.exists() && !projectRoot.mkdirs()) {
                throw new RuntimeException("Failed to create project root directory: " + projectRoot.getAbsolutePath());
            }
        }

        configDir = new File(projectRoot.getAbsolutePath(), ".starship");
        FileUtils.mkdir(configDir.getAbsolutePath());
    }

    protected void processPropertiesFile() throws IOException {
        Properties properties = new Properties();

        // Define the location in the working directory
        File externalPropertiesFile = new File(configDir, "starship-dev.properties");

        // Check if the external file exists
        if (externalPropertiesFile.exists()) {
            try (InputStream inputStream = new FileInputStream(externalPropertiesFile)) {
                properties.load(inputStream);
                getLog().info("Loaded properties from external file: " + externalPropertiesFile.getAbsolutePath());
            }
        } else {
            // Fall back to loading from resources if the external file does not exist
            try (InputStream inputStream = getClass().getClassLoader().getResourceAsStream("starship-dev.properties")) {
                if (inputStream == null) {
                    throw new FileNotFoundException("Resource 'starship-dev.properties' not found.");
                }
                properties.load(inputStream);
                getLog().info("Loaded properties from classpath resource: 'starship-dev.properties'");
            }
        }

        // Set the properties to object fields
        installToolchain = Boolean.parseBoolean(properties.getProperty("installToolchain", "false"));
        installCodebase = Boolean.parseBoolean(properties.getProperty("installCodebase", "false"));
        buildFiasco = Boolean.parseBoolean(properties.getProperty("buildFiasco", "false"));
        buildFiasco_ARM = Boolean.parseBoolean(properties.getProperty("buildFiasco.ARM", "false"));
        buildFiasco_x86_64 = Boolean.parseBoolean(properties.getProperty("buildFiasco.x86_64", "false"));
        buildL4 = Boolean.parseBoolean(properties.getProperty("buildL4", "false"));
        buildL4_ARM = Boolean.parseBoolean(properties.getProperty("buildL4.ARM", "false"));
        buildL4_x86_64 = Boolean.parseBoolean(properties.getProperty("buildL4.x86_64", "false"));
        buildJDK = Boolean.parseBoolean(properties.getProperty("buildJDK", "false"));
        buildJDK_ARM = Boolean.parseBoolean(properties.getProperty("buildJDK.ARM", "false"));
        buildJDK_x86_64 = Boolean.parseBoolean(properties.getProperty("buildJDK.x86_64", "false"));
        runQEMU = Boolean.parseBoolean(properties.getProperty("runQEMU", "false"));
        runQEMU_ARM = Boolean.parseBoolean(properties.getProperty("runQEMU.ARM", "false"));
        runQEMU_x86_64 = Boolean.parseBoolean(properties.getProperty("runQEMU.x86_64", "false"));

        // Ensure the config directory exists
        if (!configDir.exists() && !configDir.mkdirs()) {
            throw new IOException("Failed to create directory: " + configDir.getAbsolutePath());
        }

        // Save the properties back to the file in the config directory
        File propertiesFile = new File(configDir, "starship-dev.properties");
        try (FileWriter writer = new FileWriter(propertiesFile)) {
            properties.store(writer, "Starship Development Updated Properties");
            getLog().info("Properties have been updated and stored at: " + propertiesFile.getAbsolutePath());
            getLog().info("properties: " + properties);
        }
    }

}