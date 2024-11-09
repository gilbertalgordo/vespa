// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "filter_attribute_manager.h"
#include <vespa/vespalib/util/isequencedtaskexecutor.h>
#include <vespa/vespalib/util/exceptions.h>
#include <vespa/searchlib/attribute/attributevector.h>
#include <vespa/searchcommon/attribute/i_attribute_functor.h>
#include <vespa/searchlib/attribute/attribute_read_guard.h>

using search::AttributeGuard;
using searchcorespi::IFlushTarget;

namespace proton {

namespace {

const std::string FLUSH_TARGET_NAME_PREFIX("attribute.flush.");
const std::string SHRINK_TARGET_NAME_PREFIX("attribute.shrink.");

class FlushTargetFilter
{
    const std::string &_prefix;
    const IFlushTarget::Type _type;
public:
    FlushTargetFilter(const std::string &prefix, IFlushTarget::Type type)
        : _prefix(prefix),
          _type(type)
    {
    }
    ~FlushTargetFilter();

    bool match(const IFlushTarget::SP &flushTarget) const {
        const std::string &targetName = flushTarget->getName();
        if ((flushTarget->getType() != _type) ||
            (flushTarget->getComponent() != IFlushTarget::Component::ATTRIBUTE)) {
            return false;
        }
        return (targetName.substr(0, _prefix.size()) == _prefix);
    }

