#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <stdexcept>
#include <variant>
#include <stdexcept>
#include <utility>





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
    return [] <typename... Ts> (std::type_identity<std::variant<Ts...>>) {
      return (CanNoexcept(std::type_identity<VisT>{},
                          std::type_identity<std::tuple<CurActTs..., decltype(std::forward_like<SepVar>(std::declval<Ts>()))>>{},
                          std::type_identity<OtherVarsTs>{}...) && ...);
    } (std::type_identity<std::decay_t<SepVar>>{});
  }
}

template<typename VisT, typename... VarTs>
constexpr bool kIsNoexc = CanNoexcept(std::type_identity<VisT>{},
                                      std::type_identity<std::tuple<>>{},
                                      std::type_identity<VarTs>{}...);

template<bool IsNoexc>
decltype(auto) VisitImpl(auto&& vis, auto&&... var) noexcept(IsNoexc) {
  return [] (auto &&vis, auto &&sep_var, auto&&... vars) noexcept(IsNoexc) {
    return [&vis, &sep_var, &vars...] <typename... SepVarTs> (std::type_identity<std::variant<SepVarTs...>>) noexcept(IsNoexc) {
      return [&vis, &sep_var, &vars...] <size_t MyInd, size_t... Inds> (this auto &&self, std::index_sequence<MyInd, Inds...>) noexcept(IsNoexc) {
        if (sep_var.index() == MyInd) {
          if constexpr (sizeof...(vars) == 0) {
            return vis(std::forward_like<decltype(sep_var)>(std::get<SepVarTs...[MyInd]>(sep_var)));
          } else {
            return VisitImpl<IsNoexc>([&] (auto&&... vals) noexcept(IsNoexc) {
              return vis(std::forward_like<decltype(sep_var)>(std::get<SepVarTs...[MyInd]>(sep_var)), std::forward<decltype(vals)>(vals)...);
            }, std::forward<decltype(vars)>(vars)...);
          }
        } else if constexpr (sizeof...(Inds) == 0) {
          std::unreachable();
        } else {
          return self(std::index_sequence<Inds...>{});
        }
      } (std::make_index_sequence<sizeof...(SepVarTs)>{});
    } (std::type_identity<std::remove_cvref_t<decltype(sep_var)>>{});
  } (std::forward<decltype(vis)>(vis), std::forward<decltype(var)>(var)...);
}

decltype(auto) Visit(auto &&vis, auto &&...var) {
  if ((var.valueless_by_exception() || ...)) [[unlikely]]{
    throw std::bad_variant_access();
  }
  return VisitImpl<kIsNoexc<decltype(vis), decltype(var)...>>(std::forward<decltype(vis)>(vis), std::forward<decltype(var)>(var)...);
}
