
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


template<typename Func, typename... Types>
struct AllInvokeResultSameImpl;

template<typename Func, typename Type>
struct AllInvokeResultSameImpl<Func, Type> {
    static inline constexpr bool value = true;
};

template<typename Func, typename First, typename Second, typename... Types>
struct AllInvokeResultSameImpl<Func, First, Second, Types...> {
    static inline constexpr bool value = std::is_same_v<std::invoke_result_t<Func, First>, std::invoke_result_t<Func, Second>> && AllInvokeResultSameImpl<Func, Second, Types...>::value;
};

template<typename Func, typename... Types>
constexpr bool AllInvokeResultSame = AllInvokeResultSameImpl<Func, Types...>::value;

template<typename... Types>
class Wrapper {};

template<typename Func, typename InvokeResultType, typename ReapetedBytePtrWrapper, typename... Wrappers>
struct FuncArrayImpl;

template<typename Func, typename InvokeResultType, typename... ReapetedBytePtr, typename... TypesInFirstWrapper>
struct FuncArrayImpl<Func, InvokeResultType, Wrapper<ReapetedBytePtr...>, Wrapper<TypesInFirstWrapper...>> {
    using type = std::array<std::function<InvokeResultType(Func, ReapetedBytePtr...)>, sizeof...(TypesInFirstWrapper)>;
};

template<typename Func, typename InvokeResultType, typename... ReapetedBytePtr, typename... TypesInFirstWrapper, typename... OtherWrappers>
struct FuncArrayImpl<Func, InvokeResultType, Wrapper<ReapetedBytePtr...>, Wrapper<TypesInFirstWrapper...>, OtherWrappers...> {
    using type = std::array<typename FuncArrayImpl<Func, InvokeResultType, Wrapper<ReapetedBytePtr..., std::byte*>, OtherWrappers...>::type, sizeof...(TypesInFirstWrapper)>;
};

template<typename... Args>
using FuncArray = typename FuncArrayImpl<Args...>::type;



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
