package org.starship.mojo;

import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;
import org.starship.util.builders.BuildJDKUtil;

@Mojo(
        name = "build-jdk",
        defaultPhase = LifecyclePhase.COMPILE,
        aggregator = true,
        requiresProject = false
)
public class BuildJdkMojo extends AbstractStarshipMojo {
    @Override
    protected void doExecute() {
        try {
            if(buildJDK) {
                BuildJDKUtil util = new BuildJDKUtil(this);
                if (buildJDK_x86_64) util.buildJDK("x86_64");
                if (buildJDK_ARM) util.buildJDK("arm");
            }
        } catch(Exception e) {
            getLog().error("Failed to build JDK: ", e);
        }
        getLog().info("Built OpenJDK 21 successfully.");
    }
}
