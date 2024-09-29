
#include <utility>
#include <array>
#include <functional>

namespace impl {
template<size_t ind, typename... Types>
struct IthTypeImpl;

template<typename First, typename... Other>
struct IthTypeImpl<0, First, Other...> {
    using type = First;
};

template<size_t ind, typename First, typename... Other>
struct IthTypeImpl<ind, First, Other...> {
    using type = typename IthTypeImpl<ind - 1, Other...>::type;
};

template<size_t ind, typename... Types>
using IthType = typename IthTypeImpl<ind, Types...>::value;

template<typename... Types>
class Wrapper {};


template<typename Func, typename WrapperOverTypesForCastTo, typename WrapperOverBytePtrs>
consteval auto GetFunc() {
    return [] <typename... TypesForCastTo, typename... BytePtrs> (Wrapper<TypesForCastTo...>, Wrapper<BytePtrs...>) {
        return +[] (const Func &f, BytePtrs... ptrs) {
                return f(reinterpret_cast<TypesForCastTo&>(*ptrs)...);
            };
    } (WrapperOverTypesForCastTo(), WrapperOverBytePtrs());
}

template<
    typename    Func,
    typename    WrapperOverTypesForCastTo,
    typename    WrapperOverBytePtrs,
    typename    FirstWrapperOverVariantTypes,
    typename... WrappersOverVariantsTypes
>
consteval auto GetFunctionTable() {
    if constexpr (sizeof...(WrappersOverVariantsTypes) == 0) {
        return
            [] <
                    typename... VariantTypes,
                    typename... TypesForCastTo,
                    typename... BytePtrs
                > (Wrapper<VariantTypes...>, Wrapper<TypesForCastTo...>, Wrapper<BytePtrs...>) {
                return std::array {
                    GetFunc<Func, Wrapper<TypesForCastTo..., VariantTypes>, Wrapper<BytePtrs..., std::byte*>>()...
                };
            } (FirstWrapperOverVariantTypes(), WrapperOverTypesForCastTo(), WrapperOverBytePtrs());
    } else {
        return
            [] <
                    typename... CurVariantTypes,
                    typename... TypesForCastTo,
                    typename... BytePtrs,
                    typename    WrapperOverFirstVariantTypes,
                    typename... WrappersOverOtherVariantsTypes
                > (Wrapper<CurVariantTypes...>, Wrapper<TypesForCastTo...>, Wrapper<BytePtrs...>, WrapperOverFirstVariantTypes, WrappersOverOtherVariantsTypes...) {
                return std::array{
                    GetFunctionTable<Func, Wrapper<TypesForCastTo..., CurVariantTypes>, Wrapper<BytePtrs..., std::byte*>, WrapperOverFirstVariantTypes, WrappersOverOtherVariantsTypes...>()...
                };
            } (FirstWrapperOverVariantTypes(), WrapperOverTypesForCastTo(), WrapperOverBytePtrs(), (WrappersOverVariantsTypes(), ...));
    }
}


} // namespace impl
