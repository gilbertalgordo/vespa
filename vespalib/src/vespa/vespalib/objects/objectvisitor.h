// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <string>

namespace vespalib {

/**
 * This is an abstract class used to visit structured objects. It
 * contains a basic interface that is intended to be overridden by
 * subclasses. As an extension to this class, the visit.hpp file
 * contains various versions of the visit method that maps visitation
 * of various types into invocations of the basic interface defined by
 * this class.
 **/
class ObjectVisitor
{
public:
    /**
     * Open a (sub-)structure
     *
     * @param name name of structure
     * @param type type of structure
     **/
    virtual void openStruct(std::string_view name, std::string_view type) = 0;

    /**
     * Close a (sub-)structure
     **/
    virtual void closeStruct() = 0;

    /**
     * Visit a boolean value
     *
     * @param name variable name
     * @param value variable value
     **/
    virtual void visitBool(std::string_view name, bool value) = 0;

    /**
     * Visit an integer value
     *
     * @param name variable name
     * @param value variable value
     **/
    virtual void visitInt(std::string_view name, int64_t value) = 0;

    /**
     * Visit a floating-point value
     *
     * @param name variable name
     * @param value variable value
     **/
    virtual void visitFloat(std::string_view name, double value) = 0;

    /**
     * Visit a string value
     *
     * @param name variable name
     * @param value variable value
     **/
    virtual void visitString(std::string_view name, std::string_view value) = 0;

    /**
     * Visit method used to indicate that an optional substructure is
     * not present.
     *
     * @param name variable name
     **/
    virtual void visitNull(std::string_view name) = 0;

    /**
     * Visit method invoked by the default implementation of member
     * visitation to signal that member visitation is not yet implemented.
     *
     * @param name variable name
     * @param value variable value
     **/
    virtual void visitNotImplemented() = 0;

    /**
     * empty
     **/
    virtual ~ObjectVisitor();
};

} // namespace vespalib

