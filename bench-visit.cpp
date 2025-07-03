constexpr int kVariantSize = 8;

#include "somevisit.h"
#include <array>
#include <variant>
#include <random>
#include <functional>
#include <algorithm>
#include <ranges>
#include <chrono>

#include "my-visit.h"
#include <iostream>
// #include "my-variant.h"

using namespace std;

template<typename... Ts>
constexpr auto kActI = std::array {
  std::variant<Ts...>(std::in_place_type<Ts>)...
};

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

int main() {
  [] <size_t... Inds> (std::index_sequence<Inds...>) {
    volatile int noopt = 0;
    volatile size_t arr[sizeof...(Inds)];
    for (auto i : std::views::iota(0u, sizeof...(Inds))) {
      arr[i] = i;
    }
    using VarT = std::variant<std::integral_constant<size_t, Inds>...>;
    {
      VarT var;
      auto beg = std::chrono::steady_clock::now();
      for (auto _ : std::views::iota(0, 10'000'000)) {
        var = kActI<std::integral_constant<size_t, Inds>...>[rand() % sizeof...(Inds)];
        noopt += std::visit(overloaded{
          [] (std::integral_constant<size_t, Inds>) { return Inds; }...
        }, var);
      }
      auto end = std::chrono::steady_clock::now();
      std::cout << std::format("std::visit: {}ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count());
    }
    {
      VarT var;
      auto beg = std::chrono::steady_clock::now();
      for (auto _ : std::views::iota(0, 10'000'000)) {
        var = kActI<std::integral_constant<size_t, Inds>...>[rand() % sizeof...(Inds)];
        noopt += rollbear::visit(overloaded{
          [] (std::integral_constant<size_t, Inds>) { return Inds; }...
        }, var);
      }
      auto end = std::chrono::steady_clock::now();
      std::cout << std::format("rollbear::visit: {}ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count());
    }
      {
      VarT var;
      auto beg = std::chrono::steady_clock::now();
      for (auto _ : std::views::iota(0, 10'000'000)) {
        var = kActI<std::integral_constant<size_t, Inds>...>[rand() % sizeof...(Inds)];
        noopt += VisitWithUnreachable(overloaded{
          [] (std::integral_constant<size_t, Inds>) { return Inds; }...
        }, var);
      }
      auto end = std::chrono::steady_clock::now();
      std::cout << std::format("VisitWithUnreachable: {}ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count());
      }
      {
      VarT var;
      auto beg = std::chrono::steady_clock::now();
      for (auto _ : std::views::iota(0, 10'000'000)) {
        var = kActI<std::integral_constant<size_t, Inds>...>[rand() % sizeof...(Inds)];
        noopt += Visit(overloaded{
          [] (std::integral_constant<size_t, Inds>) { return Inds; }...
        }, var);
      }
      auto end = std::chrono::steady_clock::now();
      std::cout << std::format("Visit: {}ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count());
      }
  } (std::make_index_sequence<kVariantSize>{});
}