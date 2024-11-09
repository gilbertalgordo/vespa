// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <memory>
#include <vespa/vespalib/util/exceptions.h>
#include <cstdint>

namespace document {

class DataType;

/**
 * \class document::InvalidDataTypeException
 * \ingroup base
 *
 * \brief Exception used to report invalid datatype usage.
 */
class InvalidDataTypeException : public vespalib::IllegalStateException
{
public:
    InvalidDataTypeException(const DataType &actual,
                             const DataType &wanted,
                             const std::string & location);
    ~InvalidDataTypeException() override;

    const DataType& getActualDataType() const { return _actual; }
    const DataType& getExpectedDataType() const { return _expected; }

    VESPA_DEFINE_EXCEPTION_SPINE(InvalidDataTypeException);

private:
    const DataType &_actual;
    const DataType &_expected;

};

/**
 * \class document::InvalidDataTypeConversionException
 * \ingroup base
 *
 * \brief Exception used to report invalid datatype convertion.
 */
class InvalidDataTypeConversionException
    : public vespalib::IllegalStateException
{
public:
    InvalidDataTypeConversionException(const DataType &actual,
                                       const DataType &wanted,
                                       const std::string & location);
    ~InvalidDataTypeConversionException() override;

    const DataType& getActualDataType() const { return _actual; }
    const DataType& getExpectedDataType() const { return _expected; }

    VESPA_DEFINE_EXCEPTION_SPINE(InvalidDataTypeConversionException);

private:
    const DataType &_actual;
    const DataType &_expected;

};

/**
 * \class document::DocumentTypeNotFoundException
 * \ingroup base
 *
 * \brief Exception used when a document type is not found.
 */
class DocumentTypeNotFoundException : public vespalib::Exception
{
private:
    std::string _type;

public:
    DocumentTypeNotFoundException(std::string name, const std::string& location);
    DocumentTypeNotFoundException(const DocumentTypeNotFoundException &);
    DocumentTypeNotFoundException & operator = (const DocumentTypeNotFoundException &);
    DocumentTypeNotFoundException(DocumentTypeNotFoundException &&) noexcept = default;
    DocumentTypeNotFoundException & operator = (DocumentTypeNotFoundException &&) noexcept = default;
    ~DocumentTypeNotFoundException() override;

    const std::string& getDocumentTypeName() const { return _type; }

    VESPA_DEFINE_EXCEPTION_SPINE(DocumentTypeNotFoundException);
};

/**
 * \class document::DataTypeNotFoundException
 * \ingroup base
 *
 * \brief Exception used when a data type is not found.
 */
class DataTypeNotFoundException : public vespalib::Exception
{
public:
    DataTypeNotFoundException(int id, const std::string& location);
    DataTypeNotFoundException(const std::string& name, const std::string& location);
    ~DataTypeNotFoundException() override;

    VESPA_DEFINE_EXCEPTION_SPINE(DataTypeNotFoundException);
};

/**
 * \class document::AnnotationTypeNotFoundException
 * \ingroup base
 *
 * \brief Exception used when an annotation type is not found.
 */
class AnnotationTypeNotFoundException : public vespalib::Exception
{
public:
    AnnotationTypeNotFoundException(int id, const std::string& location);
    ~AnnotationTypeNotFoundException() override;

    VESPA_DEFINE_EXCEPTION_SPINE(AnnotationTypeNotFoundException);
};

/**
 * \class document::FieldNotFoundException
 * \ingroup base
 *
 * \brief Typically thrown when accessing non-existing fields in structured
 *        datatypes.
 */
class FieldNotFoundException : public vespalib::Exception
{
private:
    std::string _fieldName;
    int32_t _fieldId;

public:
    FieldNotFoundException(std::string fieldName, const std::string& location);
    FieldNotFoundException(int32_t fieldId, int16_t serializationVersion, const std::string& location);
    FieldNotFoundException(const FieldNotFoundException &);
    FieldNotFoundException & operator = (const FieldNotFoundException &);
    FieldNotFoundException(FieldNotFoundException &&) noexcept = default;
    FieldNotFoundException & operator = (FieldNotFoundException &&) noexcept = default;
    ~FieldNotFoundException() override;

    const std::string& getFieldName() const { return _fieldName; }
    int32_t getFieldId()              const { return _fieldId; };

    VESPA_DEFINE_EXCEPTION_SPINE(FieldNotFoundException);
};

VESPA_DEFINE_EXCEPTION(WrongTensorTypeException, vespalib::Exception);

}
