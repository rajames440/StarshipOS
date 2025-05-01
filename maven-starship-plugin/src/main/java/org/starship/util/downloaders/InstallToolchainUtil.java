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

package org.starship.util.downloaders;

import org.apache.maven.plugin.AbstractMojo;
import org.starship.util.AbstractUtil;

import java.io.Console;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Locale;


/**
 * Utility class for installing required development toolchains and dependencies
 * across different Linux distributions. Supports Debian, Fedora, RedHat and Arch
 * based systems.
 */
public class InstallToolchainUtil extends AbstractUtil {

    public InstallToolchainUtil(AbstractMojo mojo) {
        super(mojo);
    }

    /**
     * Installs all required development toolchains and dependencies for the current
     * operating system. Detects the OS flavor and executes appropriate installation
     * commands.
     *
     * @throws IOException          If there are issues with file operations or command execution
     * @throws InterruptedException If the installation process is interrupted
     */
    public void installToolchain() throws IOException, InterruptedException {
        // 1. Detect operating system flavor
        String osFlavor = detectOSFlavor();
        if (osFlavor == null) {
            throw new IOException("Unsupported operating system. Unable to install toolchain.");
        }

        // 2. Build and execute the installation command
        installDependencies(osFlavor);
    }

    /**
     * Detects the Linux distribution flavor by reading /etc/os-release file.
     *
     * @return String identifying the OS flavor ("debian", "fedora", "redhat", "arch")
     * or null if unsupported
     * @throws IOException If the os-release file cannot be read
     */
    private String detectOSFlavor() throws IOException {
        if (new File("/etc/os-release").exists()) {
            String osRelease = Files.readString(Path.of("/etc/os-release")).toLowerCase(Locale.ROOT);
            if (osRelease.contains("ubuntu") || osRelease.contains("debian")) {
                return "debian";
            } else if (osRelease.contains("fedora")) {
                return "fedora";
            } else if (osRelease.contains("red hat") || osRelease.contains("centos") || osRelease.contains("rhel")) {
                return "redhat";
            } else if (osRelease.contains("arch")) {
                return "arch";
            }
        }
        return null; // Unsupported OS
    }

    /**
     * Installs required development dependencies based on the detected OS flavor.
     * Handles both universal and architecture-specific dependencies.
     *
     * @param osFlavor The detected operating system flavor
     * @throws IOException          If there are issues with command execution or console access
     * @throws InterruptedException If the installation process is interrupted
     */
    private void installDependencies(String osFlavor) throws IOException, InterruptedException {
        // Define universal dependencies (always required)
        List<String> dependencies = List.of(
                "git", "make", "gcc", "g++", "libc6-dev",
                "libncurses-dev", "autoconf", "libasound2-dev",
                "libcups2-dev", "libfontconfig1-dev",
                "libx11-dev", "libxext-dev", "libxrender-dev",
                "libxrandr-dev", "libxtst-dev", "libxt-dev"
        );

        // Define architecture-specific toolchains for both x86_64 and ARM
        List<String> architectureSpecificDependencies = List.of(
                "libc6-dev-i386",      // x86_64 32-bit support
                "gcc-arm-linux-gnueabihf", // ARM GCC
                "g++-arm-linux-gnueabihf"  // ARM G++
        );

        // Combine all dependencies
        List<String> allDependencies = concatLists(dependencies, architectureSpecificDependencies);

        String baseCommand;

        // Use traditional switch for Java 11 compatibility
        switch (osFlavor) {
            case "debian":
                baseCommand = "apt-get install -y";
                break;
            case "redhat":
                baseCommand = "yum install -y";
                break;
            case "fedora":
                baseCommand = "dnf install -y";
                break;
            case "arch":
                baseCommand = "pacman -Syu --noconfirm";
                break;
            default:
                throw new IOException("Unsupported OS flavor: " + osFlavor);
        }

        // Prompt for password if needed
        Console console = System.console();
        if (console == null) {
            throw new IOException("Console unavailable to request sudo privileges.");
        }

        String password = "";
        if (!isRootUser()) {
            System.out.println("Root privileges required. Enter sudo password:");
            password = new String(console.readPassword("Password: "));
        }

        // Execute the command
        String installCommand = (password.isEmpty() ? "" : "echo \"" + password + "\" | sudo -S ") + baseCommand + " " + String.join(" ", allDependencies);
        executeCommand(installCommand);
    }

    /**
     * Checks if the current user has root privileges.
     *
     * @return true if current user is root, false otherwise
     * @throws IOException If the user ID command execution fails
     */
    private boolean isRootUser() throws IOException {
        ProcessBuilder builder = new ProcessBuilder("id", "-u");
        Process process = builder.start();
        return new String(process.getInputStream().readAllBytes()).trim().equals("0");
    }

    /**
     * Executes a shell command in the project directory.
     *
     * @param command The shell command to execute
     * @throws IOException          If the command execution fails
     * @throws InterruptedException If the command execution is interrupted
     */
    private void executeCommand(String command) throws IOException, InterruptedException {
        ProcessBuilder builder = new ProcessBuilder("bash", "-c", command);
//        builder.directory(mojo);
        builder.inheritIO();
        Process process = builder.start();

        int exitCode = process.waitFor();
        if (exitCode != 0) {
            throw new IOException("Command failed with exit code " + exitCode);
        }
    }

    // Utility helper to concatenate two lists

    /**
     * Concatenates two lists into a single list.
     *
     * @param list1 The first list to concatenate
     * @param list2 The second list to concatenate
     * @return A new list containing all elements from both input lists
     */
    private List<String> concatLists(List<String> list1, List<String> list2) {
        List<String> fullList = new java.util.ArrayList<>(list1);
        fullList.addAll(list2);
        return fullList;
    }
}
