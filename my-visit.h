#pragma once

#include <algorithm>
#include <cstdint>
#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>
#include <variant>

#define fwd(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define PRINT_TYPE(...) static_assert((std::type_identity<decltype(__VA_ARGS__)>{}, false))

template<typename... Ts>
consteval auto GetArrOfIndsArrs() {
  std::array<std::array<size_t, sizeof...(Ts)>, (std::variant_size_v<Ts> * ...)> ans{};
  std::array<size_t, sizeof...(Ts)> szs{ std::variant_size_v<Ts>... };
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
  if ((var.valueless_by_exception() || ...)) {
    throw std::bad_variant_access();
  }
  static constexpr auto case_arr = GetArrOfIndsArrs<std::remove_cvref_t<decltype(var)>...>();
  auto inds = std::tuple(var.index()...);
  return [] <size_t Ind> (this auto self, std::integral_constant<size_t, Ind>, auto &inds, auto &&vis, auto&&... var) {
    if (inds == case_arr[Ind]) {
      return [] <size_t... VarInds> (std::index_sequence<VarInds...>, auto &&vis, auto&&... var) static {
        return vis(std::get<std::get<VarInds>(case_arr[Ind])>(fwd(var))...);
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


constexpr decltype(auto) VisitWithUnreachable(auto &&vis, auto &&...var) {
  if ((var.valueless_by_exception() || ...)) {
    throw std::bad_variant_access();
  }
  static constexpr auto case_arr = GetArrOfIndsArrs<std::remove_cvref_t<decltype(var)>...>();
  auto inds = std::tuple(var.index()...);
  return [] <size_t Ind> (this auto self, std::integral_constant<size_t, Ind>, auto &inds, auto &&vis, auto&&... var) {
    if (inds == case_arr[Ind]) {
      return [] <size_t... VarInds> (std::index_sequence<VarInds...>, auto &&vis, auto&&... var) static {
        return vis(std::get<std::get<VarInds>(case_arr[Ind])>(fwd(var))...);
      } (std::make_index_sequence<sizeof...(var)>{}, fwd(vis), fwd(var)...);
    } else {
      if constexpr (Ind + 1 >= case_arr.size()) {
        std::unreachable();
      } else {
        return self(std::integral_constant<size_t, Ind + 1>{}, inds, fwd(vis), fwd(var)...);
      }
    }
  } (std::integral_constant<size_t, 0>{}, inds, fwd(vis), fwd(var)...);
}

#undef fwd
