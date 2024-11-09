// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <tests/common/storage_config_set.h>
#include <tests/common/testhelper.h>
#include <tests/common/teststorageapp.h>
#include <vespa/storage/distributor/statusreporterdelegate.h>
#include <vespa/vespalib/gtest/gtest.h>

namespace storage::distributor {

namespace {

// TODO replace with gmock impl
class MockDelegator : public StatusDelegator {
    mutable std::ostringstream _calls;
    bool handleStatusRequest(const DelegatedStatusRequest& request) const override {
        _calls << "Request(" << request.path << ")";
        return request.reporter.reportStatus(request.outputStream, request.path);
    }
public:
    std::string getCalls() const {
        return _calls.str();
    }
};

class MockStatusReporter : public framework::StatusReporter
{
public:
    MockStatusReporter()
        : framework::StatusReporter("foo", "Bar")
    {}
    std::string getReportContentType(
            const framework::HttpUrlPath&) const override
    {
        return "foo/bar";
    }

    bool reportStatus(std::ostream& os,
                      const framework::HttpUrlPath& path) const override
    {
        os << "reportStatus with " << path;
        return true;
    }
};

}

TEST(StatusReporterDelegateTest, delegate_invokes_delegator_on_status_request) {
    auto config = StorageConfigSet::make_distributor_node_config();
    TestDistributorApp app(config->config_uri());

    MockDelegator mockDelegator;
    MockStatusReporter reporter;

    StatusReporterDelegate delegate(app.getComponentRegister(), mockDelegator, reporter);
    framework::HttpUrlPath path("dummy");
    EXPECT_EQ("foo/bar", delegate.getReportContentType(path));

    std::ostringstream ss;
    ASSERT_TRUE(delegate.reportStatus(ss, path));

    EXPECT_EQ("Request(dummy)", mockDelegator.getCalls());
    EXPECT_EQ("reportStatus with dummy", ss.str());
}

} // storage::distributor
