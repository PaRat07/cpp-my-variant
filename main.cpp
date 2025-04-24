// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include <cstring>
#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <numeric>
#include <unordered_set>
#include <span>
#include <utility>
#include <variant>
#include <vector>
#include <ranges>

#include "my-variant.h"
#include <bit>
#include <condition_variable>

// template struct Variant<int, std::string>;

// #define int int64_t
// #include "my-visit.h"

using namespace std;


namespace rng = ranges;
namespace vie = views;


template<typename... Ts>
struct Overl : Ts... {
  using Ts::operator()...;
};

struct NotTrivReloc {
  NotTrivReloc(const NotTrivReloc&) {}
  NotTrivReloc(NotTrivReloc&&) {}

  ~NotTrivReloc() {}
};

// struct NotTrivReloc {};

template<typename VarT>
static const auto kActivatedFor = [] <typename... Ts> (std::type_identity<Variant<Ts...>>) {
  return std::array {
    VarT(std::type_identity<Ts>{})...
  };
} (std::type_identity<std::remove_cvref_t<VarT>>{});

signed main() {
  volatile int noopt = 0;
  // {
  //   auto beg = std::chrono::steady_clock::now();
  //   std::vector<std::variant<int, std::string, NotTrivReloc>> var_vec;
  //   var_vec.push_back(std::variant<int, std::string, NotTrivReloc>(std::in_place_index<0>));
  //   for (int i : views::iota(0, 10'000'000 * 0)) {
  //     noopt += var_vec.back().visit(Overl{
  //       [] (int) -> int { return 1; },
  //       [] (const std::string&) -> int { return 2; },
  //       [] (const NotTrivReloc&) -> int { std::terminate(); }
  //     });
  //     if (rand() % 2) {
  //       var_vec.push_back(std::variant<int, std::string, NotTrivReloc>(std::in_place_index<0>));
  //     } else {
  //       var_vec.push_back(std::variant<int, std::string, NotTrivReloc>(std::in_place_index<1>));
  //     }
  //   }
  //   auto end = std::chrono::steady_clock::now();
  //   std::println("std::variant: {}", std::chrono::duration_cast<std::chrono::milliseconds>(end - beg));
  // }
  // {
  //   auto beg = std::chrono::steady_clock::now();
  //   std::vector<Variant<int, std::string, NotTrivReloc>> var_vec;
  //   var_vec.push_back(Variant<int, std::string, NotTrivReloc>(std::in_place_index<0>));
  //   for (int i : views::iota(0, 10'000'000 * 0)) {
  //     noopt += Visit(Overl{
  //       [] (int) -> int { return 1; },
  //       [] (const std::string&) -> int { return 2; },
  //       [] (const NotTrivReloc&) -> int { std::terminate(); }
  //     }, var_vec.back());
  //
  //     if (rand() % 2) {
  //       var_vec.push_back(Variant<int, std::string, NotTrivReloc>(std::in_place_index<0>));
  //     } else {
  //       var_vec.push_back(Variant<int, std::string, NotTrivReloc>(std::in_place_index<1>));
  //     }
  //   }
  //   auto end = std::chrono::steady_clock::now();
  //   std::println("Variant: {}", std::chrono::duration_cast<std::chrono::milliseconds>(end - beg));
  // }
  // [&] <size_t... Inds> (std::index_sequence<Inds...>) {
    [&] <size_t Ind> (std::integral_constant<size_t, Ind>) {
      using UnderT = decltype([] <size_t... TInds> (std::index_sequence<TInds...>) {
        return std::type_identity<Variant<std::integral_constant<size_t, TInds>..., float, int>>{};
      } (std::make_index_sequence<Ind>{}))::type;

      static constexpr auto kOvr = [] <size_t... TInds> (std::index_sequence<TInds...>) {
        return Overl {
          [] (std::integral_constant<size_t, TInds>) { return TInds; }...,
          [] (int) { return Ind + 1; },
          [] (float) { return Ind + 2; }
        };
      } (std::make_index_sequence<Ind>{});
      {
        auto beg = std::chrono::steady_clock::now();
        UnderT var(std::in_place_index<Ind + 1>);
        for (int i : views::iota(0, 10'000'000)) {
          noopt += Visit<true>(kOvr, var);

          var = kActivatedFor<UnderT>[rand() % (Ind + 2)];
        }
        auto end = std::chrono::steady_clock::now();
        std::println("Visit<true> for {} elems: {}", Ind, std::chrono::duration_cast<std::chrono::milliseconds>(end - beg));
      }
      {
        auto beg = std::chrono::steady_clock::now();
        UnderT var(std::in_place_index<Ind + 1>);
        for (int i : views::iota(0, 10'000'000)) {
          noopt += Visit<false>(kOvr, var);

          var = kActivatedFor<UnderT>[rand() % (Ind + 2)];
        }
        auto end = std::chrono::steady_clock::now();
        std::println("Visit<false> for {} elems: {}", Ind, std::chrono::duration_cast<std::chrono::milliseconds>(end - beg));
      }
    } (std::integral_constant<size_t, 100>{});
  // } (std::make_index_sequence<100>{});
}

