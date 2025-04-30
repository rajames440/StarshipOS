package org.starship;

import org.w3c.dom.*;
import javax.swing.*;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.*;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import java.awt.*;
import java.io.*;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Properties;

public class App {
    private static JCheckBox installToolchainCheckBox;
    private static JCheckBox initCodebaseCheckBox;
    private static JCheckBox buildFiascoCheckBox;
    private static JCheckBox buildFiascoARMCheckBox;
    private static JCheckBox buildFiascoX86CheckBox;
    private static JCheckBox buildL4CheckBox;
    private static JCheckBox buildL4ARMCheckBox;
    private static JCheckBox buildL4X86CheckBox;
    private static JCheckBox buildJDKCheckBox;
    private static JCheckBox buildJDKARMCheckBox;
    private static JCheckBox buildJDKX86CheckBox;
    private static JCheckBox runQEMUCheckBox;
    private static JCheckBox runQEMUARMCheckBox;
    private static JCheckBox runQEMUX86CheckBox;
    private static final Map<String, JCheckBox> moduleCheckboxes = new LinkedHashMap<>();
    private static JPanel modulesPanel;

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            JFrame frame = createAndShowGUI();
            readProperties(args[0]);
            readPom(args[1], frame);
        });
    }

    private static JFrame createAndShowGUI() {
        JFrame frame = new JFrame("Configure StarshipOS build");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(600, 400);
        frame.setLayout(new BorderLayout());

        JTabbedPane tabbedPane = new JTabbedPane();
        tabbedPane.addTab("Properties", createPropertiesPanel());
        modulesPanel = new JPanel();
        modulesPanel.setLayout(new BoxLayout(modulesPanel, BoxLayout.Y_AXIS));
        tabbedPane.addTab("Modules", modulesPanel);
        frame.add(tabbedPane, BorderLayout.CENTER);

        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        JButton saveButton = new JButton("Save & Exit");
        JButton cancelButton = new JButton("Cancel");

        saveButton.addActionListener(e -> {
            savePropertiesToFile(".starship/starship-dev.properties");
            saveModulesToPom("./pom.xml");
            System.exit(0);
        });

        cancelButton.addActionListener(e -> System.exit(0));

        buttonPanel.add(saveButton);
        buttonPanel.add(cancelButton);

        frame.add(buttonPanel, BorderLayout.SOUTH);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
        return frame;
    }

    private static JPanel createPropertiesPanel() {
        JPanel propertiesPanel = new JPanel();
        propertiesPanel.setLayout(new GridLayout(6, 3, 5, 5));

        installToolchainCheckBox = new JCheckBox("Install Toolchain");
        initCodebaseCheckBox = new JCheckBox("Initialize Codebase");
        buildFiascoCheckBox = new JCheckBox("Build Fiasco.OC");
        buildFiascoARMCheckBox = new JCheckBox("ARM");
        buildFiascoX86CheckBox = new JCheckBox("x86_64");
        buildL4CheckBox = new JCheckBox("Build L4Re");
        buildL4ARMCheckBox = new JCheckBox("ARM");
        buildL4X86CheckBox = new JCheckBox("x86_64");
        buildJDKCheckBox = new JCheckBox("Build OpenJDK 21");
        buildJDKARMCheckBox = new JCheckBox("ARM");
        buildJDKX86CheckBox = new JCheckBox("x86_64");
        runQEMUCheckBox = new JCheckBox("Run QEMU");
        runQEMUARMCheckBox = new JCheckBox("ARM");
        runQEMUX86CheckBox = new JCheckBox("x86_64");

        propertiesPanel.add(installToolchainCheckBox);
        propertiesPanel.add(new JLabel(""));
        propertiesPanel.add(new JLabel(""));
        propertiesPanel.add(initCodebaseCheckBox);
        propertiesPanel.add(new JLabel(""));
        propertiesPanel.add(new JLabel(""));
        propertiesPanel.add(buildFiascoCheckBox);
        propertiesPanel.add(buildFiascoX86CheckBox);
        propertiesPanel.add(buildFiascoARMCheckBox);
        propertiesPanel.add(buildL4CheckBox);
        propertiesPanel.add(buildL4X86CheckBox);
        propertiesPanel.add(buildL4ARMCheckBox);
        propertiesPanel.add(buildJDKCheckBox);
        propertiesPanel.add(buildJDKX86CheckBox);
        propertiesPanel.add(buildJDKARMCheckBox);
        propertiesPanel.add(runQEMUCheckBox);
        propertiesPanel.add(runQEMUX86CheckBox);
        propertiesPanel.add(runQEMUARMCheckBox);

        return propertiesPanel;
    }

    private static void readProperties(String filePath) {
        Properties properties = new Properties();
        try (FileInputStream fis = new FileInputStream(filePath)) {
            properties.load(fis);
            installToolchainCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("installToolchain", "false")));
            initCodebaseCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("initCodebase", "false")));
            buildFiascoCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildFiasco", "false")));
            buildFiascoARMCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildFiasco.ARM", "false")));
            buildFiascoX86CheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildFiasco.x86", "false")));
            buildL4CheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildL4", "false")));
            buildL4ARMCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildL4.ARM", "false")));
            buildL4X86CheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildL4.x86", "false")));
            buildJDKCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildJDK", "false")));
            buildJDKARMCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildJDK.ARM", "false")));
            buildJDKX86CheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("buildJDK.x86", "false")));
            runQEMUCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("runQEMU", "false")));
            runQEMUARMCheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("runQEMU.ARM", "false")));
            runQEMUX86CheckBox.setSelected(Boolean.parseBoolean(properties.getProperty("runQEMU.x86", "false")));
        } catch (IOException e) {
            System.err.println("Failed to load properties file: " + filePath);
        }
    }

    private static void savePropertiesToFile(String filePath) {
        Properties properties = new Properties();
        properties.setProperty("installToolchain", Boolean.toString(installToolchainCheckBox.isSelected()));
        properties.setProperty("initCodebase", Boolean.toString(initCodebaseCheckBox.isSelected()));
        properties.setProperty("buildFiasco", Boolean.toString(buildFiascoCheckBox.isSelected()));
        properties.setProperty("buildFiasco.ARM", Boolean.toString(buildFiascoARMCheckBox.isSelected()));
        properties.setProperty("buildFiasco.x86", Boolean.toString(buildFiascoX86CheckBox.isSelected()));
        properties.setProperty("buildL4", Boolean.toString(buildL4CheckBox.isSelected()));
        properties.setProperty("buildL4.ARM", Boolean.toString(buildL4ARMCheckBox.isSelected()));
        properties.setProperty("buildL4.x86", Boolean.toString(buildL4X86CheckBox.isSelected()));
        properties.setProperty("buildJDK", Boolean.toString(buildJDKCheckBox.isSelected()));
        properties.setProperty("buildJDK.ARM", Boolean.toString(buildJDKARMCheckBox.isSelected()));
        properties.setProperty("buildJDK.x86", Boolean.toString(buildJDKX86CheckBox.isSelected()));
        properties.setProperty("runQEMU", Boolean.toString(runQEMUCheckBox.isSelected()));
        properties.setProperty("runQEMU.ARM", Boolean.toString(runQEMUARMCheckBox.isSelected()));
        properties.setProperty("runQEMU.x86", Boolean.toString(runQEMUX86CheckBox.isSelected()));

        try (FileOutputStream fos = new FileOutputStream(filePath)) {
            properties.store(fos, "Starship Dev Properties");
        } catch (IOException e) {
            System.err.println("Failed to save properties file: " + filePath);
        }
    }

    private static void readPom(String pomFilePath, JFrame frame) {
        try {
            moduleCheckboxes.clear();
            modulesPanel.removeAll();

            File pomFile = new File(pomFilePath);
            if (!pomFile.exists()) return;

            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = factory.newDocumentBuilder();
            Document document = builder.parse(pomFile);

            NodeList modulesNodeList = document.getElementsByTagName("modules");
            if (modulesNodeList.getLength() > 0) {
                Node modulesNode = modulesNodeList.item(0);
                NodeList moduleElements = modulesNode.getChildNodes();

                for (int i = 0; i < moduleElements.getLength(); i++) {
                    Node moduleNode = moduleElements.item(i);
                    if (moduleNode.getNodeType() == Node.ELEMENT_NODE && moduleNode.getNodeName().equals("module")) {
                        String moduleName = moduleNode.getTextContent().trim();
                        JCheckBox checkBox = new JCheckBox(moduleName, true);
                        moduleCheckboxes.put(moduleName, checkBox);
                        modulesPanel.add(checkBox);
                    } else if (moduleNode.getNodeType() == Node.COMMENT_NODE) {
                        String comment = moduleNode.getTextContent().trim();
                        if (comment.startsWith("<module>") && comment.endsWith("</module>")) {
                            String moduleName = comment.substring(8, comment.length() - 9).trim();
                            JCheckBox checkBox = new JCheckBox(moduleName, false);
                            moduleCheckboxes.put(moduleName, checkBox);
                            modulesPanel.add(checkBox);
                        }
                    }
                }
            }

            modulesPanel.revalidate();
            modulesPanel.repaint();
        } catch (Exception ignored) {
        }
    }

    private static void saveModulesToPom(String pomFilePath) {
        try {
            File pomFile = new File(pomFilePath);
            if (!pomFile.exists()) return;

            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = factory.newDocumentBuilder();
            Document document = builder.parse(pomFile);

            NodeList modulesNodeList = document.getElementsByTagName("modules");
            if (modulesNodeList.getLength() > 0) {
                Node modulesNode = modulesNodeList.item(0);

                while (modulesNode.hasChildNodes()) {
                    modulesNode.removeChild(modulesNode.getFirstChild());
                }

                for (String moduleName : moduleCheckboxes.keySet()) {
                    JCheckBox checkBox = moduleCheckboxes.get(moduleName);
                    if (checkBox.isSelected()) {
                        Element moduleElement = document.createElement("module");
                        moduleElement.setTextContent(moduleName);
                        modulesNode.appendChild(moduleElement);
                    } else {
                        Comment comment = document.createComment("<module>" + moduleName + "</module>");
                        modulesNode.appendChild(comment);
                    }
                }

                TransformerFactory transformerFactory = TransformerFactory.newInstance();
                Transformer transformer = transformerFactory.newTransformer();
                transformer.setOutputProperty(OutputKeys.INDENT, "yes");
                DOMSource source = new DOMSource(document);
                StreamResult result = new StreamResult(pomFile);
                transformer.transform(source, result);
            }
        } catch (Exception ignored) {
        }
    }
}
