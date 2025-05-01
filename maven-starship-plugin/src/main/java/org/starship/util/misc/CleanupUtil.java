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

package org.starship.util.misc;

import java.io.File;


/**
 * Utility class for cleaning up Git metadata files and directories from a project.
 * This includes removing .git directories and Git-specific files.
 */
@SuppressWarnings("ResultOfMethodCallIgnored")
public class CleanupUtil {

    /**
     * Recursively removes Git metadata from a directory and its subdirectories.
     * This includes .git directories and Git-specific files.
     *
     * @param rootDir the root directory to start the cleanup process from
     */
    public void scrubGitMetadata(File rootDir) {
        if (!rootDir.exists() || !rootDir.isDirectory()) return;

        File[] files = rootDir.listFiles();
        if (files == null) return;

        for (File file : files) {
            if (file.isDirectory()) {
                if (file.getName().equals(".git")) {
                    deleteRecursively(file);
                } else {
                    scrubGitMetadata(file);
                }
            } else if (isGitMetadataFile(file.getName())) {
                file.delete();
            }
        }
    }

    /**
     * Checks if a file is a Git metadata file.
     *
     * @param fileName the name of the file to check
     * @return true if the file is a Git metadata file, false otherwise
     */
    private boolean isGitMetadataFile(String fileName) {
        return fileName.equals(".gitignore") ||
                fileName.equals(".gitattributes") ||
                fileName.equals(".gitmodules") ||
                fileName.equals(".gitkeep");
    }

    /**
     * Recursively deletes a file or directory and all its contents.
     *
     * @param file the file or directory to delete
     */
    private void deleteRecursively(File file) {
        File[] contents = file.listFiles();
        if (contents != null) {
            for (File f : contents) {
                deleteRecursively(f);
            }
        }
        file.delete();
    }

    /**
     * Scrubs Git metadata from the StarshipOS directory in the current working directory.
     * This method prepares the project for a clean Git initialization.
     */
    public void scrubAndInit() {
        File root = new File(System.getProperty("user.dir"), "StarshipOS");
        scrubGitMetadata(root);
//        initializeGitRepository(root);
    }
}
