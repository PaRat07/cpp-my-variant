// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>

struct EmptyT {};


template<template<typename> typename Op, typename... Ts>
constexpr bool kCanAllDoTrivial = ((std::is_trivially_copyable_v<Ts> && Op<Ts>::value) && ...);

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
  using index_t =
      std::conditional_t<sizeof...(Types) < std::numeric_limits<uint8_t>::max(),
                         uint8_t, uint16_t>;

private:
  static constexpr index_t npos = sizeof...(Types);

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
    requires(std::is_trivially_copy_constructible_v<Types> && ...) = default;

  constexpr Variant(const Variant &)
    requires(!std::is_trivially_copy_constructible_v<Types> || ...);

  constexpr Variant(Variant &&)
    requires(std::is_trivially_move_constructible_v<Types> && ...) = default;

  constexpr Variant(Variant &&)
    requires(!std::is_trivially_move_constructible_v<Types> || ...);

  constexpr Variant &operator=(const Variant&)
    requires (std::is_trivially_copy_assignable_v<Types> && ...) = default;

  constexpr Variant &operator=(const Variant &)
    requires(!std::is_trivially_copy_assignable_v<Types> || ...);

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

template <typename VisT,
          typename SepVar,
          typename... OtherVarsTs,
          typename... CurActTs>
constexpr bool CanNoexcept(std::type_identity<VisT>,
                           std::type_identity<std::tuple<CurActTs...>>,
                           std::type_identity<SepVar>,
                           std::type_identity<OtherVarsTs>...) {

  if constexpr (sizeof...(OtherVarsTs) == 0) {
    return std::is_nothrow_invocable_v<VisT, CurActTs...>;
  } else {
    return [] <typename... Ts> (std::type_identity<Variant<Ts...>>) {
      return (CanNoexcept(std::type_identity<VisT>{},
                          std::type_identity<std::tuple<CurActTs..., decltype(std::forward_like<SepVar>(std::declval<Ts>()))>>{},
                          std::type_identity<OtherVarsTs>{}...) && ...);
    } (std::type_identity<std::decay_t<SepVar>>{});
  }
}

template<typename VisT, typename... CurHaveTs, typename SepVarT, typename... VarsT, bool IsNoexcept, typename... VarsTs>
consteval auto BuildFtable(std::bool_constant<IsNoexcept>,
                          std::type_identity<VisT>,
                          std::type_identity<std::tuple<VarsTs...>>,
                          std::type_identity<std::tuple<CurHaveTs...>>,
                          std::type_identity<SepVarT>,
                          std::type_identity<VarsT>...) {
  if constexpr (sizeof...(VarsT) == 0) {
    return [] <typename... Ts> (std::type_identity<Variant<Ts...>>) {
      return std::array {
        [] <typename... AllCastTs> (std::type_identity<std::tuple<AllCastTs...>>) {
          return +[] (VisT vis, VarsTs... vars) static noexcept(IsNoexcept) {
              return vis(std::forward_like<VarsTs>(vars.template Get<AllCastTs>())...);
            };
        } (std::type_identity<std::tuple<CurHaveTs..., std::remove_cvref_t<Ts>>>{})...
      };
    } (std::type_identity<std::remove_cvref_t<SepVarT>>{});
  } else {
    return [] <typename... Ts> (std::type_identity<Variant<Ts...>>) {
      return std::array {
        BuildFtable(std::bool_constant<IsNoexcept>{},
                    std::type_identity<VisT>{},
                    std::type_identity<std::tuple<VarsTs...>>{},
                    std::type_identity<std::tuple<CurHaveTs..., std::remove_cvref_t<Ts>>>{},
                    std::type_identity<VarsT>{}...)...
      };
    } (std::type_identity<std::remove_cvref_t<SepVarT>>{});
  }
}

