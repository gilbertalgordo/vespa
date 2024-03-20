// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.application.validation;

import com.yahoo.config.application.api.ValidationOverrides;
import com.yahoo.vespa.model.application.validation.Validation.Context;

import java.io.Reader;
import java.util.Optional;

/**
 * Validate validation overrides (validation-overrides.xml). Done as a validator to make sure this is
 * done when validating the mode and not when building the model
 *
 * @author hmusum
 */
public class ValidationOverridesValidator implements Validator {

    @Override
    public void validate(Context context) {
        Optional<Reader> overrides = context.deployState().getApplicationPackage().getValidationOverrides();
        if (overrides.isEmpty()) return;

        ValidationOverrides validationOverrides = ValidationOverrides.fromXml(overrides.get());
        validationOverrides.validate(context.deployState().now(), context::illegal);
    }

}
