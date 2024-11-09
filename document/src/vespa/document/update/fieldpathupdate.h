// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "updatevisitor.h"
#include <vespa/document/base/fieldpath.h>
#include <vespa/document/util/identifiableid.h>

namespace document {

class DocumentTypeRepo;
class Field;
class FieldValue;
class BucketIdFactory;
class Document;
class DataType;

namespace select { class Node; }
namespace fieldvalue { class IteratorHandler; }

class FieldPathUpdate
{
public:
    using nbostream = vespalib::nbostream;
    using string_view = std::string_view;

    enum FieldPathUpdateType {
        Add    = IDENTIFIABLE_CLASSID(AddFieldPathUpdate),
        Assign = IDENTIFIABLE_CLASSID(AssignFieldPathUpdate),
        Remove = IDENTIFIABLE_CLASSID(RemoveFieldPathUpdate)
    };

    virtual ~FieldPathUpdate();

    FieldPathUpdateType type() const noexcept { return _type; }
    void applyTo(Document& doc) const;

    virtual bool operator==(const FieldPathUpdate& other) const;
    bool operator!=(const FieldPathUpdate& other) const {
        return ! (*this == other);
    }

    const std::string& getOriginalFieldPath() const { return _originalFieldPath; }
    const std::string& getOriginalWhereClause() const { return _originalWhereClause; }

    /**
     * Check that a given field value is of the type inferred by
     * the field path.
     * @throws IllegalArgumentException upon datatype mismatch.
     */
    void checkCompatibility(const FieldValue& fv, const DataType & type) const;

    virtual void print(std::ostream& out, bool verbose, const std::string& indent) const = 0;

    virtual void accept(UpdateVisitor &visitor) const = 0;
    virtual uint8_t getSerializedType() const = 0;

    /** Deserializes and creates a new FieldPathUpdate instance.
     * Requires type id to be not yet consumed.
     */
    static std::unique_ptr<FieldPathUpdate> createInstance(const DocumentTypeRepo& repo, const DataType &type, nbostream & stream);

protected:
    /** To be used for deserialization */
    explicit FieldPathUpdate(FieldPathUpdateType type) noexcept;
    FieldPathUpdate(const FieldPathUpdate &);
    FieldPathUpdate & operator =(const FieldPathUpdate &);

    static string_view getString(nbostream & stream);
    FieldPathUpdate(FieldPathUpdateType type, string_view fieldPath, string_view whereClause = string_view());

    virtual void deserialize(const DocumentTypeRepo& repo, const DataType& type, nbostream & stream);

    /** @return the datatype of the last path element in the field path */
    const DataType& getResultingDataType(const FieldPath & path) const;
    enum SerializedMagic {AssignMagic=0, RemoveMagic=1, AddMagic=2};
private:
    // TODO: rename to createIteratorHandler?
    virtual std::unique_ptr<fieldvalue::IteratorHandler> getIteratorHandler(Document& doc, const DocumentTypeRepo & repo) const = 0;

    FieldPathUpdateType _type;
    std::string    _originalFieldPath;
    std::string    _originalWhereClause;
};

}
