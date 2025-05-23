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
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
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

    public final void installToolchain() throws IOException, InterruptedException {
        String osFlavor = detectOSFlavor();
        if (osFlavor == null) {
            throw new IOException("Unsupported operating system. Unable to install toolchain.");
        }

        installDependencies(osFlavor);
    }

    private @Nullable String detectOSFlavor() throws IOException {
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
        return null;
    }

    private void installDependencies(String osFlavor) throws IOException, InterruptedException {
        List<String> universal = List.of(
                "git", "make", "gcc", "g++", "libc6-dev",
                "libncurses-dev", "autoconf", "libasound2-dev",
                "libcups2-dev", "libfontconfig1-dev",
                "libx11-dev", "libxext-dev", "libxrender-dev",
                "libxrandr-dev", "libxtst-dev", "libxt-dev",
                "flex", "bison", "device-tree-compiler"
        );

        List<String> l4reToolchains = List.of(
                // x86 (32-bit)
                "libc6-dev-i386", "gcc-multilib", "g++-multilib",

                // ARM (32-bit)
                "gcc-arm-linux-gnueabi", "g++-arm-linux-gnueabi",
                "gcc-arm-linux-gnueabihf", "g++-arm-linux-gnueabihf",

                // ARM64 (AArch64)
                "gcc-aarch64-linux-gnu", "g++-aarch64-linux-gnu"
        );

        List<String> openJdkGuesses = List.of(
                // For later OpenJDK cross-compilation on ARM
                "libc6-dev-armhf-cross",        // for armhf
                "libc6-dev-arm64-cross",        // for aarch64
                "libstdc++-arm-linux-gnueabihf-dev",
                "libstdc++-aarch64-linux-gnu-dev"
        );

        List<String> all = concatLists(universal, concatLists(l4reToolchains, openJdkGuesses));

        String baseCommand = switch (osFlavor) {
            case "debian" -> "apt-get install -y";
            case "redhat" -> "yum install -y";
            case "fedora" -> "dnf install -y";
            case "arch" -> "pacman -Syu --noconfirm";
            default -> throw new IOException("Unsupported OS flavor: " + osFlavor);
        };

        Console console = System.console();
        if (console == null) {
            throw new IOException("Console unavailable to request sudo privileges.");
        }

        String password = "";
        if (!isRootUser()) {
            System.out.println("Root privileges required. Enter sudo password:");
            password = new String(console.readPassword("Password: "));
        }

        String installCommand = (password.isEmpty() ? "" : "echo \"" + password + "\" | sudo -S ") +
                baseCommand + " " + String.join(" ", all);

        executeCommand(installCommand);
    }

    private boolean isRootUser() throws IOException {
        ProcessBuilder builder = new ProcessBuilder("id", "-u");
        Process process = builder.start();
        return new String(process.getInputStream().readAllBytes()).trim().equals("0");
    }

    private void executeCommand(String command) throws IOException, InterruptedException {
        ProcessBuilder builder = new ProcessBuilder("bash", "-c", command);
        builder.inheritIO();
        Process process = builder.start();

        int exitCode = process.waitFor();
        if (exitCode != 0) {
            throw new IOException("Command failed with exit code " + exitCode);
        }
    }

    private @NotNull List<String> concatLists(List<String> list1, List<String> list2) {
        List<String> fullList = new java.util.ArrayList<>(list1);
        fullList.addAll(list2);
        return fullList;
    }
}
