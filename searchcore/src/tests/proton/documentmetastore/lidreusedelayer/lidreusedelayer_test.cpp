// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/searchcore/proton/documentmetastore/i_store.h>
#include <vespa/searchcore/proton/documentmetastore/lidreusedelayer.h>
#include <vespa/searchcore/proton/test/thread_utils.h>
#include <vespa/searchcore/proton/test/threading_service_observer.h>
#include <vespa/searchcore/proton/test/transport_helper.h>
#include <vespa/vespalib/gtest/gtest.h>
#include <vespa/vespalib/util/lambdatask.h>
#include <vespa/vespalib/util/destructor_callbacks.h>
#include <vespa/vespalib/util/size_literals.h>
#include <vespa/vespalib/util/gate.h>

#include <vespa/log/log.h>
LOG_SETUP("lidreusedelayer_test");

using vespalib::makeLambdaTask;

namespace proton {

namespace {

bool
assertThreadObserver(uint32_t masterExecuteCnt,
                     uint32_t indexExecuteCnt,
                     uint32_t summaryExecuteCnt,
                     const test::ThreadingServiceObserver &observer)
{
    bool failed = false;
    EXPECT_EQ(masterExecuteCnt, observer.masterObserver().getExecuteCnt()) << (failed = true, "");
    if (failed) {
        return false;
    }
    EXPECT_EQ(summaryExecuteCnt, observer.summaryObserver().getExecuteCnt()) << (failed = true, "");
    if (failed) {
        return false;
    }
    EXPECT_EQ(indexExecuteCnt, observer.indexObserver().getExecuteCnt()) << (failed = true, "");
    return !failed;
}

}

class MyMetaStore : public documentmetastore::IStore
{
public:
    bool     _freeListActive;
    uint32_t _removes_complete_count;
    uint32_t _removes_complete_lids;

    MyMetaStore()
        : _freeListActive(false),
          _removes_complete_count(0),
          _removes_complete_lids(0)
    {
    }

    ~MyMetaStore() override = default;

    Result inspectExisting(const GlobalId &, uint64_t) override {
        return Result();
    }

    Result inspect(const GlobalId &, uint64_t) override {
        return Result();
    }

    Result put(const GlobalId &, const BucketId &, Timestamp , uint32_t, DocId, uint64_t) override {
        return Result();
    }

    bool updateMetaData(DocId, const BucketId &, Timestamp ) override {
        return true;
    }

    bool remove(DocId, uint64_t) override {
        return true;
    }

    void removes_complete(const std::vector<DocId>& lids) override{
        ++_removes_complete_count;
        _removes_complete_lids += lids.size();
    }

    void move(DocId, DocId, uint64_t) override {
    }

    bool validLid(DocId) const override {
        return true;
    }

    void removeBatch(const std::vector<DocId> &, const DocId) override {}

    const RawDocumentMetaData &getRawMetaData(DocId) const override {
        LOG_ABORT("should not be reached");
    }

    bool getFreeListActive() const override {
        return _freeListActive;
    }

    bool
    assertWork(uint32_t exp_removes_complete_count,
               uint32_t exp_removes_complete_lids) const
    {
        bool failed = false;
        EXPECT_EQ(exp_removes_complete_count, _removes_complete_count) << (failed = true, "");
        if (failed) {
            return false;
        }
        EXPECT_EQ(exp_removes_complete_lids, _removes_complete_lids) << (failed = true, "");
        return !failed;
    }
};

class LidReuseDelayerTest : public ::testing::Test
{
protected:
    using LidReuseDelayer = documentmetastore::LidReuseDelayer;
    TransportAndExecutorService _service;
    test::ThreadingServiceObserver _writeService;
    MyMetaStore _store;
    std::unique_ptr<LidReuseDelayer> _lidReuseDelayer;

    LidReuseDelayerTest();
    ~LidReuseDelayerTest() override;

    template <typename FunctionType>
    void runInMasterAndSync(FunctionType func) {
        test::runInMasterAndSync(_writeService, func);
    }

    void cycledLids(const std::vector<uint32_t> &lids) {
        _store.removes_complete(lids);
    }

    void performCycleLids(const std::vector<uint32_t> &lids, vespalib::IDestructorCallback::SP onDone);

    void cycleLids(const std::vector<uint32_t> &lids, vespalib::IDestructorCallback::SP onDone);

    void delayReuse(uint32_t lid) {
        runInMasterAndSync([&]() { _lidReuseDelayer->delayReuse(lid); });
    }

    void delayReuse(const std::vector<uint32_t> &lids) {
        runInMasterAndSync([&]() { _lidReuseDelayer->delayReuse(lids); });
    }

    void commit() {
        vespalib::Gate gate;
        test::runInMaster(_writeService, [this, onDone=std::make_shared<vespalib::GateCallback>(gate)]() {
            cycleLids(_lidReuseDelayer->getReuseLids(), std::move(onDone));
        });
        gate.await();
    }
};

LidReuseDelayerTest::LidReuseDelayerTest()
    : _service(1),
      _writeService(_service.write()),
      _store(),
      _lidReuseDelayer(std::make_unique<LidReuseDelayer>(_writeService, _store))
{
}

LidReuseDelayerTest::~LidReuseDelayerTest()
{
    commit();
}

void
LidReuseDelayerTest::cycleLids(const std::vector<uint32_t> &lids, vespalib::IDestructorCallback::SP onDone) {
    if (lids.empty())
        return;
    _writeService.index().execute(makeLambdaTask([this, lids, onDone]() {
        (void) onDone;
        performCycleLids(lids, onDone);
    }));
}

void
LidReuseDelayerTest::performCycleLids(const std::vector<uint32_t> &lids, vespalib::IDestructorCallback::SP onDone) {
    _writeService.master().execute(makeLambdaTask([this, lids, onDone]() {
        (void) onDone;
        cycledLids(lids);
    }));
}

TEST_F(LidReuseDelayerTest, require_that_nothing_happens_before_free_list_is_active)
{
    delayReuse(4);
    delayReuse({ 5, 6});
    EXPECT_TRUE(_store.assertWork(0, 0));
    EXPECT_TRUE(assertThreadObserver(2, 0, 0, _writeService));
}

TEST_F(LidReuseDelayerTest, require_that_reuse_can_be_batched)
{
    _store._freeListActive = true;
    delayReuse(4);
    delayReuse({ 5, 6, 7});
    EXPECT_TRUE(_store.assertWork(0, 0));
    EXPECT_TRUE(assertThreadObserver(2, 0, 0, _writeService));
    commit();
    EXPECT_TRUE(_store.assertWork(1, 4));
    EXPECT_TRUE(assertThreadObserver(4, 1, 0, _writeService));
    delayReuse(8);
    delayReuse({ 9, 10});
    EXPECT_TRUE(_store.assertWork(1, 4));
    EXPECT_TRUE(assertThreadObserver(6, 1, 0, _writeService));
}

}

GTEST_MAIN_RUN_ALL_TESTS()
