package org.starship.mojo;

import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.plugins.annotations.LifecyclePhase;
import org.apache.maven.plugins.annotations.Mojo;

import java.io.File;
import java.io.IOException;

/**
 * Maven Mojo to execute the Starship Configuration UI JAR.
 */
@Mojo(
        name = "configure",
        defaultPhase = LifecyclePhase.NONE,
        aggregator = true,
        requiresProject = false
)
public class ConfigUIMojo extends AbstractStarshipMojo {

    @Override
    protected void doExecute() throws MojoExecutionException {
        // Define the `.starship` directory and JAR file name
        File jarFile = new File("./", ".starship/starshipConfig.jar");
        File propsFile = new File("./", ".starship/starship.properties");
        File pomFile = new File("./", "pom.xml");

        // Execute the command
        try {
            Process process = new ProcessBuilder()
                    .command("java", "-jar", jarFile.getAbsolutePath(), propsFile.getAbsolutePath(), pomFile.getAbsolutePath())
                    .directory(new File("./")) // Set the .starship directory as the working directory
                    .inheritIO()             // Inherit I/O streams for visibility
                    .start();

            // Wait for the process to complete
            int exitCode = process.waitFor();
            if (exitCode != 0) {
                throw new MojoExecutionException("Failed to execute the Starship Config JAR. Exit code: " + exitCode);
            }

        } catch (IOException | InterruptedException e) {
            throw new MojoExecutionException("Error while executing the Starship Config JAR: " + e.getMessage(), e);
        }
    }
}