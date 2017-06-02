/**
 * Contains msgpack adaptor implementation for boost::variant.
 *
 * Works for any number of template argument types of any msgpack serializable types.
 * @author Chen Weiguang
 */

#pragma once

#include "msgpack.hpp"

#include "boost/variant.hpp"

#include <array>
#include <cstddef>
#include <functional>

namespace details {
    
    // packer section

    template <class Stream, class Var>
    using packer_fn = std::function<void(msgpack::packer<Stream> &, const Var &)>;

    template <class Stream, class Var, size_t N>
    using packer_fn_arr = std::array<packer_fn<Stream, Var>, N>;

    template <class Stream, class Var, size_t N>
    void create_packers_impl(packer_fn_arr<Stream, Var, N> &) {
        // base case
    }

    template <class Stream, class Var, size_t N, class T, class... Ts>
    void create_packers_impl(packer_fn_arr<Stream, Var, N> &arr) {
        static constexpr auto Index = N - sizeof...(Ts) - 1;

        arr[Index] = [](msgpack::packer<Stream> &p, const Var &v) {
            p.pack(boost::get<T>(v));
        };

        create_packers_impl<Stream, Var, N, Ts...>(arr);
    }

    /**
     * Creates the runtime array of pack functions to call based on the type index.
     * @tparam Stream stream type required by adaptor pack
     * @tparam N number of elements in variant type
     * @tparam T current msgpack serializable template argument
     * @tparam Ts other msgpack serializable template arguments
     * @return runtime array of pack functions in template argument order
     */
    template <class Stream, size_t N, class T, class... Ts>
    auto create_packers() -> packer_fn_arr<Stream, boost::variant<T, Ts...>, N> {
        using Var = boost::variant<T, Ts...>;

        packer_fn_arr<Stream, Var, N> arr;
        create_packers_impl<Stream, Var, N, T, Ts...>(arr);
        return arr;
    }

    // unpacker section

    template <class Var>
    using unpacker_fn = std::function<void(const msgpack::object &, Var &)>;

    template <class Var, size_t N>
    using unpacker_fn_arr = std::array<unpacker_fn<Var>, N>;

    template <class Var, size_t N>
    void create_unpackers_impl(unpacker_fn_arr<Var, N> &) {
        // base case
    }

    template <class Var, size_t N, class T, class... Ts>
    void create_unpackers_impl(unpacker_fn_arr<Var, N> &arr) {
        static constexpr auto Index = N - sizeof...(Ts) - 1;

        arr[Index] = [](const msgpack::object &o, Var &v) {
            T item;
            o.convert(item);
            v = item;
        };

        create_unpackers_impl<Var, N, Ts...>(arr);
    }

    /**
     * Creates the runtime array of unpack functions to call based on the type index.
     * @tparam N number of elements in variant type
     * @tparam T current msgpack serializable template argument
     * @tparam Ts other msgpack serializable template arguments
     * @return runtime array of unpack functions in template argument order
     */
    template <size_t N, class T, class... Ts>
    auto create_unpackers() -> unpacker_fn_arr<boost::variant<T, Ts...>, N> {
        using Var = boost::variant<T, Ts...>;

        unpacker_fn_arr<Var, N> arr;
        create_unpackers_impl<Var, N, T, Ts...>(arr);
        return arr;
    }
}

namespace msgpack {
    MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
        namespace adaptor {
            /**
             * Contains the pack implementation for boost::variant.
             * @tparam Ts msgpack serializable template arguments
             */
            template <class... Ts>
            struct pack<boost::variant<Ts...>> {
                template <class Stream>
                auto operator()(
                    msgpack::packer<Stream> &p,
                    const boost::variant<Ts...> &v) const
                    -> msgpack::packer<Stream> & {

                    static constexpr auto N = sizeof...(Ts);
                    static const auto PACKERS = details::create_packers<Stream, N, Ts...>();

                    p.pack_array(2);

                    const auto type_index = v.which();
                    p.pack(type_index);

                    // need to dynamically pack the correct value type
                    PACKERS.at(type_index)(p, v);

                    return p;
                }
            };

            /**
             * Contains the convert implementation for boost::variant.
             * @tparam Ts msgpack serializable template arguments
             */
            template <class... Ts>
            struct convert<boost::variant<Ts...>> {
                auto operator()(
                    const msgpack::object &o,
                    boost::variant<Ts...> &v) const
                    -> const msgpack::object & {

                    static constexpr auto N = sizeof...(Ts);
                    static const auto UNPACKERS = details::create_unpackers<N, Ts...>();

                    if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
                    if (o.via.array.size != 2) throw msgpack::type_error();

                    const auto type_index = o.via.array.ptr[0].as<int>();

                    // need to dynamically unpack the correct value type back
                    UNPACKERS.at(type_index)(o.via.array.ptr[1], v);

                    return o;
                }
            };
        }
    }
}