    std::string attributeName(const IFlushTarget::SP &flushTarget) {
        const std::string &targetName = flushTarget->getName();
        return targetName.substr(_prefix.size());
    }
};

FlushTargetFilter::~FlushTargetFilter() = default;

FlushTargetFilter syncFilter(FLUSH_TARGET_NAME_PREFIX, IFlushTarget::Type::SYNC);
FlushTargetFilter shrinkFilter(SHRINK_TARGET_NAME_PREFIX, IFlushTarget::Type::GC);

}

bool
FilterAttributeManager::acceptAttribute(std::string_view name) const
{
    return _acceptedAttributes.contains(name);
}

FilterAttributeManager::FilterAttributeManager(AttributeSet acceptedAttributes, IAttributeManager::SP mgr)
    : _acceptedAttributes(std::move(acceptedAttributes)),
      _mgr(std::move(mgr))
{
    // Assume that list of attributes in mgr doesn't change
    for (const auto attr : _mgr->getWritableAttributes()) {
        if (acceptAttribute(attr->getName())) {
            _acceptedWritableAttributes.push_back(attr);
        }
    }
}

FilterAttributeManager::~FilterAttributeManager() = default;

search::attribute::IAttributeContext::UP
FilterAttributeManager::createContext() const {
    throw vespalib::IllegalArgumentException("Not implemented");
}

std::unique_ptr<IAttributeManagerReconfig>
FilterAttributeManager::prepare_create(AttributeCollectionSpec&&) const
{
    throw vespalib::IllegalArgumentException("Not implemented");
}

std::vector<searchcorespi::IFlushTarget::SP>
FilterAttributeManager::getFlushTargets() const {
    std::vector<searchcorespi::IFlushTarget::SP> completeList = _mgr->getFlushTargets();
    std::vector<searchcorespi::IFlushTarget::SP> list;
    list.reserve(completeList.size());
    for (const auto &flushTarget : completeList) {
        if (syncFilter.match(flushTarget)) {
            if (acceptAttribute(syncFilter.attributeName(flushTarget))) {
                list.push_back(flushTarget);
            }
        } else if (shrinkFilter.match(flushTarget)) {
            if (acceptAttribute(shrinkFilter.attributeName(flushTarget))) {
                list.push_back(flushTarget);
            }
        }
    }
    return list;
}

search::SerialNum
FilterAttributeManager::getOldestFlushedSerialNumber() const {
    throw vespalib::IllegalArgumentException("Not implemented");
}
search::SerialNum
FilterAttributeManager::getNewestFlushedSerialNumber() const {
    throw vespalib::IllegalArgumentException("Not implemented");
}
void
FilterAttributeManager::getAttributeListAll(std::vector<search::AttributeGuard> &) const {
    throw vespalib::IllegalArgumentException("Not implemented");
}
void
FilterAttributeManager::pruneRemovedFields(search::SerialNum) {
    throw vespalib::IllegalArgumentException("Not implemented");
}
const IAttributeFactory::SP &
FilterAttributeManager::getFactory() const {
    throw vespalib::IllegalArgumentException("Not implemented");
}

AttributeGuard::UP
FilterAttributeManager::getAttribute(std::string_view name) const
{
    if (acceptAttribute(name)) {
        return _mgr->getAttribute(name);
    }
    return {};
}

std::unique_ptr<search::attribute::AttributeReadGuard>
FilterAttributeManager::getAttributeReadGuard(std::string_view name, bool stableEnumGuard) const
{
    if (acceptAttribute(name)) {
        return _mgr->getAttributeReadGuard(name, stableEnumGuard);
    }
    return {};
}

void
FilterAttributeManager::getAttributeList(std::vector<AttributeGuard> &list) const
{
    std::vector<AttributeGuard> completeList;
    _mgr->getAttributeList(completeList);
    for (const auto &attr : completeList) {
        if (acceptAttribute(attr->getName())) {
            list.push_back(attr);
        }
    }
}

search::SerialNum
FilterAttributeManager::getFlushedSerialNum(const std::string &name) const
{
    if (acceptAttribute(name)) {
        return _mgr->getFlushedSerialNum(name);
    }
    return 0;
}

vespalib::ISequencedTaskExecutor &
FilterAttributeManager::getAttributeFieldWriter() const
{
    return _mgr->getAttributeFieldWriter();
}

search::AttributeVector *
FilterAttributeManager::getWritableAttribute(std::string_view name) const
{
    if (acceptAttribute(name)) {
        return _mgr->getWritableAttribute(name);
    } else {
        return nullptr;
    }
}

const std::vector<search::AttributeVector *> &
FilterAttributeManager::getWritableAttributes() const
{
    return _acceptedWritableAttributes;
}

void
FilterAttributeManager::asyncForEachAttribute(std::shared_ptr<IConstAttributeFunctor> func) const
{
    // Run by document db master thread
    std::vector<AttributeGuard> completeList;
    _mgr->getAttributeList(completeList);
    vespalib::ISequencedTaskExecutor &attributeFieldWriter = getAttributeFieldWriter();
    for (auto &guard : completeList) {
        search::AttributeVector::SP attrsp = guard.getSP();
        // Name must be extracted in document db master thread or attribute
        // writer thread
        attributeFieldWriter.execute(attributeFieldWriter.getExecutorIdFromName(attrsp->getNamePrefix()),
                                     [attrsp, func]() { (*func)(*attrsp); });
    }
}

void
FilterAttributeManager::asyncForEachAttribute(std::shared_ptr<IAttributeFunctor> func, OnDone onDone) const
{
    // Run by document db master thread
    std::vector<AttributeGuard> completeList;
    _mgr->getAttributeList(completeList);
    vespalib::ISequencedTaskExecutor &attributeFieldWriter = getAttributeFieldWriter();
    for (auto &guard : completeList) {
        search::AttributeVector::SP attrsp = guard.getSP();
        // Name must be extracted in document db master thread or attribute
        // writer thread
        attributeFieldWriter.execute(attributeFieldWriter.getExecutorIdFromName(attrsp->getNamePrefix()),
                                     [attrsp, func, onDone]() {
            (void) onDone;
            (*func)(*attrsp);
        });
    }
}

void
FilterAttributeManager::asyncForAttribute(std::string_view name, std::unique_ptr<IAttributeFunctor> func) const {
    AttributeGuard::UP attr = _mgr->getAttribute(name);
    if (!attr) { return; }
    vespalib::ISequencedTaskExecutor &attributeFieldWriter = getAttributeFieldWriter();
    std::string_view attrName = (*attr)->getNamePrefix();
    attributeFieldWriter.execute(attributeFieldWriter.getExecutorIdFromName(attrName),
                                  [attr=std::move(attr), func=std::move(func)]() mutable {
                                      (*func)(**attr);
                                  });

}

void
FilterAttributeManager::setImportedAttributes(std::unique_ptr<ImportedAttributesRepo>)
{
    throw vespalib::IllegalArgumentException("Not implemented");
}

const ImportedAttributesRepo *
FilterAttributeManager::getImportedAttributes() const
{
    return nullptr;
}

std::shared_ptr<search::attribute::ReadableAttributeVector>
FilterAttributeManager::readable_attribute_vector(std::string_view name) const
{
    if (acceptAttribute(name)) {
        return _mgr->readable_attribute_vector(name);
    }
    return {};
}

}
