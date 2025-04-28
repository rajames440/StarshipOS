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

package org.starship.util.downloaders;

import org.apache.maven.plugin.AbstractMojo;
import org.starship.util.AbstractUtil;

import java.io.File;


/**
 * Utility class for downloading and cloning OpenJDK source code from the official repository.
 * This class provides functionality to clone specific versions or the latest version of OpenJDK.
 */
public class DownloadOpenJdkUtil extends AbstractUtil {

    private static final String GIT_URL = "https://github.com/openjdk/jdk.git";

    public DownloadOpenJdkUtil(AbstractMojo mojo) {
        super(mojo);
    }

    /**
     * Clones the OpenJDK repository to a local directory. If a tag is specified, performs a shallow clone
     * of that specific version. Otherwise, performs a full clone of the master branch.
     *
     * @param tag The specific OpenJDK version tag to clone. If null or blank, clones the entire repository
     * @throws Exception If the git clone operation fails or encounters any issues during execution
     */
    public void cloneOpenJdk(String tag) throws Exception {
        File baseDir = new File(System.getProperty("user.dir"), "StarshipOS/openjdk");

        if (baseDir.exists()) {
            return; // Already cloned
        }

        if (tag != null && !tag.isBlank()) {
            ProcessBuilder shallowTagClone = new ProcessBuilder(
                    "git", "clone",
                    "--branch", tag,
                    "--depth", "1",
                    "--single-branch",
                    GIT_URL,
                    baseDir.getAbsolutePath()
            ).inheritIO();

            Process process = shallowTagClone.start();
            if (process.waitFor() != 0) {
                throw new Exception("Failed to shallow-clone OpenJDK tag: " + tag);
            }
        } else {
            // Fall back to full depth master clone
            ProcessBuilder defaultClone = new ProcessBuilder(
                    "git", "clone", GIT_URL, baseDir.getAbsolutePath()
            ).inheritIO();

            Process process = defaultClone.start();
            if (process.waitFor() != 0) {
                throw new Exception("Failed to clone OpenJDK from: " + GIT_URL);
            }
        }
    }
}
