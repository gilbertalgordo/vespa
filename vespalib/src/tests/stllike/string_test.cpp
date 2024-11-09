// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/stllike/small_string.h>
#include <vespa/vespalib/stllike/string.h>
#include <algorithm>

using string = vespalib::vespa_string;

TEST("testStringInsert") {
    string s("first string ");
    string a;
    EXPECT_TRUE("first string " == a.insert(0, s));
    EXPECT_EQUAL(string("first first string string "), a.insert(6, s));
    EXPECT_EQUAL(2*s.size(), a.size());
    EXPECT_TRUE(string("first first string string ") == s.insert(6, s));
}

TEST("testStringIterator") {
    string s("abcabccba");
    std::replace(s.begin(), s.end(), 'a','z');
    EXPECT_TRUE(s == "zbczbccbz");
}

TEST("test iterator assignment") {
    std::vector<char> empty;
    string s(empty.begin(), empty.end());
    EXPECT_TRUE(strstr(s.c_str(), "mumbo jumbo.") == nullptr);
}

namespace {

template <typename S>
void assign(S &lhs, const S &rhs) __attribute__((noinline));

template <typename S>
void
assign(S &lhs, const S &rhs)
{
    lhs = rhs;
}

}


TEST("test self assignment of small string") {
    const char * text = "abc";
    string s(text);
    const char * addr(reinterpret_cast<const char *>(&s));
    EXPECT_TRUE((addr < s.c_str()) && (s.c_str() < addr + sizeof(s)));
    assign(s, s);
    EXPECT_EQUAL(text, s);
}

TEST("test self assignment of big string") {
    const char * text = "abcbcdefghijklmnopqrstuvwxyz-abcbcdefghijklmnopqrstuvwxyz";
    string s(text);
    const char * addr(reinterpret_cast<const char *>(&s));
    EXPECT_TRUE((addr > s.c_str()) || (s.c_str() > addr + sizeof(s)));
    assign(s, s);
    EXPECT_EQUAL(text, s);
}

void verify_move_constructor(string org) {
    string copy(org);
    EXPECT_EQUAL(org, copy);
    string moved_into(std::move(copy));
    EXPECT_EQUAL(org, moved_into);
    EXPECT_NOT_EQUAL(org, copy);
    EXPECT_EQUAL(string(), copy);
}

void verify_move_operator(string org) {
    string copy(org);
    EXPECT_EQUAL(org, copy);
    string moved_into_short("short movable string");
    EXPECT_LESS(moved_into_short.size(), string().capacity());
    EXPECT_NOT_EQUAL(org, moved_into_short);
    moved_into_short = std::move(copy);
    EXPECT_EQUAL(org, moved_into_short);
    EXPECT_NOT_EQUAL(org, copy);
    EXPECT_EQUAL(string(), copy);

    string moved_into_long("longer movable string than the 47 bytes that can be held in the short string optimization.");
    EXPECT_GREATER(moved_into_long.size(), string().capacity());
    EXPECT_NOT_EQUAL(org, moved_into_long);
    moved_into_long = std::move(moved_into_short);
    EXPECT_EQUAL(org, moved_into_long);
    EXPECT_NOT_EQUAL(org, moved_into_short);
    EXPECT_EQUAL(string(), moved_into_short);
}

void verify_move(string org) {
    verify_move_constructor(org);
    verify_move_operator(org);
}

TEST("test move constructor") {
    TEST_DO(verify_move("short string"));
    TEST_DO(verify_move("longer string than the 47 bytes that can be held in the short string optimization."));
}

TEST("testStringCompare") {
    fprintf(stderr, "... testing comparison\n");
    string abc("abc");
    string abb("abb");
    string abd("abd");

    string a5("abcde");

    std::string other("abc");

    EXPECT_TRUE(abc == "abc");
    EXPECT_TRUE(abc == other);
    EXPECT_TRUE(!(abc == "aaa"));
    EXPECT_TRUE(!(abc == "a"));
    EXPECT_TRUE(!(abc == "abcde"));
    EXPECT_TRUE(!(abc == abb));
    EXPECT_TRUE(!(abc == a5));

    EXPECT_TRUE(abc != abd);
    EXPECT_TRUE(abc != "aaa");
    EXPECT_TRUE(abc != "a");
    EXPECT_TRUE(abc != a5);
    EXPECT_TRUE(!(abc != abc));
    EXPECT_TRUE(!(abc != other));

    EXPECT_TRUE(abc < abd);
    EXPECT_TRUE(abb < abc);
    EXPECT_TRUE(abc < a5);
    EXPECT_TRUE(abc.compare(abd) < 0);
    EXPECT_TRUE(abd.compare(abc) > 0);
    EXPECT_TRUE(abc.compare(abc) == 0);

    TEST_FLUSH();
}

