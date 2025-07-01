#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <stdexcept>
#include <variant>
#include <stdexcept>
#include <utility>


template<typename... Ts>
consteval std::array<std::array<size_t, sizeof...(Ts)>, (std::variant_size_v<Ts> * ...)>  GetArrOfIndsArrs() {
  std::array<std::array<size_t, sizeof...(Ts)>, (std::variant_size_v<Ts> * ...)> ans;
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
  static constexpr auto case_arr = GetArrOfIndsArrs<std::remove_cvref_t<decltype(var)>...>();
  std::array<size_t, sizeof...(var)> inds = { var.index()... };
  return [&inds, &var..., &vis] <size_t... Inds> (std::index_sequence<Inds...>) {
    ([&inds, &var..., &vis] <size_t Ind> (std::integral_constant<size_t, Ind>) {
      if (inds == case_arr[Ind]) {
        return [&var..., &vis] <size_t... VarInds> (std::index_sequence<VarInds...>) {
          return vis(std::get<case_arr[Inds][VarInds]>(std::forward<decltype(var)>(var))...);
        } (std::make_index_sequence<sizeof...(var)>{});
      }
    } (std::integral_constant<size_t, Inds>{}), ...);
  } (std::make_index_sequence<case_arr.size()>{});
}
