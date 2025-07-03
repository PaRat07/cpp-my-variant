// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#pragma once

#include "my-variant.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>
#include <variant>

#define fwd(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
// #define PRINT_TYPE(...) static_assert((std::type_identity<decltype(__VA_ARGS__)>{}, false))
namespace my {
template <bool IsTrivDestr, typename... Ts>
union VariadicUnion;

template <bool IsTrivDestr, typename T>
union VariadicUnion<IsTrivDestr, T> {
  T data_;

  constexpr VariadicUnion(std::in_place_index_t<0>, auto&&... args)
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
  constexpr VariadicUnion(std::in_place_index_t<kTInd>, auto&&... args)
    : other_(std::in_place_index_t<kTInd - 1>{}, std::forward<decltype(args)>(args)...){
  }

  constexpr VariadicUnion(std::in_place_index_t<0>, auto&&... args)
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
      return fwd(self).other_.template Get<U>();
    }
  }
};

template<size_t Ind, typename... Ts>
struct IthType;

template<typename Need, typename... Other>
struct IthType<0, Need, Other...> : std::type_identity<Need> {};

template<size_t Ind, typename Sep, typename... Other>
struct IthType<Ind, Sep, Other...> : IthType<Ind - 1, Other...> {};


template<typename T, template<typename...> typename Templ>
constexpr bool IsInstanceOf = std::is_invocable_v<decltype([] <class... Ts> (std::type_identity<Templ<Ts...>>){}), std::type_identity<std::remove_cvref_t<T>>>;


template<typename T, template<auto...> typename Templ>
constexpr bool IsInstanceOfWithVals = std::is_invocable_v<decltype([] <auto... Ts> (std::type_identity<Templ<Ts...>>){}), std::type_identity<std::remove_cvref_t<T>>>;

template <typename... Types>
class Variant {
public:
  using index_t = size_t;

  static constexpr index_t npos = static_cast<index_t>(-1);

private:
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


  template<size_t TInd>
  using IthT = typename IthType<TInd, Types...>::type;

  template<typename T>
  static constexpr index_t kInitInd = [] <size_t... Inds> (std::index_sequence<Inds...>) {
    if ((std::is_constructible_v<Types, T> + ...) != 1) {
      return npos;
    }
    index_t ans = npos;
    ([&ans] <size_t Ind> (std::integral_constant<size_t, Ind>) {
      ++ans;
      return std::is_constructible_v<IthT<Ind>, T&&>;
    } (std::integral_constant<size_t, Inds>{}) ||  ...);
    return ans;
  } (std::make_index_sequence<sizeof...(Types)>{});

public:
  template<template<typename> typename Trait>
  static constexpr bool kIsAll = (Trait<Types>::value && ...);

  template<size_t TInd>
  constexpr Variant(std::in_place_index_t<TInd>, auto&&... args) noexcept(std::is_nothrow_constructible_v<IthT<TInd>, decltype(args)...>)
    : data_(std::in_place_index<TInd>, std::forward<decltype(args)>(args)...),
      cur_type_ind_(TInd) {}

  template<typename T>
  constexpr Variant(std::in_place_type_t<T>, auto&&... args) noexcept(std::is_nothrow_constructible_v<T, decltype(args)...>)
    : Variant(std::in_place_index<FindType<T>()>, std::forward<decltype(args)>(args)...) {}

  template<typename T> requires (sizeof...(Types) > 0 && !IsInstanceOf<T, Variant> && kInitInd<T> != npos && !IsInstanceOf<T, std::in_place_type_t> && !IsInstanceOfWithVals<T, std::in_place_index_t>)
  constexpr Variant(T&& val)
    : Variant(std::in_place_index<kInitInd<T>>, fwd(val)) {
  }

  constexpr Variant() : Variant(std::in_place_index<0>) {}

  constexpr index_t index() const { return cur_type_ind_; }

  template <typename T>
  constexpr auto &&Get(this auto &&self) {
    return fwd(self).data_.template Get<T>();
  }

  template <size_t TInd>
  constexpr auto &&Get(this auto &&self) {
    return fwd(self).template Get<IthT<TInd>>();
  }

  template<typename T>
  constexpr T &Emplace(auto&&... args) noexcept(noexcept(Destroy()) && std::is_nothrow_constructible_v<T, decltype(args)...>) {
    Destroy();
    ::new (&data_.template Get<T>()) T(std::forward<decltype(args)>(args)...);
    cur_type_ind_ = FindType<T>();
    return Get<T>();
  }

