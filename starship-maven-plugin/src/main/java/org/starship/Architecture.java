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

package org.starship;

public enum Architecture {
    X86_64("x86_64", "amd64"),
    ARM("arm", "aarch64", "arm64");

    private final String canonical;
    private final String[] aliases;

    Architecture(String canonical, String... aliases) {
        this.canonical = canonical;
        this.aliases = aliases;
    }

    public static Architecture from(String name) {
        String n = name.toLowerCase();
        for (Architecture arch : values()) {
            if (arch.canonical.equals(n)) return arch;
            for (String alias : arch.aliases) {
                if (alias.equals(n)) return arch;
            }
        }
        throw new IllegalArgumentException("Unknown architecture: " + name);
    }

    public String getClassifier() {
        return canonical;
    }

    @Override
    public String toString() {
        return canonical;
    }
}