TEST("testString") {
    fprintf(stderr, "... testing basic functionality\n");
    string a;
    EXPECT_EQUAL(sizeof(a), 48 + sizeof(uint32_t)*2 + sizeof(char *));
    EXPECT_EQUAL(0u, a.size());
    a.append("a");
    EXPECT_EQUAL(1u, a.size());
    EXPECT_TRUE(strcmp("a", a.c_str()) == 0);
    a.append("b");
    EXPECT_EQUAL(2u, a.size());
    EXPECT_TRUE(strcmp("ab", a.c_str()) == 0);
    string b(a);
    EXPECT_EQUAL(2u, a.size());
    EXPECT_TRUE(strcmp("ab", a.c_str()) == 0);
    EXPECT_EQUAL(2u, b.size());
    EXPECT_TRUE(strcmp("ab", b.c_str()) == 0);
    string c("dfajsg");
    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);
    b = c;
    EXPECT_EQUAL(6u, b.size());
    EXPECT_TRUE(strcmp("dfajsg", b.c_str()) == 0);

    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);

    TEST_FLUSH();

    std::string::size_type exp = std::string::npos;
    std::string::size_type act = std::string::npos;
    EXPECT_EQUAL(exp, act);
    std::string::size_type idx = a.find('a');
    EXPECT_EQUAL(0u, idx);
    idx = a.find('b');
    EXPECT_EQUAL(1u, idx);
    idx = a.find('x');
    EXPECT_EQUAL(std::string::npos, idx);
    EXPECT_EQUAL(1u, a.find('b', 1));
    EXPECT_EQUAL(std::string::npos, a.find('b', 2));
    // causes warning:
    EXPECT_TRUE(std::string::npos == idx);

    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);

    TEST_FLUSH();

    string slow;
    for (int i = 0; i < 9; i++) {
        EXPECT_EQUAL(i*5u, slow.size());
        slow.append("abcde");
        EXPECT_EQUAL(sizeof(slow) - 17u, slow.capacity());
    }

    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);

    EXPECT_EQUAL(45u, slow.size());
    EXPECT_EQUAL(47u, slow.capacity());
    slow.append("1");
    EXPECT_EQUAL(46u, slow.size());
    slow.append("1");
    EXPECT_EQUAL(47u, slow.size());
    EXPECT_EQUAL(47u, slow.capacity());
    slow.append("1");
    EXPECT_EQUAL(48u, slow.size());
    EXPECT_EQUAL(63u, slow.capacity());

    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);


    string fast;
    fast.append(slow);

    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);

    EXPECT_EQUAL(48u, fast.size());
    EXPECT_EQUAL(63u, fast.capacity());
    fast.append(slow);

    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);

    EXPECT_EQUAL(48u*2, fast.size());
    EXPECT_EQUAL(127u, fast.capacity());
    fast.append(slow);

    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);

    EXPECT_EQUAL(48u*3, fast.size());
    EXPECT_EQUAL(255u, fast.capacity());
    fast.append(slow);
    EXPECT_EQUAL(48u*4, fast.size());
    EXPECT_EQUAL(255u, fast.capacity());

    EXPECT_EQUAL(6u, c.size());
    EXPECT_TRUE(strcmp("dfajsg", c.c_str()) == 0);

    std::istringstream is("test streng");
    string test, streng;
    is >> test >> streng;
    EXPECT_EQUAL(test, "test");
    EXPECT_EQUAL(streng, "streng");
    std::ostringstream os;
    os << test << streng;
    EXPECT_EQUAL(os.str(), "teststreng");

    {
        string s("abcabca");
        EXPECT_EQUAL(string::npos, s.find('g'));
        EXPECT_EQUAL(string::npos, s.rfind('g'));
        EXPECT_EQUAL(0u, s.find('a'));
        EXPECT_EQUAL(6u, s.rfind('a'));
        EXPECT_EQUAL(1u, s.find('b'));
        EXPECT_EQUAL(4u, s.rfind('b'));
        EXPECT_EQUAL(2u, s.find("ca"));
        EXPECT_EQUAL(5u, s.rfind("ca"));
        EXPECT_EQUAL(0u, s.find("ab"));
        EXPECT_EQUAL(3u, s.rfind("ab"));
    }
    {
        std::string_view s("abcabca");
        EXPECT_EQUAL(string::npos, s.find('g'));
        EXPECT_EQUAL(string::npos, s.rfind('g'));
        EXPECT_EQUAL(0u, s.find('a'));
        EXPECT_EQUAL(6u, s.rfind('a'));
        EXPECT_EQUAL(1u, s.find('b'));
        EXPECT_EQUAL(4u, s.rfind('b'));
        EXPECT_EQUAL(2u, s.find("ca"));
        EXPECT_EQUAL(5u, s.rfind("ca"));
        EXPECT_EQUAL(0u, s.find("ab"));
        EXPECT_EQUAL(3u, s.rfind("ab"));
        std::string_view s2("abc");
        EXPECT_EQUAL(2u, s2.rfind('c'));
        EXPECT_EQUAL(1u, s2.rfind('b'));
        EXPECT_EQUAL(0u, s2.rfind('a'));
        EXPECT_EQUAL(string::npos, s2.rfind('d'));
    }

    EXPECT_EQUAL("a" + string("b"), string("ab"));
    EXPECT_EQUAL(string("a") + string("b"), string("ab"));
    EXPECT_EQUAL(string("a") + std::string_view("b"), string("ab"));
    EXPECT_EQUAL(string("a") + "b", string("ab"));

    // Test std::string conversion of empty string
    std::string_view sref;
    std::string stdString(sref);
    EXPECT_EQUAL(nullptr, sref.data());
    EXPECT_TRUE(strcmp("", stdString.data()) == 0);
    stdString = "abc";
    std::string_view sref2(stdString);
    EXPECT_TRUE(stdString.c_str() == sref2.data());
    EXPECT_TRUE(stdString == sref2);
    EXPECT_TRUE(sref2 == stdString);
    {
        string s;
        s = std::string("cba");
        EXPECT_TRUE("cba" == s);
        s = sref2;
        EXPECT_TRUE("abc" == s);
        string s2;
        s2.swap(s);
        EXPECT_TRUE(s.empty());
        EXPECT_TRUE("abc" == s2);
    }
    {
        EXPECT_EQUAL(string("abc"), string("abcd", 3));
        EXPECT_EQUAL(string("abc"), string(std::string_view("abc")));
    }
    {
        string s("abc");
        EXPECT_EQUAL(string("a"), s.substr(0,1));
        EXPECT_EQUAL(string("b"), s.substr(1,1));
        EXPECT_EQUAL(string("c"), s.substr(2,1));
        EXPECT_EQUAL(string("abc"), s.substr(0));
        EXPECT_EQUAL(string("bc"), s.substr(1));
        EXPECT_EQUAL(string("c"), s.substr(2));
    }
    {
        std::string_view s("abc");
        EXPECT_EQUAL(string("a"), s.substr(0,1));
        EXPECT_EQUAL(string("b"), s.substr(1,1));
        EXPECT_EQUAL(string("c"), s.substr(2,1));
        EXPECT_EQUAL(string("abc"), s.substr(0));
        EXPECT_EQUAL(string("bc"), s.substr(1));
        EXPECT_EQUAL(string("c"), s.substr(2));
    }

    {
        string s(" A very long string that is longer than what fits on the stack so that it will be initialized directly on the heap");
        EXPECT_TRUE( ! s.empty());
        EXPECT_TRUE(s.length() > sizeof(s));
    }

    TEST_FLUSH();
}

