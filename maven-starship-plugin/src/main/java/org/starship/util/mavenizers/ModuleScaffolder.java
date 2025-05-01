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

package org.starship.util.mavenizers;

import org.apache.maven.plugin.logging.Log;

import java.io.File;
import java.io.FileWriter;

public class ModuleScaffolder {

    private final Log log;

    public ModuleScaffolder(Log log) {
        this.log = log;
    }

    public void createStandardStructure(File moduleDir) {
        String[] dirs = {
                "src/main/java",
                "src/main/groovy",
                "src/main/resources",
                "src/test/java",
                "src/test/groovy",
                "src/test/resources"
        };
        for (String path : dirs) {
            File dir = new File(moduleDir, path);
            if (!dir.mkdirs()) {
                log.warn("Could not create: " + dir);
            }
        }
    }

    public void writePomFile(File moduleDir, String artifactId, boolean isBundle) throws Exception {
        File pom = new File(moduleDir, "pom.xml");
        try (FileWriter out = new FileWriter(pom)) {
            String content = String.format(
                    "<project xmlns=\"http://maven.apache.org/POM/4.0.0\"\n" +
                            "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n" +
                            "         xsi:schemaLocation=\"http://maven.apache.org/POM/4.0.0 http://maven.apache.org/xsd/maven-4.0.0.xsd\">\n" +
                            "  <modelVersion>4.0.0</modelVersion>\n" +
                            "  <parent>\n" +
                            "    <groupId>org.starship</groupId>\n" +
                            "    <artifactId>starship-parent</artifactId>\n" +
                            "    <version>1.0.0-SNAPSHOT</version>\n" +
                            "  </parent>\n" +
                            "  <artifactId>%s</artifactId>\n" +
                            "  <version>1.0.0-SNAPSHOT</version>\n" +
                            "  <packaging>%s</packaging>\n" +
                            "  <build>\n" +
                            "    <plugins>\n" +
                            "      <plugin>\n" +
                            "        <groupId>org.codehaus.gmavenplus</groupId>\n" +
                            "        <artifactId>gmavenplus-plugin</artifactId>\n" +
                            "        <version>1.15.0</version>\n" +
                            "        <executions>\n" +
                            "          <execution>\n" +
                            "            <goals><goal>compile</goal><goal>testCompile</goal></goals>\n" +
                            "          </execution>\n" +
                            "        </executions>\n" +
                            "      </plugin>\n" +
                            "      <plugin>\n" +
                            "        <groupId>org.apache.maven.plugins</groupId>\n" +
                            "        <artifactId>maven-shade-plugin</artifactId>\n" +
                            "        <version>3.5.1</version>\n" +
                            "        <executions><execution><phase>package</phase><goals><goal>shade</goal></goals></execution></executions>\n" +
                            "      </plugin>\n" +
                            "      <plugin>\n" +
                            "        <groupId>org.apache.maven.plugins</groupId>\n" +
                            "        <artifactId>maven-jarsigner-plugin</artifactId>\n" +
                            "        <version>3.0.0</version>\n" +
                            "        <executions><execution><phase>verify</phase><goals><goal>sign</goal></goals></execution></executions>\n" +
                            "      </plugin>\n" +
                            "    </plugins>\n" +
                            "  </build>\n" +
                            "%s\n" +
                            "</project>\n",
                    artifactId,
                    isBundle ? "bundle" : "jar",
                    isBundle ? getFelixMetadata(artifactId) : ""
            );
            out.write(content);
        }
    }

    private String getFelixMetadata(String artifactId) {
        return String.format(
                "  <properties>\n" +
                        "    <bundle.symbolicName>org.starship.%s</bundle.symbolicName>\n" +
                        "    <bundle.version>1.0.0.SNAPSHOT</bundle.version>\n" +
                        "    <bundle.exportPackage>org.starship.%s.*</bundle.exportPackage>\n" +
                        "    <bundle.importPackage>*</bundle.importPackage>\n" +
                        "  </properties>",
                artifactId, artifactId
        );
    }
}
