// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <functional>

namespace impl {
template<size_t ind, typename... Types>
struct IthTypeImpl;

template<typename First, typename... Other>
struct IthTypeImpl<0, First, Other...> {
    using type = First;
};

template<size_t ind, typename First, typename... Other>
struct IthTypeImpl<ind, First, Other...> {
    using type = typename IthTypeImpl<ind - 1, Other...>::type;
};

template<size_t ind, typename... Types>
using IthType = typename IthTypeImpl<ind, Types...>::value;


template<typename Func, typename... Types>
struct AllInvokeResultSameImpl;

template<typename Func, typename Type>
struct AllInvokeResultSameImpl<Func, Type> {
    static inline constexpr bool value = true;
};

template<typename Func, typename First, typename Second, typename... Types>
struct AllInvokeResultSameImpl<Func, First, Second, Types...> {
    static inline constexpr bool value = std::is_same_v<std::invoke_result_t<Func, First>, std::invoke_result_t<Func, Second>> && AllInvokeResultSameImpl<Func, Second, Types...>::value;
};

template<typename Func, typename... Types>
constexpr bool AllInvokeResultSame = AllInvokeResultSameImpl<Func, Types...>::value;


template<typename Func, typename... Types>
auto GetFunctionTable() {
    static_assert(AllInvokeResultSame<Func, Types...>, "Variant::Visit requires the visitor to have the same return type for all alternatives of a variant");
    std::array<std::function<std::invoke_result_t<Func, typename IthTypeImpl<0, Types...>::type>(Func, std::byte*)>, sizeof...(Types)> ans;
    [&ans] <size_t... Inds> (std::index_sequence<Inds...>) {
        ([&ans] <size_t ind> (std::index_sequence<ind>) {
            ans[ind] = [] (const Func &to_apply, std::byte *arg) {
                return to_apply(reinterpret_cast<IthTypeImpl<ind, Types...>&>(*arg));
            };
        } (std::index_sequence<Inds>()), ...);
    } (std::make_index_sequence<sizeof...(Types)>());
    return ans;
}
} // namespace impl


//template<typename...

template<typename... Types>
class Variant {
 public:
    using index_t = unsigned int;
    template<typename T>
    Variant(T&& value) {
        new (storage_) T(std::forward<T>(value));
        cur_type_ind_ = FindType<T>();
    }

    index_t Index() const {
        return cur_type_ind_;
    }
    
    auto Visit(const auto &f) {
        static const auto funtion_table = impl::GetFunctionTable<decltype(f), Types...>();
        if (cur_type_ind_ == npos) {
            throw std::logic_error("Visit for empty variant");
        }
        return funtion_table[cur_type_ind_](f, storage_);
    }



 private:
    alignas(Types...) std::byte storage_[std::max({(sizeof(Types), ...)})];

    static constexpr index_t npos = -1;
    index_t cur_type_ind_ = npos;

    template<typename ToSearch>
    static constexpr auto FindType() {
        index_t ans = npos;
        [&ans] <size_t... Inds> (std::index_sequence<Inds...>) {
            ([&ans] <size_t Ind> (std::index_sequence<Ind>) {
                if constexpr (std::is_same_v<std::decay_t<ToSearch>, std::decay_t<typename impl::IthTypeImpl<Ind, Types...>::type>>) {
                    ans = Ind;
                }
            } (std::index_sequence<Inds>()), ...);
        } (std::make_index_sequence<sizeof...(Types)>());
        return ans;
    }
};
