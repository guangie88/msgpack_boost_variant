#include "gtest/gtest.h"

#include "msgpack_boost_variant.hpp"

#include "msgpack.hpp"

#include "boost/variant.hpp"

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// boost
using boost::variant;

// std
using std::pair;
using std::string;
using std::stringstream;
using std::unordered_map;
using std::vector;

template <class T>
auto pack(const T &v) -> std::string {
    stringstream ss;
    msgpack::pack(ss, v);
    return ss.str();
}

template <class T>
auto unpack(const std::string &str) -> T {
    auto handle = msgpack::unpack(str.data(), str.size());
    auto deserialized = handle.get();

    T v;
    deserialized.convert(v);
    return std::move(v);
}

TEST(MsgpackBoostVar, Simple) {
    using var_t = variant<int, double, bool>;

    // v1 -> v2 -> v3 -> v1
    var_t v1 = 3.14;
    var_t v2 = 7;
    var_t v3 = true;

    EXPECT_EQ(3.14, boost::get<double>(v1));
    EXPECT_EQ(7, boost::get<int>(v2));
    EXPECT_EQ(true, boost::get<bool>(v3));

    const auto v1_packed_str = pack(v1);
    const auto v2_packed_str = pack(v2);
    const auto v3_packed_str = pack(v3);

    v2 = unpack<var_t>(v1_packed_str);
    v3 = unpack<var_t>(v2_packed_str);
    v1 = unpack<var_t>(v3_packed_str);

    EXPECT_EQ(3.14, boost::get<double>(v2));
    EXPECT_EQ(7, boost::get<int>(v3));
    EXPECT_EQ(true, boost::get<bool>(v1));
}

int main(int argc, char * argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