TEST("require that vespalib::resize works") {
    string s("abcdefghijk");
    EXPECT_EQUAL(11u, s.size());
    s.resize(5);
    EXPECT_EQUAL(5u, s.size());
    EXPECT_EQUAL("abcde", s);
    s.resize(7, 'X');
    EXPECT_EQUAL(7u, s.size());
    EXPECT_EQUAL("abcdeXX", s);
    EXPECT_EQUAL(47u, s.capacity());
    s.resize(50, 'Y');
    EXPECT_EQUAL(50u, s.size());
    EXPECT_EQUAL("abcdeXXYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY", s);
}

TEST("require that you can format a number into a std::string easily") {
    std::string str = vespalib::stringify(0);
    EXPECT_EQUAL(str, "0");
    EXPECT_EQUAL(vespalib::stringify(1), "1");
    EXPECT_EQUAL(vespalib::stringify(123), "123");
    EXPECT_EQUAL(vespalib::stringify(123456789), "123456789");
    EXPECT_EQUAL(vespalib::stringify(987654321uLL), "987654321");
    EXPECT_EQUAL(vespalib::stringify(18446744073709551615uLL), "18446744073709551615");
}

TEST("require that contains works") {
    std::string s("require that contains works");
    EXPECT_TRUE(s.contains("require"));
    EXPECT_TRUE(s.contains("require that contains work"));
    EXPECT_TRUE(s.contains("require that contains works"));
    EXPECT_TRUE(s.contains("equire"));
    EXPECT_TRUE(s.contains("ks"));
    EXPECT_FALSE(s.contains("not in there"));
}

