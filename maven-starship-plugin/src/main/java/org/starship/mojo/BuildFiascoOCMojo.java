package org.starship.mojo;

import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;
import org.starship.util.builders.BuildFiascoUtil;

@Mojo(
        name = "build-fiasco",
        defaultPhase = LifecyclePhase.COMPILE,
        aggregator = true,
        requiresProject = false
)
public class BuildFiascoOCMojo extends AbstractStarshipMojo {

    @Override
    protected void doExecute() {
        try {
            if(buildFiasco) {
                BuildFiascoUtil util = new BuildFiascoUtil(this);
                if (buildFiasco_x86_64) util.buildFiasco("x86_64");
                if (buildFiasco_ARM) util.buildFiasco("arm");
            }
        } catch(Exception e) {
            getLog().error("Failed to build FiascoOC: ", e);
        }
        getLog().info("Built FiascoOC successfully.");
    }
}
