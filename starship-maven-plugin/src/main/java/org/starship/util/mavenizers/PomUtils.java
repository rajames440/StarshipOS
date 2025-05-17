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
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import java.io.File;

public class PomUtils {

    public static void insertModuleEntry(File pomFile, String moduleName, Log log) throws Exception {
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        Document doc = factory.newDocumentBuilder().parse(pomFile);
        doc.getDocumentElement().normalize();

        NodeList modulesList = doc.getElementsByTagName("modules");
        Element modulesElement;
        if (modulesList.getLength() == 0) {
            modulesElement = doc.createElement("modules");
            doc.getElementsByTagName("project").item(0).appendChild(modulesElement);
        } else {
            modulesElement = (Element) modulesList.item(0);
        }

        NodeList children = modulesElement.getElementsByTagName("module");
        for (int i = 0; i < children.getLength(); i++) {
            if (children.item(i).getTextContent().equals(moduleName)) {
                log.warn("Module already exists in pom.xml");
                return;
            }
        }

        Element newModule = doc.createElement("module");
        newModule.setTextContent(moduleName);
        modulesElement.appendChild(newModule);

        Transformer transformer = TransformerFactory.newInstance().newTransformer();
        transformer.setOutputProperty(OutputKeys.INDENT, "yes");
        transformer.transform(new DOMSource(doc), new StreamResult(pomFile));
    }
}