TEST("require that starts_with works") {
    std::string s("require that starts_with works");
    EXPECT_TRUE(s.starts_with("require"));
    EXPECT_TRUE(s.starts_with("require that starts_with work"));
    EXPECT_TRUE(s.starts_with("require that starts_with works"));
    EXPECT_FALSE(s.starts_with("equire"));
    EXPECT_FALSE(s.starts_with("not in there"));
}

TEST("require that ends_with works") {
    std::string s("require that ends_with works");
    EXPECT_FALSE(s.ends_with("require"));
    EXPECT_TRUE(s.ends_with("works"));
    EXPECT_TRUE(s.ends_with("equire that ends_with works"));
    EXPECT_TRUE(s.ends_with("require that ends_with works"));
    EXPECT_FALSE(s.ends_with("work"));
    EXPECT_FALSE(s.ends_with("not in there"));
}

TEST("test that small_string::pop_back works") {
    std::string s("string");
    EXPECT_EQUAL(s.size(), 6u);
    s.pop_back();
    EXPECT_EQUAL(s.size(), 5u);
    EXPECT_EQUAL(s, string("strin"));
    EXPECT_NOT_EQUAL(s, string("string"));
    s.pop_back();
    EXPECT_EQUAL(s, string("stri"));
}


TEST("test that operator<() works with std::string_view versus string") {
    std::string_view sra("a");
    std::string sa("a");
    std::string_view srb("b");
    std::string sb("b");
    EXPECT_FALSE(sra < sra);
    EXPECT_FALSE(sra < sa);
    EXPECT_TRUE(sra < srb);
    EXPECT_TRUE(sra < sb);
    EXPECT_FALSE(sa < sra);
    EXPECT_FALSE(sa < sa);
    EXPECT_TRUE(sa < srb);
    EXPECT_TRUE(sa < sb);
    EXPECT_FALSE(srb < sra);
    EXPECT_FALSE(srb < sa);
    EXPECT_FALSE(srb < srb);
    EXPECT_FALSE(srb < sb);
    EXPECT_FALSE(sb < sra);
    EXPECT_FALSE(sb < sa);
    EXPECT_FALSE(sb < srb);
    EXPECT_FALSE(sb < sb);
}

TEST("test that empty_string is shared and empty") {
    EXPECT_TRUE(&vespalib::empty_string() == &vespalib::empty_string());
    EXPECT_EQUAL(vespalib::empty_string(), "");
}

TEST("starts_with has expected semantics for small_string") {
    std::string a("foobar");
    EXPECT_TRUE(a.starts_with(""));
    EXPECT_TRUE(a.starts_with("foo"));
    EXPECT_TRUE(a.starts_with("foobar"));
    EXPECT_FALSE(a.starts_with("foobarf"));
    EXPECT_FALSE(a.starts_with("oobar"));
}

TEST("starts_with has expected semantics for std::string_view") {
    std::string a("foobar");
    std::string_view ar(a);
    EXPECT_TRUE(ar.starts_with(""));
    EXPECT_TRUE(ar.starts_with("foo"));
    EXPECT_TRUE(ar.starts_with("foobar"));
    EXPECT_FALSE(ar.starts_with("foobarf"));
    EXPECT_FALSE(ar.starts_with("oobar"));
}

TEST("test allowed nullptr construction - legal, but not legitimate use.") {
    EXPECT_TRUE(std::string(nullptr, 0).empty());
    EXPECT_TRUE(std::string(nullptr, 0).empty());
    EXPECT_TRUE(std::string_view(nullptr, 0).empty());
}

TEST_MAIN() { TEST_RUN_ALL(); }
