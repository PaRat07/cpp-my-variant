// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace impl {
template<size_t ind, typename... Types>
struct IthType;

template<typename First, typename... Other>
struct IthType<0, First, Other...> {
    using type = First;
};

template<size_t ind, typename First, typename... Other>
struct IthType<ind, First, Other...> {
    using type = typename IthType<ind - 1, Other...>::type;
};
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

    index_t index() const {
        return cur_type_ind_;
    }
    
    void visit(const auto &f) {
        static constexpr auto apply =
        [] <size_t... Inds> (std::index_sequence<Inds...>, const auto &f, std::byte *storage, index_t cur_ind) {
            ([] <size_t ind> (std::index_sequence<ind>, const auto &f, std::byte *storage, index_t cur_ind) {
                if (ind == cur_ind) {
                    f(reinterpret_cast<impl::IthType<ind, Types...>::type&>(*storage));
                }
            } (std::index_sequence<Inds>(), f, storage, cur_ind), ...);
        };
        if (cur_type_ind_ == npos) {
            throw std::logic_error("visit for empty variant");
        }
        apply(std::make_index_sequence<sizeof...(Types)>(), f, storage_, cur_type_ind_);
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
                if constexpr (std::is_same_v<std::decay_t<ToSearch>, std::decay_t<typename impl::IthType<Ind, Types...>::type>>) {
                    ans = Ind;
                }
            } (std::index_sequence<Inds>()), ...);
        } (std::make_index_sequence<sizeof...(Types)>());
        return ans;
    }

    static constexpr auto GetVtable() {
        std::array<
    }
};
