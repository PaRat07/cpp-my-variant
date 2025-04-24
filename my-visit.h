#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <stdexcept>
#include <variant>




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
