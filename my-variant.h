// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include "helpers.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <functional>

template<typename... Types>
class Variant {
 public:
    using index_t = unsigned int;

    Variant() = default;

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

    template<typename... Variants>
    friend auto Visit(auto &&vis, Variants&&... vars);

 private:
    alignas(Types...) std::byte storage_[std::max({ (sizeof(Types), ...) })];

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

template<typename... Inds>
auto Ind(const auto &arr, unsigned first_ind, Inds... other_inds) {
    if constexpr (sizeof...(Inds) == 0) {
        return arr[first_ind];
    } else {
        return Ind(arr[first_ind], other_inds...);
    }
}


template<typename... Types>
auto GetWrapperForVariantTypesImpl(Variant<Types...>) {
    return impl::Wrapper<Types...>();
}

template<typename Var>
auto GetWrapperForVariantTypes() {
    return
        [] <typename... Types> (impl::Wrapper<Variant<Types...>>) {
            return impl::Wrapper<Types...>();
        } (impl::Wrapper<std::remove_reference_t<Var>>());
}

template<typename... Variants>
auto Visit(auto &&vis, Variants&&... vars) {
    static constexpr auto func_table =
    [&] <
            typename... TypesOfFirstVariant,
            typename... OtherVariants
        > (impl::Wrapper<Variant<TypesOfFirstVariant...>, OtherVariants...>) {
        return impl::GetFunctionTable<
                    decltype(vis),
                    impl::Wrapper<>,
                    impl::Wrapper<>,
                    impl::Wrapper<TypesOfFirstVariant...>,
                    decltype(GetWrapperForVariantTypes<OtherVariants>())...>();
    } (impl::Wrapper<std::remove_reference_t<Variants>...>());
    return Ind(func_table, vars.Index()...)(vis, vars.storage_...);
}
