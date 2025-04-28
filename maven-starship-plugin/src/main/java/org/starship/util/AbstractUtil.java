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

package org.starship.util;

import org.apache.maven.plugin.AbstractMojo;
import org.apache.maven.plugin.logging.Log;

public abstract class AbstractUtil implements StarshipUtil {

    protected final Log log;
    protected AbstractMojo mojo;

    protected AbstractUtil(AbstractMojo mojo) {
        this.mojo = mojo;
        this.log = mojo.getLog();
    }

    @Override
    public void info(String msg) {
        log.info(msg);
    }

    @Override
    public void warn(String msg) {
        log.warn(msg);
    }

    @Override
    public void error(String msg) {
        log.error(msg);
    }

    @Override
    public void debug(String msg) {
        log.debug(msg);
    }

    // Extend with shared functionality (e.g., property handling, file ops, etc.)
}
