// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
#include <exception>
#include <variant>


struct EmptyT {};


template<template<typename> typename Op, typename... Ts>
constexpr bool kNeedTrivial = ((std::is_trivially_copyable_v<Ts> && Op<Ts>::value) && ...);

template<template<typename> typename Op, typename... Ts>
constexpr bool kNeedNonTrivial = ((!std::is_trivially_copyable_v<Ts> && Op<Ts>::value) && ...);

template <bool IsTrivDestr, typename... Ts>
union VariadicUnion;

template <bool IsTrivDestr, typename T>
union VariadicUnion<IsTrivDestr, T> {
  T data_;

  VariadicUnion(std::in_place_index_t<0>, auto&&... args)
    : data_(std::forward<decltype(args)>(args)...) {
  }

  ~VariadicUnion() requires (IsTrivDestr) = default;
  ~VariadicUnion() requires (!IsTrivDestr) {}

  template <typename U>
  constexpr auto &&Get(this auto &&self) {
    if constexpr (std::is_same_v<U, T>) {
      return std::forward_like<decltype(self)>(self.data_);
    } else {
      static_assert(false);
    }
  }
};

template <bool IsTrivDestr, typename T, typename... Ts>
union VariadicUnion<IsTrivDestr, T, Ts...> {
  T data_;
  VariadicUnion<IsTrivDestr, Ts...> other_;

  template<size_t kTInd>
  VariadicUnion(std::in_place_index_t<kTInd>, auto&&... args)
    : other_(std::in_place_index_t<kTInd - 1>{}, std::forward<decltype(args)>(args)...){
  }

  VariadicUnion(std::in_place_index_t<0>, auto&&... args)
    : data_(std::forward<decltype(args)>(args)...) {
  }

  ~VariadicUnion() requires (IsTrivDestr) = default;
  ~VariadicUnion() requires (!IsTrivDestr) {}

  template <typename U>
  constexpr auto &&Get(this auto &&self) {
    static_assert(std::is_same_v<U, std::remove_cvref_t<U>>);
    if constexpr (std::is_same_v<U, T>) {
      return std::forward_like<decltype(self)>(*std::launder(&self.data_));
    } else {
      return self.other_.template Get<U>();
    }
  }
};

template<typename Search, typename... Arr>
consteval size_t FindT() {
  size_t ans;
  size_t cur_ind = 0;
  ([&ans, &cur_ind] <typename T> (std::type_identity<T>) {
    if constexpr (std::is_same_v<Search, T>) {
      ans = cur_ind;
    }
    ++cur_ind;
  } (std::type_identity<Arr>{}), ...);
}

template <typename... Types>
class Variant {
public:
  using index_t = size_t;

private:
  static constexpr index_t npos = static_cast<index_t>(-1);

  template <typename T> static constexpr index_t FindType() {
    index_t ans = npos;
    index_t cur_ind = 0;
    ([&ans, &cur_ind] {
      if constexpr (std::is_same_v<Types, T>) {
        ans = cur_ind;
      }
      ++cur_ind;
    } (), ...);
    return ans;
  }

public:
  template<size_t TInd>
  constexpr Variant(std::in_place_index_t<TInd>, auto&&... args)
    : data_(std::in_place_index<TInd>, std::forward<decltype(args)>(args)...),
      cur_type_ind_(TInd) {}

  template<typename T>
  constexpr Variant(std::type_identity<T>, auto&&... args)
    : Variant(std::in_place_index<FindType<T>()>, std::forward<decltype(args)>(args)...) {}

  constexpr Variant() : Variant(std::in_place_index<sizeof...(Types)>) {}

  constexpr index_t index() const { return cur_type_ind_; }

  template <typename T>
  constexpr auto &&Get(this auto &&self) {
    return self.data_.template Get<T>();
  }

  template<typename T>
  constexpr void Emplace(auto&&... args) {
    Destroy();
    ::new (&data_.template Get<T>()) T(std::forward<decltype(args)>(args)...);
    cur_type_ind_ = FindType<T>();
  }

  constexpr Variant(const Variant &)
    requires (kNeedTrivial<std::is_copy_constructible, Types...>) = default;

  constexpr Variant(const Variant &)
    requires(kNeedNonTrivial<std::is_copy_constructible, Types...>);

  constexpr Variant(Variant &&)
    requires(kNeedTrivial<std::is_move_constructible, Types...>) = default;

  constexpr Variant(Variant &&)
    requires(kNeedNonTrivial<std::is_move_constructible, Types...>);

  constexpr Variant &operator=(const Variant&)
    requires (kNeedTrivial<std::is_copy_assignable, Types...>) = default;

  constexpr Variant &operator=(const Variant &)
    requires(kNeedNonTrivial<std::is_copy_assignable, Types...>);

  constexpr bool ValuelessByException() const noexcept {
    return index() == npos;
  }

