// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.container.plugin.bundle;

import com.yahoo.container.plugin.osgi.ExportPackageParser;
import com.yahoo.container.plugin.osgi.ExportPackages.Export;
import com.yahoo.container.plugin.util.JarFiles;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.jar.Manifest;

import static com.yahoo.container.plugin.util.JarFiles.getMainAttributeValue;

/**
 * Static utilities for analyzing jar files.
 *
 * @author Tony Vaagenes
 * @author ollivir
 * @author gjoranv
 */
public class AnalyzeBundle {

    public static List<Export> exportedPackagesAggregated(Collection<File> jarFiles) {
        List<Export> exports = new ArrayList<>();

        for (File jarFile : jarFiles) {
            var exported = exportedPackages(jarFile);
            exports.addAll(exported);
        }
        return exports;
    }

    static List<Export> exportedPackages(File jarFile) {
        var manifest = getOsgiManifest(jarFile);
        if (manifest == null) return Collections.emptyList();
        try {
            return parseExports(manifest);
        } catch (Exception e) {
            throw new RuntimeException(String.format("Invalid manifest in bundle '%s'", jarFile.getPath()), e);
        }
    }

    public static List<String> nonPublicApiPackagesAggregated(Collection<File> jarFiles) {
        return jarFiles.stream()
                .map(AnalyzeBundle::nonPublicApiPackages)
                .flatMap(List::stream)
                .distinct()
                .toList();
    }

    private static List<String> nonPublicApiPackages(File jarFile) {
        var manifest = getOsgiManifest(jarFile);
        if (manifest == null) return Collections.emptyList();
        return getMainAttributeValue(manifest, "X-JDisc-Non-PublicApi-Export-Package")
                .map(s -> Arrays.asList(s.split(",")))
                .orElseGet(ArrayList::new);
    }

    private static Manifest getOsgiManifest(File jarFile) {
        Optional<Manifest> jarManifest = JarFiles.getManifest(jarFile);
        if (jarManifest.isPresent()) {
            Manifest manifest = jarManifest.get();
            if (isOsgiManifest(manifest)) {
                return manifest;
            }
        }
        return null;
    }

    public static Optional<String> bundleSymbolicName(File jarFile) {
        return JarFiles.getManifest(jarFile).flatMap(AnalyzeBundle::getBundleSymbolicName);
    }

    private static List<Export> parseExports(Manifest jarManifest) {
        return getMainAttributeValue(jarManifest, "Export-Package")
                .map(ExportPackageParser::parseExports)
                .orElseGet(ArrayList::new);
    }

    private static boolean isOsgiManifest(Manifest mf) {
        return getBundleSymbolicName(mf).isPresent();
    }

    private static Optional<String> getBundleSymbolicName(Manifest mf) {
        return getMainAttributeValue(mf, "Bundle-SymbolicName");
    }
}
