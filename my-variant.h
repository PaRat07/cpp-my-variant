// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once


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

    template<typename T>
    T &Get() {
        return reinterpret_cast<T&>(*storage_);
    }

    index_t Index() const {
        return cur_type_ind_;
    }

 private:
    alignas(Types...) std::byte storage_[std::max({ sizeof(Types)... })];

    static constexpr index_t npos = -1;
    index_t cur_type_ind_ = npos;

    template<typename ToSearch>
    static constexpr auto FindType() {
        index_t ans = npos;
        index_t cur_index = 0;
        ((ans = std::is_same_v<ToSearch, Types> ? cur_index++ : ans), ...);
        return ans;
    }
};
