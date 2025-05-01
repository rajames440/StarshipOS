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

// MavenizeUtil.java
package org.starship.util.mavenizers;

import org.apache.maven.plugin.AbstractMojo;
import org.starship.util.AbstractUtil;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.List;

/**
 * Utility class for generating Maven POM files for StarshipOS modules.
 * This class provides functionality to create a multi-module Maven project structure
 * with predefined configurations for StarshipOS components.
 */
public class MavenizeUtil extends AbstractUtil {

    public MavenizeUtil(AbstractMojo mojo) {
        super(mojo);
    }

    private static List<String> writeModulePom(File projectRoot) throws IOException {
        List<String> modules = List.of("fiasco", "l4", "openjdk");

        for (String module : modules) {
            File moduleDir = new File(projectRoot, module);
            if (!moduleDir.exists() && !moduleDir.mkdirs()) {
                throw new IOException("Failed to create module directory: " + moduleDir.getAbsolutePath());
            }

            File pomFile = new File(moduleDir, "pom.xml");
            try (FileWriter writer = new FileWriter(pomFile)) {
                writer.write("<project xmlns=\"http://maven.apache.org/POM/4.0.0\"\n");
                writer.write("         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
                writer.write("         xsi:schemaLocation=\"http://maven.apache.org/POM/4.0.0 http://maven.apache.org/xsd/maven-4.0.0.xsd\">\n");
                writer.write("  <modelVersion>4.0.0</modelVersion>\n");
                writer.write("  <parent>\n");
                writer.write("    <groupId>org.starshipos</groupId>\n");
                writer.write("    <artifactId>starship-parent</artifactId>\n");
                writer.write("    <version>1.0.0-SNAPSHOT</version>\n");
                writer.write("    <relativePath>../pom.xml</relativePath>\n");
                writer.write("  </parent>\n");
                writer.write("  <artifactId>" + module + "</artifactId>\n");
                writer.write("  <version>1.0.0-SNAPSHOT</version>\n");
                writer.write("  <build>\n");
                writer.write("    <plugins>\n");
                writer.write("      <plugin>\n");
                writer.write("        <groupId>org.starshipos</groupId>\n");
                writer.write("        <artifactId>starship-maven-plugin</artifactId>\n");
                writer.write("        <version>1.0.0-SNAPSHOT</version>\n");
                writer.write("        <executions>\n");
                writer.write("          <execution>\n");
                writer.write("            <id>build-core</id>\n");
                writer.write("            <goals><goal>build-core</goal></goals>\n");
                writer.write("            <phase>compile</phase>\n");
                writer.write("          </execution>\n");
                writer.write("        </executions>\n");
                writer.write("      </plugin>\n");
                writer.write("    </plugins>\n");
                writer.write("  </build>\n");
                writer.write("</project>\n");
            }
        }
        return modules;
    }

    /**
     * Generates Maven POM files for StarshipOS modules in the specified base directory.
     * Creates module directories if they don't exist and generates both module-specific POMs
     * and a parent POM file with proper Maven configuration.
     *
     * @throws IOException if there are issues creating directories or writing POM files
     */
    public void generateModulePoms(AbstractMojo mojo) throws IOException {
        if (!(mojo instanceof org.starship.mojo.AbstractStarshipMojo starshipMojo)) {
            throw new IllegalStateException("MavenizeUtil.generateModulePoms() requires a StarshipMojo instance.");
        }

        File projectRoot = starshipMojo.projectRoot;
        List<String> modules = writeModulePom(projectRoot);

        File parentPom = new File(projectRoot, "pom.xml");
        try (FileWriter writer = new FileWriter(parentPom)) {
            writer.write("<project xmlns=\"http://maven.apache.org/POM/4.0.0\"\n");
            writer.write("         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
            writer.write("         xsi:schemaLocation=\"http://maven.apache.org/POM/4.0.0 http://maven.apache.org/xsd/maven-4.0.0.xsd\">\n");
            writer.write("  <modelVersion>4.0.0</modelVersion>\n");
            writer.write("  <groupId>org.starshipos</groupId>\n");
            writer.write("  <artifactId>starship-parent</artifactId>\n");
            writer.write("  <version>1.0.0-SNAPSHOT</version>\n");
            writer.write("  <packaging>pom</packaging>\n");
            writer.write("  <modules>\n");
            for (String module : modules) {
                writer.write("    <module>" + module + "</module>\n");
            }
            writer.write("  </modules>\n");
            writer.write("  <dependencies>\n");

            writer.write("    <dependency>\n");
            writer.write("      <groupId>org.openjfx</groupId>\n");
            writer.write("      <artifactId>javafx-controls</artifactId>\n");
            writer.write("      <version>16</version>\n");
            writer.write("    </dependency>\n");

            writer.write("    <dependency>\n");
            writer.write("      <groupId>org.openjfx</groupId>\n");
            writer.write("      <artifactId>javafx-fxml</artifactId>\n");
            writer.write("      <version>16</version>\n");
            writer.write("    </dependency>\n");

            writer.write("    <dependency>\n");
            writer.write("      <groupId>org.openjfx</groupId>\n");
            writer.write("      <artifactId>javafx-graphics</artifactId>\n");
            writer.write("      <version>16</version>\n");
            writer.write("    </dependency>\n");

            writer.write("    <dependency>\n");
            writer.write("      <groupId>org.starshipos</groupId>\n");
            writer.write("      <artifactId>starship-maven-plugin</artifactId>\n");
            writer.write("      <version>1.0.0-SNAPSHOT</version>\n");
            writer.write("    </dependency>\n");


            writer.write("  </dependencies>\n");
            writer.write("  <build>\n");
            writer.write("    <plugins>\n");
            writer.write("      <plugin>\n");
            writer.write("        <groupId>org.starshipos</groupId>\n");
            writer.write("        <artifactId>starship-maven-plugin</artifactId>\n");
            writer.write("        <version>1.0.0-SNAPSHOT</version>\n");
            writer.write("        <executions>\n");
            writer.write("          <execution>\n");
            writer.write("            <id>build-core</id>\n");
            writer.write("            <goals><goal>build-core</goal></goals>\n");
            writer.write("            <phase>compile</phase>\n");
            writer.write("          </execution>\n");
            writer.write("        </executions>\n");
            writer.write("      </plugin>\n");
            writer.write("    </plugins>\n");
            writer.write("  </build>\n");
            writer.write("</project>\n");
        }

        info("Generated module POMs and parent POM in: " + projectRoot.getAbsolutePath());
    }


}
