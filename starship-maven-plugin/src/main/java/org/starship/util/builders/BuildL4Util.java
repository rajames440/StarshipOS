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

package org.starship.util.builders;

import org.codehaus.plexus.util.FileUtils;
import org.starship.Architecture;
import org.starship.mojo.AbstractStarshipMojo;
import org.starship.mojo.InitializeMojo;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

public class BuildL4Util {

    private final AbstractStarshipMojo mojo;

    public BuildL4Util(AbstractStarshipMojo mojo) {
        this.mojo = mojo;
    }

    private String getL4BaseDir(AbstractStarshipMojo mojo) {
        return mojo instanceof InitializeMojo ? "StarshipOS/l4" : "l4";
    }

    public final void buildL4Re(String architecture) {
        try {
            File l4Dir = new File(getL4BaseDir(mojo));

            validateDirectory(l4Dir);
            setupBuild(l4Dir, architecture);
            copyPrebuiltConfig(l4Dir, Architecture.from(architecture).getClassifier());
            buildL4Re(l4Dir);
        } catch (Exception e) {
            throw new IllegalStateException("L4Re build for architecture '" + architecture + "' failed: " + e.getMessage(), e);
        }
    }

    private void setupBuild(File dir, String architecture) throws Exception {
        if (new ProcessBuilder("make", "B=build/" + architecture).directory(dir).inheritIO().start().waitFor() != 0) {
            throw new Exception("Setup build failed.");
        }
    }

    private void buildL4Re(File dir) throws Exception {
        if (new ProcessBuilder("make").directory(dir).inheritIO().start().waitFor() != 0) {
            throw new Exception("make failed for L4Re.");
        }
    }

    private void copyPrebuiltConfig(File objDir, String arch) throws IOException {
        String resourceName = "l4.config." + arch;
        try (InputStream in = getClass().getClassLoader().getResourceAsStream(resourceName)) {
            if (in == null) throw new IOException("Missing config: " + resourceName);
            FileUtils.copyStreamToFile(() -> in, new File(objDir, ".config"));
        }
    }

    private void validateDirectory(File dir) throws IOException {
        if (!dir.exists() || !dir.isDirectory()) {
            throw new IOException("L4Re directory invalid: " + dir.getAbsolutePath());
        }
    }
}