auto &&MdAt(auto &&arr, auto sep_ind, auto... inds) {
  if constexpr (sizeof...(inds) == 0) {
    return std::forward<decltype(arr)>(arr)[sep_ind];
  } else {
    return MdAt(std::forward<decltype(arr)>(arr)[sep_ind], inds...);
  }
}
template<bool BuildTable = false>
decltype(auto) Visit(auto &&vis, auto &&...var) noexcept {
  if constexpr (BuildTable) {
    static constexpr bool kIsNoexcept = CanNoexcept(std::type_identity<decltype(vis)>{},
                                                    std::type_identity<std::tuple<>>{},
                                                    std::type_identity<decltype(var)>{}...);

    static constexpr auto ftable = BuildFtable(std::bool_constant<kIsNoexcept>{},
                                               std::type_identity<decltype(vis)>{},
                                               std::type_identity<std::tuple<decltype(var)...>>{},
                                               std::type_identity<std::tuple<>>{},
                                               std::type_identity<decltype(var)>{}...);

    return MdAt(ftable, var.index()...)(std::forward<decltype(vis)>(vis), std::forward<decltype(var)>(var)...);
  } else {
    return [] (auto &&vis, auto &&sep_var, auto&&... vars) {
      return [&vis, &sep_var, &vars...] <typename... SepVarTs> (std::type_identity<Variant<SepVarTs...>>) {
        return [&vis, &sep_var, &vars...] <size_t MyInd, size_t... Inds> (this auto &&self, std::index_sequence<MyInd, Inds...>) {
          if (sep_var.index() == MyInd) {
            if constexpr (sizeof...(vars) == 0) {
              return vis(std::forward_like<decltype(sep_var)>(sep_var.template Get<SepVarTs...[MyInd]>()));
            } else {
              return Visit<false>([&] (auto&&... vals) {
                return vis(std::forward_like<decltype(sep_var)>(sep_var.template Get<SepVarTs...[MyInd]>()), std::forward<decltype(vals)>(vals)...);
              }, std::forward<decltype(vars)>(vars)...);
            }
          } else if constexpr (sizeof...(Inds) == 0) {
            std::terminate();
          } else {
            return self(std::index_sequence<Inds...>{});
          }
        } (std::make_index_sequence<sizeof...(SepVarTs)>{});
      } (std::type_identity<std::remove_cvref_t<decltype(sep_var)>>{});
    } (std::forward<decltype(vis)>(vis), std::forward<decltype(var)>(var)...);
  }
}

template <typename... Types>
constexpr Variant<Types...>::Variant(const Variant &other)
  requires(!std::is_trivially_copy_constructible_v<Types> || ...)
  : Variant(std::in_place_index<sizeof...(Types)>)
{
  
  Visit([this] <typename T> (const T &val) noexcept(std::is_nothrow_copy_constructible_v<decltype(val)>) {
    ::new (&data_.template Get<T>()) T(val);
  }, other);
  cur_type_ind_ = other.index();
}

template <typename... Types>
constexpr Variant<Types...>::Variant(Variant &&other)
  requires(!std::is_trivially_move_constructible_v<Types> || ...)
  : Variant(std::in_place_index<sizeof...(Types)>)
{
  Visit([this] <typename T> (T &&val) noexcept(std::is_nothrow_move_constructible_v<T>) {
    ::new (&data_.template Get<T>()) T(std::forward<decltype(val)>(val));
  }, std::move(other));
  cur_type_ind_ = other.index();
}

template <typename... Types>
constexpr Variant<Types...> &Variant<Types...>::operator=(const Variant &other)
  requires(!std::is_trivially_copy_assignable_v<Types> || ...) {
  if (index() == other.index()) {
    if constexpr ((std::is_trivially_copyable_v<Types> && ...)) {
      std::memcpy(&data_, &other.data_, sizeof(data_));
    } else if (kIsTrivCopy<Types...>[index()]) {
      std::memcpy(&data_, &other.data_, sizeof(data_));
    } else {
      Visit([this] <typename T> (const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        data_.template Get<T>() = val;
      }, other);
    }
  } else {
    Destroy();
    if constexpr ((std::is_trivially_copyable_v<Types> && ...)) {
      std::memcpy(&data_, &other.data_, sizeof(data_));
    } else if (kIsTrivCopy<Types...>[other.index()]) {
      std::memcpy(&data_, &other.data_, sizeof(data_));
    } else {
      Visit([this] <typename T> (const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        ::new (&data_.template Get<T>()) T(std::forward<decltype(val)>(val));
      }, other);
    }
    cur_type_ind_ = other.index();
  }
  return *this;
}

template <typename... Types>
constexpr void Variant<Types...>::Destroy() noexcept((std::is_nothrow_destructible_v<Types> && ...)) {
  if (index() != npos) {
    if constexpr ((!std::is_trivially_destructible_v<Types> || ...)) {
      if (!kIsTrivDestr<Types...>[index()]) {
        Visit([] <typename T> (T &val) static noexcept(std::is_nothrow_destructible_v<decltype(val)>) {
          val.~T();
        }, *this);
      }
    }
    cur_type_ind_ = npos;
  }
}
