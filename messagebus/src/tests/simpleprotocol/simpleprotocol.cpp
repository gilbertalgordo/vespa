// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/messagebus/testlib/simpleprotocol.h>
#include <vespa/messagebus/testlib/simplemessage.h>
#include <vespa/messagebus/testlib/simplereply.h>
#include <vespa/messagebus/testlib/slobrok.h>
#include <vespa/messagebus/testlib/testserver.h>
#include <vespa/messagebus/ireplyhandler.h>
#include <vespa/messagebus/routing/routingcontext.h>
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/component/vtag.h>

using namespace mbus;

TEST("simpleprotocol_test") {

    vespalib::Version version = vespalib::Vtag::currentVersion;
    SimpleProtocol protocol;
    EXPECT_TRUE(protocol.getName() == "Simple");

    EXPECT_EQUAL(24u + 2 *sizeof(std::string), sizeof(Result));
    EXPECT_EQUAL(8u + 2 *sizeof(std::string), sizeof(Error));
    EXPECT_EQUAL(56u, sizeof(Routable));
    {
        // test protocol
        IRoutingPolicy::UP bogus = protocol.createPolicy("bogus", "");
        EXPECT_FALSE(bogus);
    }
    TEST_FLUSH();
    {
        // test SimpleMessage
        EXPECT_EQUAL(104u, sizeof(Message));
        EXPECT_EQUAL(120u + sizeof(std::string), sizeof(SimpleMessage));
        auto msg = std::make_unique<SimpleMessage>("test");
        EXPECT_TRUE(!msg->isReply());
        EXPECT_TRUE(msg->getProtocol() == SimpleProtocol::NAME);
        EXPECT_TRUE(msg->getType() == SimpleProtocol::MESSAGE);
        EXPECT_TRUE(static_cast<SimpleMessage&>(*msg).getValue() == "test");
        Blob b = protocol.encode(version, *msg);
        EXPECT_TRUE(b.size() > 0);
        Routable::UP tmp = protocol.decode(version, BlobRef(b));
        ASSERT_TRUE(tmp);
        EXPECT_TRUE(!tmp->isReply());
        EXPECT_TRUE(tmp->getProtocol() == SimpleProtocol::NAME);
        EXPECT_TRUE(tmp->getType() == SimpleProtocol::MESSAGE);
        EXPECT_TRUE(static_cast<SimpleMessage&>(*tmp).getValue() == "test");
    }
    TEST_FLUSH();
    {
        // test SimpleReply
        EXPECT_EQUAL(96u, sizeof(Reply));
        EXPECT_EQUAL(96u + sizeof(std::string), sizeof(SimpleReply));
        auto reply = std::make_unique<SimpleReply>("reply");
        EXPECT_TRUE(reply->isReply());
        EXPECT_TRUE(reply->getProtocol() == SimpleProtocol::NAME);
        EXPECT_TRUE(reply->getType() == SimpleProtocol::REPLY);
        EXPECT_TRUE(static_cast<SimpleReply&>(*reply).getValue() == "reply");
        Blob b = protocol.encode(version, *reply);
        EXPECT_TRUE(b.size() > 0);
        Routable::UP tmp = protocol.decode(version, BlobRef(b));
        ASSERT_TRUE(tmp);
        EXPECT_TRUE(tmp->isReply());
        EXPECT_TRUE(tmp->getProtocol() == SimpleProtocol::NAME);
        EXPECT_TRUE(tmp->getType() == SimpleProtocol::REPLY);
        EXPECT_TRUE(static_cast<SimpleReply&>(*tmp).getValue() == "reply");
    }
}

TEST_MAIN() { TEST_RUN_ALL(); }
