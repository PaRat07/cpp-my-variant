constexpr int kVariantSize = 30;

#include "somevisit.h"
#include <array>
#include <variant>
#include <random>
#include <functional>
#include <algorithm>
#include <ranges>
#include <chrono>
#include <iostream>

// #include "my-visit.h"
#include <print>
#include "my-variant.h"

using namespace std;

// template<typename... Ts>
// constexpr auto kActI = std::array {
//   std::variant<Ts...>(std::in_place_type<Ts>)...
// };
//
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

consteval void TestTrivial() {
    static_assert(std::is_trivially_copyable_v<Variant<int, float, double>>);
}

consteval void TestLogic() {

}


int main() {
    TestTrivial();
}
