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
import java.io.IOException;


/**
 * Utility class for installing and setting up StarshipOS development environment.
 * This class handles the cloning of necessary repositories, building the HAM tool,
 * and performing initial setup operations for StarshipOS development.
 */
public class InstallHamUtil extends AbstractUtil {

    private static final String hamRepoUrl = "https://github.com/kernkonzept/ham.git";
    private static final String manifestRepoUrl = "https://github.com/kernkonzept/manifest.git";


    public InstallHamUtil(AbstractMojo mojo) {
        super(mojo);
    }

    /**
     * Performs the complete setup of StarshipOS development environment.
     * This includes creating directories, cloning repositories, building the HAM tool,
     * executing necessary HAM commands, and cleaning up temporary files.
     *
     * @throws IllegalStateException if any step of the setup process fails
     */
    public void cloneRepositoriesAndSetupStarshipOS() throws IllegalStateException {
        try {
            File starshipOSDir = createAndNavigateToStarshipOSDirectory();

            cloneRepository(hamRepoUrl, starshipOSDir);
            buildHamTool(new File(starshipOSDir, "ham"));

            executeHamCommands(starshipOSDir);
            verifyRequiredDirectories(starshipOSDir);

            cleanupUnnecessaryDirectories(starshipOSDir);
        } catch (Exception e) {
            String errorMessage = "Failed to clone repositories or set up StarshipOS: " + e.getMessage();
            throw new IllegalStateException(errorMessage, e);
        }
    }

    /**
     * Creates and returns the StarshipOS directory in the current working directory.
     *
     * @return File object representing the StarshipOS directory
     * @throws Exception if directory creation fails
     */
    private File createAndNavigateToStarshipOSDirectory() throws Exception {
        File currentDir = new File(System.getProperty("user.dir"));
        File starshipOSDir = new File(currentDir, "StarshipOS");

        if (!starshipOSDir.exists() && !starshipOSDir.mkdirs()) {
            throw new IOException("Failed to create 'StarshipOS' directory.");
        }

        return starshipOSDir;
    }

    /**
     * Clones a Git repository to the specified working directory.
     *
     * @param repoUrl          URL of the repository to clone
     * @param workingDirectory directory where the repository should be cloned
     * @throws Exception                if the cloning process fails
     * @throws IllegalArgumentException if repository URL is null or blank
     */
    private void cloneRepository(String repoUrl, File workingDirectory) throws Exception {
        if (repoUrl == null || repoUrl.isBlank()) {
            throw new IllegalArgumentException("Repository URL cannot be null or blank.");
        }

        ProcessBuilder builder = new ProcessBuilder("git", "clone", repoUrl, "ham")
                .directory(workingDirectory)
                .inheritIO();
        Process process = builder.start();
        int exitCode = process.waitFor();

        if (exitCode != 0) {
            throw new Exception("Command 'git clone' failed for " + "ham" + " with exit code: " + exitCode);
        }
    }

    /**
     * Builds the HAM tool using make command in the specified directory.
     *
     * @param hamDir directory containing the HAM tool source code
     * @throws Exception if the build process fails or directory doesn't exist
     */
    private void buildHamTool(File hamDir) throws Exception {
        if (!hamDir.exists() || !hamDir.isDirectory()) {
            throw new Exception("'ham' directory does not exist. Cannot build HAM tool.");
        }

        ProcessBuilder builder = new ProcessBuilder("make")
                .directory(hamDir)
                .inheritIO();
        Process process = builder.start();
        int exitCode = process.waitFor();

        if (exitCode != 0) {
            throw new Exception("Building HAM tool failed with exit code: " + exitCode);
        }
    }

    /**
     * Executes necessary HAM commands for initial setup.
     *
     * @param workingDirectory directory where HAM commands should be executed
     * @throws Exception if HAM execution fails or executable is not found
     */
    private void executeHamCommands(File workingDirectory) throws Exception {
        File hamExecutable = new File(workingDirectory, "ham/ham");

        if (!hamExecutable.exists() || !hamExecutable.canExecute()) {
            throw new IOException("HAM executable not found or not executable: " + hamExecutable.getAbsolutePath());
        }

        executeShellCommand(workingDirectory, hamExecutable.getAbsolutePath(), "init", "-u", manifestRepoUrl);
        executeShellCommand(workingDirectory, hamExecutable.getAbsolutePath(), "sync");
    }

    /**
     * Executes a shell command in the specified working directory.
     *
     * @param workingDirectory directory where the command should be executed
     * @param command          variable argument list containing the command and its arguments
     * @throws Exception if the command execution fails
     */
    private void executeShellCommand(File workingDirectory, String... command) throws Exception {
        ProcessBuilder builder = new ProcessBuilder(command)
                .directory(workingDirectory)
                .inheritIO();
        Process process = builder.start();
        int exitCode = process.waitFor();

        if (exitCode != 0) {
            throw new Exception("Command '" + String.join(" ", command) + "' failed with exit code: " + exitCode);
        }
    }

    /**
     * Verifies that all required directories exist after setup.
     *
     * @param starshipOSDir the root StarshipOS directory to check
     * @throws Exception if any required directory is missing
     */
    private void verifyRequiredDirectories(File starshipOSDir) throws Exception {
        File fiascoDir = new File(starshipOSDir, "fiasco");
        File l4Dir = new File(starshipOSDir, "l4");

        if (!fiascoDir.exists() || !fiascoDir.isDirectory()) {
            throw new Exception("Fiasco directory is missing under 'StarshipOS'.");
        }
        if (!l4Dir.exists() || !l4Dir.isDirectory()) {
            throw new Exception("L4 directory is missing under 'StarshipOS'.");
        }
    }

    /**
     * Removes temporary directories that are no longer needed after setup.
     *
     * @param starshipOSDir the root StarshipOS directory
     * @throws IOException if directory deletion fails
     */
    private void cleanupUnnecessaryDirectories(File starshipOSDir) throws IOException {
        File hamDir = new File(starshipOSDir, "ham");
        File hamMetadataDir = new File(starshipOSDir, ".ham");

        deleteDirectoryRecursively(hamDir);
        deleteDirectoryRecursively(hamMetadataDir);
    }

    /**
     * Recursively deletes a directory and all its contents.
     *
     * @param dir the directory to delete
     * @throws IOException if deletion of any file or directory fails
     */
    private void deleteDirectoryRecursively(File dir) throws IOException {
        if (dir.exists()) {
            File[] files = dir.listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.isDirectory()) {
                        deleteDirectoryRecursively(file);
                    } else if (!file.delete()) {
                        throw new IOException("Failed to delete file: " + file.getAbsolutePath());
                    }
                }
            }
            if (!dir.delete()) {
                throw new IOException("Failed to delete directory: " + dir.getAbsolutePath());
            }
        }
    }
}