  constexpr Variant(const Variant &)
    noexcept (kIsAll<std::is_nothrow_copy_constructible>)
    requires (kIsAll<std::is_trivially_copy_constructible>) = default;

  constexpr Variant(const Variant &)
    noexcept (kIsAll<std::is_nothrow_copy_constructible>)
    requires (kIsAll<std::is_copy_constructible> && !kIsAll<std::is_trivially_copy_constructible>);

  constexpr Variant(Variant &&)
    noexcept (kIsAll<std::is_nothrow_move_constructible>)
    requires (kIsAll<std::is_trivially_move_constructible>) = default;

  constexpr Variant(Variant &&)
    noexcept (kIsAll<std::is_nothrow_move_constructible>)
    requires (kIsAll<std::is_move_constructible> && !kIsAll<std::is_trivially_move_constructible>);

  constexpr Variant &operator=(const Variant&)
    noexcept (kIsAll<std::is_nothrow_copy_assignable>)
    requires (kIsAll<std::is_trivially_copy_assignable>) = default;

  constexpr Variant &operator=(const Variant &)
    noexcept (kIsAll<std::is_nothrow_copy_assignable>)
    requires (kIsAll<std::is_copy_assignable> && !kIsAll<std::is_trivially_copy_assignable>);

  constexpr Variant &operator=(Variant&&)
    noexcept (kIsAll<std::is_nothrow_move_assignable>)
    requires (kIsAll<std::is_trivially_move_assignable>) = default;

  constexpr Variant &operator=(Variant&&)
    noexcept (kIsAll<std::is_nothrow_move_assignable>)
    requires (kIsAll<std::is_move_assignable> && !kIsAll<std::is_trivially_move_assignable>);

  constexpr bool ValuelessByException() const noexcept {
    return index() == npos;
  }

  ~Variant()
    requires(!kIsAll<std::is_trivially_destructible>) {
    Destroy();
    }

  using __trivially_relocatable _LIBCPP_NODEBUG =
      std::conditional_t<(std::__libcpp_is_trivially_relocatable<Types>::value && ...), Variant, void>;

  ~Variant()
    requires(kIsAll<std::is_trivially_destructible>) = default;

private:
  VariadicUnion<(std::is_trivially_destructible_v<Types> && ...), Types...> data_;

  index_t cur_type_ind_ = npos;

  constexpr void Destroy() noexcept((std::is_nothrow_destructible_v<Types> && ...));
};

template<size_t Ind, typename T>
using VariantAlternative = typename decltype([] <size_t CurNeedInd, typename SepT, typename... Ts> (this auto self, std::integral_constant<size_t, CurNeedInd>, std::type_identity<Variant<SepT, Ts...>>) {
  if constexpr (Ind == 0) {
    return std::type_identity<SepT>{};
  } else {
    return std::type_identity<typename decltype(self(std::integral_constant<size_t, Ind - 1>{}, std::type_identity<Variant<Ts...>>{}))::type>{};
  }
} (std::integral_constant<size_t, Ind>{}, std::type_identity<T>{}))::type;

template<typename T>
constexpr size_t VariantSz = [] <typename... Ts> (std::type_identity<Variant<Ts...>>) { return sizeof...(Ts); } (std::type_identity<T>{});

template<typename... Ts>
consteval auto GetArrOfIndsArrs() {
  std::array<std::array<size_t, sizeof...(Ts)>, (VariantSz<Ts> * ...)> ans{};
  std::array<size_t, sizeof...(Ts)> szs{ VariantSz<Ts>... };
  for (size_t cur_ind = 1; cur_ind < ans.size(); ++cur_ind) {
    ans[cur_ind] = ans[cur_ind - 1];
    for (size_t i = 0, carry = 1; carry > 0; ++i) {
      if (i >= ans[cur_ind].size()) {
        throw std::logic_error(__PRETTY_FUNCTION__);
      }
      size_t buf = ans[cur_ind][i] + carry;
      carry = buf / szs[i];
      ans[cur_ind][i] = buf % szs[i];
    }
  }
  std::array<std::tuple<std::conditional_t<true, size_t, Ts>...>, ans.size()> ans_tr{};
  for (size_t i = 0; i < ans.size(); ++i) {
    ans_tr[i] = [&ansi = ans[i]] <size_t... Inds> (std::index_sequence<Inds...>) {
      return std::tuple(ansi[Inds]...);
    } (std::make_index_sequence<sizeof...(Ts)>{});
  }
  return ans_tr;
}


