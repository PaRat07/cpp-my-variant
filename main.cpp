// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "my-variant.h"
#include "bechmark.h"

#include <iostream>
#include <ranges>
#include <variant>
#include <chrono>

using namespace std;

int main() {
    volatile unsigned no_optimize = 0;
    for (int _ : views::iota(0, 100)) {
        {
            std::variant<ALL, std::string> va1(std::to_string(52));
            auto beg_time = std::chrono::steady_clock::now();
            for (int i : std::views::iota(0, 10'000'000)) {
                std::visit([&no_optimize](auto x) {
                    no_optimize = 0;
                }, va1);
            }
            auto end_time = std::chrono::steady_clock::now();
            std::cout << "std::Visit: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - beg_time).count()
                      << std::endl;
        }
        {
            Variant<ALL, std::string> var(std::to_string(52));
            auto beg_time = std::chrono::steady_clock::now();
            for (int i : std::views::iota(0, 10'000'000)) {
                var.Visit([&no_optimize](auto x) {
                    no_optimize = 1;
                });
            }
            auto end_time = std::chrono::steady_clock::now();
            std::cout << "Visit: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - beg_time).count()
                      << std::endl;
        }
    }
    return 0;
}
