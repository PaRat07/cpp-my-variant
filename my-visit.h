#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <variant>


decltype(auto) Visit(auto &&vis, auto &&var) noexcept {
  static constexpr auto ftable = [] <typename... Ts> (std::type_identity<std::variant<Ts...>>) {
    return std::array {
        +[] (decltype(vis) vis, std::variant<Ts...> &var) static noexcept {
          return vis(std::get<Ts>(var));
        }...
      };
  } (std::type_identity<std::decay_t<decltype(var)>>{});
  return ftable[var.index()](std::forward<decltype(vis)>(vis), std::forward<decltype(var)>(var));
}


decltype(auto) Visit(auto &&vis, auto &&fvar, auto&&... var) noexcept {
  return Visit([&vis, &var...] (auto &&val) noexcept {
    return Visit([&vis, &val] (auto&&... vals) noexcept {
      return vis(std::forward<decltype(val)>(val), std::forward<decltype(vals)>(vals)...);
    }, std::forward<decltype(var)>(var)...);
  }, std::forward<decltype(fvar)>(fvar));
}
