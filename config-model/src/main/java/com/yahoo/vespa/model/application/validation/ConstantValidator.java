// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.application.validation;

import com.yahoo.config.application.api.ApplicationFile;
import com.yahoo.config.application.api.ApplicationPackage;
import com.yahoo.config.model.application.provider.FilesApplicationPackage;
import com.yahoo.path.Path;
import com.yahoo.schema.DistributableResource;
import com.yahoo.schema.RankProfile;
import com.yahoo.schema.Schema;
import com.yahoo.vespa.model.application.validation.ConstantTensorJsonValidator.InvalidConstantTensorException;
import com.yahoo.vespa.model.application.validation.Validation.Context;

import java.io.FileNotFoundException;

/**
 * RankingConstantsValidator validates all constant tensors (ranking constants) bundled with an application package
 *
 * @author Vegard Sjonfjell
 */
public class ConstantValidator implements Validator {

    @Override
    public void validate(Context context) {
        var exceptionMessageCollector = new ExceptionMessageCollector("Invalid constant tensor file(s):");
        for (Schema schema : context.deployState().getSchemas()) {
            for (var constant : schema.declaredConstants().values())
                validate(constant, context.deployState().getApplicationPackage(), exceptionMessageCollector);
            for (var profile : context.deployState().rankProfileRegistry().rankProfilesOf(schema)) {
                for (var constant : profile.declaredConstants().values())
                    validate(constant, context.deployState().getApplicationPackage(), exceptionMessageCollector);
            }
        }

        if (exceptionMessageCollector.exceptionsOccurred)
            context.illegal(exceptionMessageCollector.combinedMessage);
    }

    private void validate(RankProfile.Constant constant,
                          ApplicationPackage applicationPackage,
                          ExceptionMessageCollector exceptionMessageCollector) {
        try {
            validate(constant, applicationPackage);
        } catch (InvalidConstantTensorException | FileNotFoundException exception) {
            exceptionMessageCollector.add(constant, exception);
        }
    }

    private void validate(RankProfile.Constant rankingConstant, ApplicationPackage application) throws FileNotFoundException {
        // Only validate file backed constants:
        if (rankingConstant.valuePath().isEmpty()) return;
        if (rankingConstant.pathType().get() != DistributableResource.PathType.FILE) return;

        String constantFile = rankingConstant.valuePath().get();
        if (application.getFileReference(Path.fromString("")).getAbsolutePath().endsWith(FilesApplicationPackage.preprocessed)
            && constantFile.startsWith(FilesApplicationPackage.preprocessed)) {
            constantFile = constantFile.substring(FilesApplicationPackage.preprocessed.length());
        }

        ApplicationFile tensorApplicationFile = application.getFile(Path.fromString(constantFile));
        new ConstantTensorJsonValidator(rankingConstant.type()).validate(constantFile, tensorApplicationFile.createReader());
    }

    private static class ExceptionMessageCollector {

        String combinedMessage;
        boolean exceptionsOccurred = false;

        ExceptionMessageCollector(String messagePrelude) {
            this.combinedMessage = messagePrelude;
        }

        public void add(RankProfile.Constant constant, Exception exception) {
            exceptionsOccurred = true;
            combinedMessage += "\n" + constant.name() + " " + constant.type() + ": file:" + constant.valuePath().get() +
                               ": " + exception.getMessage();
        }
    }

}
