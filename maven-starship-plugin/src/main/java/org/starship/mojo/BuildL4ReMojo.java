package org.starship.mojo;

import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;
import org.starship.util.builders.BuildL4Util;

@Mojo(
        name = "build-l4",
        defaultPhase = LifecyclePhase.COMPILE,
        aggregator = true,
        requiresProject = false
)
public class BuildL4ReMojo extends AbstractStarshipMojo {
    @Override
    protected void doExecute() {
        try {
            if(buildL4) {
                BuildL4Util util = new BuildL4Util(this);
                if (buildL4_x86_64) util.buildL4("x86_64");
                if (buildL4_ARM) util.buildL4("arm");
            }
        } catch(Exception e) {
            getLog().error("Failed to build L4Re: ", e);
        }
        getLog().info("Built L4Re successfully.");
    }
}
