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
        defaultPhase = LifecyclePhase.COMPILE,
        aggregator = true,
        requiresProject = false
)
public class ConfigUIMojo extends AbstractStarshipMojo {

    @Override
    protected void doExecute() throws MojoExecutionException {
        // Define the `.starship` directory and JAR file name
        File starshipDir = new File(".starship");
        File jarFile = new File(starshipDir, "starshipConfig.jar");

        // Ensure the `.starship` directory exists
        if (!starshipDir.exists() || !starshipDir.isDirectory()) {
            throw new MojoExecutionException(".starship directory does not exist in the project root!");
        }

        // Ensure the JAR file exists
        if (!jarFile.exists() || !jarFile.isFile()) {
            throw new MojoExecutionException("JAR not found: " + jarFile.getAbsolutePath());
        }

        // Construct the command to run the JAR
        String command = String.format("java -jar %s", jarFile.getAbsolutePath());

        // Execute the command
        try {
            getLog().info("Executing: " + command);
            Process process = new ProcessBuilder()
                    .command("java", "-jar", jarFile.getAbsolutePath())
                    .directory(starshipDir) // Set the .starship directory as the working directory
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