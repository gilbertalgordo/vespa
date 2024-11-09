// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/searchcore/proton/common/document_type_inspector.h>
#include <vespa/document/base/field.h>
#include <vespa/document/datatype/datatypes.h>
#include <vespa/vespalib/gtest/gtest.h>

using namespace document;
using namespace proton;

namespace document_type_inspector_test {

template <class Type>
void
addFields(Type &type, bool fieldF3IsString, bool hasFieldF4, bool hasFieldF5)
{
    type.addField(Field("f1", 1, *DataType::STRING));
    type.addField(Field("f2", 2, *DataType::STRING));
    type.addField(Field("f3", 3, fieldF3IsString ? *DataType::STRING : *DataType::INT));
    if (hasFieldF4) {
        type.addField(Field("f4", 4, *DataType::STRING));
    }
    if (hasFieldF5) {
        type.addField(Field("f5", 5, *DataType::STRING));
    }
}

struct DocumentTypeFixture
{
    DocumentType _documentType;
    StructDataType _structFieldType;
    ArrayDataType _structArrayFieldType;
    MapDataType _structMapFieldType;
    MapDataType _mapFieldType;

    DocumentTypeFixture(bool fieldF3IsString, bool hasFieldF4, bool hasFieldF5, bool hasStruct, bool mapKeyIsByte);
    ~DocumentTypeFixture();
};

DocumentTypeFixture::DocumentTypeFixture(bool fieldF3IsString, bool hasFieldF4, bool hasFieldF5, bool hasStruct, bool mapKeyIsByte)
    : _documentType("test"),
      _structFieldType("struct"),
      _structArrayFieldType(_structFieldType),
      _structMapFieldType(mapKeyIsByte ? *DataType::BYTE : *DataType::STRING, _structFieldType),
      _mapFieldType(mapKeyIsByte ? *DataType::BYTE : *DataType::STRING, *DataType::STRING)
{
    addFields(_documentType, fieldF3IsString, hasFieldF4, hasFieldF5);
    if (hasStruct) {
        addFields(_structFieldType, fieldF3IsString, hasFieldF4, hasFieldF5);
        _documentType.addField(Field("sarray", 11, _structArrayFieldType));
        _documentType.addField(Field("smap", 12, _structMapFieldType));
        _documentType.addField(Field("map", 13, _mapFieldType));
    }
}

DocumentTypeFixture::~DocumentTypeFixture() = default;

struct Fixture
{
    DocumentTypeFixture _oldDocType;
    DocumentTypeFixture _newDocType;
    DocumentTypeInspector _inspector;
    explicit Fixture(bool hasStruct = true, bool mapKeyIsByte = false)
        : _oldDocType(true, true, false, hasStruct, mapKeyIsByte),
          _newDocType(false, false, true, true, false),
          _inspector(_oldDocType._documentType, _newDocType._documentType)
    {
    }
};

TEST(DocumentTypeInspectorTest, require_that_unchanged_fields_are_known)
{
    Fixture f;
    const IDocumentTypeInspector &inspector = f._inspector;
    EXPECT_TRUE(inspector.hasUnchangedField("f1"));
    EXPECT_TRUE(inspector.hasUnchangedField("f2"));
    EXPECT_TRUE(inspector.hasUnchangedField("sarray.f1"));
    EXPECT_TRUE(inspector.hasUnchangedField("sarray.f2"));
    EXPECT_TRUE(inspector.hasUnchangedField("smap.key"));
    EXPECT_TRUE(inspector.hasUnchangedField("smap.value.f1"));
    EXPECT_TRUE(inspector.hasUnchangedField("smap.value.f2"));
    EXPECT_TRUE(inspector.hasUnchangedField("map.key"));
    EXPECT_TRUE(inspector.hasUnchangedField("map.value"));
}

TEST(DocumentTypeInspectorTest, require_that_changed_fields_are_detected)
{
    Fixture f;
    const IDocumentTypeInspector &inspector = f._inspector;
    EXPECT_FALSE(inspector.hasUnchangedField("f3"));
    EXPECT_FALSE(inspector.hasUnchangedField("sarray.f3"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.value.f3"));
}

TEST(DocumentTypeInspectorTest, require_that_partially_missing_fields_are_detected)
{
    Fixture f;
    const IDocumentTypeInspector &inspector = f._inspector;
    EXPECT_FALSE(inspector.hasUnchangedField("f4"));
    EXPECT_FALSE(inspector.hasUnchangedField("f5"));
    EXPECT_FALSE(inspector.hasUnchangedField("sarray.f4"));
    EXPECT_FALSE(inspector.hasUnchangedField("sarray.f5"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.value.f4"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.value.f5"));
}

TEST(DocumentTypeInspectorTest, require_that_non_existing_fields_are_NOT_known)
{
    Fixture f;
    const IDocumentTypeInspector &inspector = f._inspector;
    EXPECT_FALSE(inspector.hasUnchangedField("not"));
    EXPECT_FALSE(inspector.hasUnchangedField("sarray.not"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.not"));
}

TEST(DocumentTypeInspectorTest, require_that_map_key_type_change_is_detected)
{
    Fixture f(true, true);
    const IDocumentTypeInspector &inspector = f._inspector;
    EXPECT_FALSE(inspector.hasUnchangedField("smap.key"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.value.f1"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.value.f2"));
    EXPECT_FALSE(inspector.hasUnchangedField("map.key"));
    EXPECT_FALSE(inspector.hasUnchangedField("map.value"));
}

TEST(DocumentTypeInspectorTest, require_that_struct_addition_is_detected)
{
    Fixture f(false, false);
    const IDocumentTypeInspector &inspector = f._inspector;
    EXPECT_FALSE(inspector.hasUnchangedField("sarray.f1"));
    EXPECT_FALSE(inspector.hasUnchangedField("sarray.f2"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.key"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.value.f1"));
    EXPECT_FALSE(inspector.hasUnchangedField("smap.value.f2"));
    EXPECT_FALSE(inspector.hasUnchangedField("map.key"));
    EXPECT_FALSE(inspector.hasUnchangedField("map.value"));
}

}
