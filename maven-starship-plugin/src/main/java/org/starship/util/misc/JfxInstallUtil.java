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

package org.starship.util.misc;

import org.apache.maven.plugin.AbstractMojo;
import org.starship.util.AbstractUtil;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URL;
import java.util.Enumeration;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class JfxInstallUtil extends AbstractUtil {

    private static final String JAVAFX_VERSION = "11.0.2"; // Compatible JavaFX version for Java 11
    private static final String BASE_URL = "https://download2.gluonhq.com/openjfx/";

    public JfxInstallUtil(AbstractMojo mojo) {
        super(mojo);
    }

    /**
     * Installs JavaFX SDK for the current operating system.
     *
     * @throws IOException If there are issues with downloading or file operations
     */
    public void installJavaFXSDK() throws IOException {
        // 1. Detect OS and fetch download URL
        String osFlavor = detectOSFlavor();
        if (osFlavor == null) {
            error("Unsupported operating system. Unable to install JavaFX.");
            throw new IOException("Unsupported operating system.");
        }
        String downloadUrl = getJavaFXDownloadURL(osFlavor);

        // 2. Download and extract SDK
        File targetDir = new File("target/javafx-sdk");
        if (!targetDir.exists()) {
            targetDir.mkdirs();
        }
        File zipFile = new File(targetDir, "javafx-sdk.zip");

        info("Downloading JavaFX SDK (" + JAVAFX_VERSION + ") for " + osFlavor + "...");
        downloadFile(downloadUrl, zipFile);

        info("Extracting JavaFX SDK...");
        extractZipFile(zipFile, targetDir);

        info("JavaFX SDK installation completed. SDK available at: " + targetDir.getAbsolutePath());
        zipFile.delete(); // Cleanup
    }

    /**
     * Detects the operating system for selecting the appropriate JavaFX SDK variant.
     *
     * @return "linux", "windows", or "mac" based on the OS, or null if unsupported
     */
    private String detectOSFlavor() {
        String os = System.getProperty("os.name").toLowerCase(Locale.ROOT);
        if (os.contains("win")) {
            return "windows";
        } else if (os.contains("mac")) {
            return "mac";
        } else if (os.contains("nix") || os.contains("nux")) {
            return "linux";
        } else {
            return null; // Unsupported OS
        }
    }

    /**
     * Constructs the download URL for the JavaFX SDK based on the OS flavor.
     *
     * @param osFlavor The detected OS flavor (linux, windows, mac)
     * @return JavaFX SDK download URL
     */
    private String getJavaFXDownloadURL(String osFlavor) {
        return BASE_URL + JAVAFX_VERSION + "/javafx-sdk-" + JAVAFX_VERSION + "-" + osFlavor + "-x64.zip";
    }

    /**
     * Downloads a file from the given URL and saves it to a specified path.
     *
     * @param url        The URL to download from
     * @param targetFile The target file to save to
     * @throws IOException If an I/O error occurs during download
     */
    private void downloadFile(String url, File targetFile) throws IOException {
        try (BufferedInputStream in = new BufferedInputStream(new URL(url).openStream());
             FileOutputStream out = new FileOutputStream(targetFile)) {
            byte[] buffer = new byte[1024];
            int bytesRead;
            while ((bytesRead = in.read(buffer, 0, 1024)) != -1) {
                out.write(buffer, 0, bytesRead);
            }
        }
    }

    /**
     * Extracts a ZIP file to the specified directory.
     *
     * @param zipFile   The ZIP file to extract
     * @param targetDir The directory to extract to
     * @throws IOException If there is an error during extraction
     */
    private void extractZipFile(File zipFile, File targetDir) throws IOException {
        try (ZipFile zip = new ZipFile(zipFile)) {
            Enumeration<? extends ZipEntry> entries = zip.entries();
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();
                File outFile = new File(targetDir, entry.getName());
                if (entry.isDirectory()) {
                    outFile.mkdirs();
                } else {
                    outFile.getParentFile().mkdirs();
                    try (BufferedInputStream in = new BufferedInputStream(zip.getInputStream(entry));
                         FileOutputStream out = new FileOutputStream(outFile)) {
                        byte[] buffer = new byte[1024];
                        int bytesRead;
                        while ((bytesRead = in.read(buffer, 0, 1024)) != -1) {
                            out.write(buffer, 0, bytesRead);
                        }
                    }
                }
            }
        }
    }
}