constexpr decltype(auto) Visit(auto &&vis, auto &&...var) {
  if ((var.ValuelessByException() || ...)) {
    throw std::bad_variant_access();
  }
  static constexpr auto case_arr = GetArrOfIndsArrs<std::remove_cvref_t<decltype(var)>...>();
  auto inds = std::tuple(var.index()...);
  return [] <size_t Ind> (this auto self, std::integral_constant<size_t, Ind>, auto &inds, auto &&vis, auto&&... var) {
    if (inds == case_arr[Ind]) {
      return [] <size_t... VarInds> (std::index_sequence<VarInds...>, auto &&vis, auto&&... var) static {
        return vis(fwd(var).template Get<std::get<VarInds>(case_arr[Ind])>()...);
      } (std::make_index_sequence<sizeof...(var)>{}, fwd(vis), fwd(var)...);
    } else {
      if constexpr (Ind + 1 >= case_arr.size()) {
        std::terminate();
      } else {
        return self(std::integral_constant<size_t, Ind + 1>{}, inds, fwd(vis), fwd(var)...);
      }
    }
  } (std::integral_constant<size_t, 0>{}, inds, fwd(vis), fwd(var)...);
}

template <typename... Types>
constexpr Variant<Types...>::Variant(const Variant &other)
    noexcept (kIsAll<std::is_nothrow_copy_constructible>)
    requires (kIsAll<std::is_copy_constructible> && !kIsAll<std::is_trivially_copy_constructible>)
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
    noexcept (kIsAll<std::is_nothrow_move_constructible>)
    requires (kIsAll<std::is_move_constructible> && !kIsAll<std::is_trivially_move_constructible>)
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
    noexcept (kIsAll<std::is_nothrow_copy_assignable>)
    requires(kIsAll<std::is_copy_assignable> && !kIsAll<std::is_trivially_copy_assignable>) {
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
constexpr Variant<Types...> &Variant<Types...>::operator=(Variant &&other)
    noexcept (kIsAll<std::is_nothrow_move_assignable>)
    requires (kIsAll<std::is_move_assignable> && !kIsAll<std::is_trivially_move_assignable>) {
  if (other.ValuelessByException()) {
    Destroy();
    return *this;
  }
  if (index() == other.index()) {
    Visit([this] <typename T> (T &&val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
      data_.template Get<T>() = val;
    }, std::move(other));
  } else {
    Destroy();
    Visit([this] <typename T> (T &&val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
      ::new (&data_.template Get<T>()) T(std::forward<decltype(val)>(val));
    }, std::move(other));
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

template<typename T>
constexpr size_t VariantSize = [] <typename... Ts> (std::type_identity<Variant<Ts...>>) {
  return sizeof...(Ts);
} (std::type_identity<T>{});

template<typename T>
constexpr size_t VariantNpos = T::npos;

template<typename... Ts>
void swap(Variant<Ts...> &a, Variant<Ts...> &b)
    noexcept (((std::is_nothrow_swappable_v<Ts> && std::is_trivially_move_constructible_v<Ts>) && ...))
    requires (((std::is_move_constructible_v<Ts> && std::is_swappable_v<Ts>) && ...))
{
  if (a.ValuelessByException() && b.ValuelessByException()) {

  } else if (a.index() == b.index()) {
    Visit([&b] <class T> (T &a_val) mutable {
      auto &b_val = b.template Get<T>();
      std::iter_swap(&a_val, &b_val);
    }, a);
  } else {
    auto c = std::move(a);
    a = std::move(b);
    b = std::move(c);
  }
}
} // namespace my

namespace std {
template<typename... Ts> requires ((requires { std::hash<std::remove_const_t<Ts>>{}; } && ...))
struct hash<my::Variant<Ts...>> {
  static constexpr size_t operator() (const my::Variant<Ts...> &v) noexcept {
    if (v.ValuelessByException()) {
      return 0;
    }
    return v.index() + 1 + my::Visit([] (auto &&val) { return std::hash<std::remove_cvref_t<decltype(val)>>{}(val); }, v) * (sizeof...(Ts) + 1);
  }
};
}

#undef fwd