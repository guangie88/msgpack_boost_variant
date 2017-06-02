#include "gtest/gtest.h"

#include "msgpack_boost_variant.hpp"

#include "msgpack.hpp"

#include "boost/variant.hpp"

#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// boost
using boost::variant;

// std
using std::pair;
using std::string;
using std::stringstream;
using std::unordered_map;
using std::unordered_set;
using std::vector;

enum class status {
    inprogress, completed, error,
};

MSGPACK_ADD_ENUM(status);

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
    using var_t = variant<double, int, bool>;

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

TEST(MsgpackBoostVar, Complex) {
    using pair_t = pair<string, status>;
    using vec_t = vector<status>;
    using map_t = unordered_map<string, unordered_set<status>>;

    using var_t = variant<pair_t, vec_t, map_t>;

    // v1 -> v2 -> v3 -> v1
    var_t v1 = pair_t("Hello World", status::completed);
    var_t v2 = vec_t{status::inprogress, status::completed, status::error};
    var_t v3 = map_t{{"one", {status::error, status::inprogress}}, {"two", {status::completed, status::error}}};

    const auto pair_cmp = [](const pair_t &p) {
        return
            p.first == "Hello World" &&
            p.second == status::completed;
    };

    const auto vec_cmp = [](const vec_t &v) {
        return
            v.at(0) == status::inprogress &&
            v.at(1) == status::completed &&
            v.at(2) == status::error;
    };

    const auto map_cmp = [](const map_t &m) {
        const auto &ones = m.at("one");
        const auto &twos = m.at("two");

        const auto exists = [](const unordered_set<status> &s, const status rhs) {
            return s.find(rhs) != s.cend();
        };

        return
            exists(ones, status::error) && exists(ones, status::inprogress) && !exists(ones, status::completed) &&
            exists(twos, status::completed) && exists(twos, status::error) && !exists(twos, status::inprogress);
    };

    EXPECT_TRUE(pair_cmp(boost::get<pair_t>(v1)));
    EXPECT_TRUE(vec_cmp(boost::get<vec_t>(v2)));
    EXPECT_TRUE(map_cmp(boost::get<map_t>(v3)));

    const auto v1_packed_str = pack(v1);
    const auto v2_packed_str = pack(v2);
    const auto v3_packed_str = pack(v3);

    v2 = unpack<var_t>(v1_packed_str);
    v3 = unpack<var_t>(v2_packed_str);
    v1 = unpack<var_t>(v3_packed_str);

    EXPECT_TRUE(pair_cmp(boost::get<pair_t>(v2)));
    EXPECT_TRUE(vec_cmp(boost::get<vec_t>(v3)));
    EXPECT_TRUE(map_cmp(boost::get<map_t>(v1)));
}

int main(int argc, char * argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