  ~Variant()
    requires(!std::is_trivially_destructible_v<Types> || ...) {
    Destroy();
  }

  using __trivially_relocatable _LIBCPP_NODEBUG =
      std::conditional_t<(std::__libcpp_is_trivially_relocatable<Types>::value && ...), Variant, void>;

  ~Variant()
    requires(std::is_trivially_destructible_v<Types> && ...) = default;

private:
  VariadicUnion<(std::is_trivially_destructible_v<Types> && ...), Types..., EmptyT> data_;

  index_t cur_type_ind_ = npos;

  constexpr void Destroy() noexcept((std::is_nothrow_destructible_v<Types> && ...));
};


template<typename T>
constexpr size_t VariantSz = [] <typename... Ts> (std::type_identity<Variant<Ts...>>) { return sizeof...(Ts); } (std::type_identity<T>{});

template<typename... Ts>
consteval std::array<std::array<size_t, sizeof...(Ts)>, (VariantSz<Ts> * ...)>  GetArrOfIndsArrs() {
  std::array<std::array<size_t, sizeof...(Ts)>, (VariantSz<Ts> * ...)> ans;
  static constexpr std::array<size_t, sizeof...(Ts)> szs;
  std::ranges::fill(ans[0], 0);
  for (size_t cur_ind = 1; cur_ind < szs.size(); ++cur_ind) {
    ans[cur_ind] = ans[cur_ind - 1];
    for (size_t i = 0, carry = 1; carry > 0; ++i) {
      size_t buf = ans[cur_ind][i] + carry;
      carry = buf / szs[i];
      ans[cur_ind][i] = buf % szs[i];
    }
  }
  return ans;
}

decltype(auto) Visit(auto &&vis, auto &&...var) {
  if ((var.ValuelessByException() || ...)) {
    throw std::bad_variant_access();
  }
  static constexpr auto arr = GetArrOfIndsArrs<std::remove_cvref_t<decltype(vis)>...>();
  std::array<size_t, sizeof...(var)> inds = { var.index()... };
  return [&inds, &var..., &vis] <size_t... Inds> (std::index_sequence<Inds...>) {
    ([&inds, &var..., &vis] <size_t Ind> (std::integral_constant<size_t, Ind>) {
      if (inds == arr[Ind]) {
        return [&var..., &vis] <size_t... VarInds> (std::index_sequence<VarInds>) {
          return vis(var.template Get<arr[Inds][VarInds]>()...);
        } (std::make_index_sequence<sizeof...(var)>{});
      }
    } (std::integral_constant<size_t, Inds>{}), ...);
  } (std::make_index_sequence<arr.size()>{});
}

template <typename... Types>
constexpr Variant<Types...>::Variant(const Variant &other)
  requires(kNeedNonTrivial<std::is_copy_constructible, Types...>)
  : Variant(std::in_place_index<sizeof...(Types)>)
{
  cur_type_ind_ = npos;
  if (other.ValuelessByException()) return;
  Visit([this] <typename T> (const T &val) noexcept(std::is_nothrow_copy_constructible_v<decltype(val)>) {
    ::new (&data_.template Get<T>()) T(val);
  }, other);
  cur_type_ind_ = other.index();
}

template <typename... Types>
constexpr Variant<Types...>::Variant(Variant &&other)
  requires(kNeedNonTrivial<std::is_move_constructible, Types...>)
  : Variant(std::in_place_index<sizeof...(Types)>)
{
  cur_type_ind_ = npos;
  if (other.ValuelessByException()) return;
  Visit([this] <typename T> (T &&val) noexcept(std::is_nothrow_move_constructible_v<T>) {
    ::new (&data_.template Get<T>()) T(std::forward<decltype(val)>(val));
  }, std::move(other));
  cur_type_ind_ = other.index();
}

template <typename... Types>
constexpr Variant<Types...> &Variant<Types...>::operator=(const Variant &other)
  requires(kNeedNonTrivial<std::is_copy_assignable, Types...>) {
  if (other.ValuelessByException()) {
    Destroy();
    return *this;;
  }
  if (index() == other.index()) {
    Visit([this] <typename T> (const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
      data_.template Get<T>() = val;
    }, other);
  } else {
    Destroy();
    Visit([this] <typename T> (const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
      ::new (&data_.template Get<T>()) T(std::forward<decltype(val)>(val));
    }, other);
    cur_type_ind_ = other.index();
  }
  return *this;
}

template <typename... Types>
constexpr void Variant<Types...>::Destroy() noexcept((std::is_nothrow_destructible_v<Types> && ...)) {
  if (!ValuelessByException()) {
    if constexpr ((!std::is_trivially_destructible_v<Types> || ...)) {
      Visit([] <typename T> (T &val) static noexcept(std::is_nothrow_destructible_v<decltype(val)>) {
        val.~T();
      }, *this);
    }
    cur_type_ind_ = npos;
  }
}
