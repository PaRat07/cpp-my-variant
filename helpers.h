
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

template<typename Func, typename... BytePtrs>
auto GetFunctionTable() {
    std::array ans;
    [&ans] <size_t... Inds> (std::index_sequence<Inds...>) {
        ([&ans] <size_t ind> (std::index_sequence<ind>) {
            ans[ind] = [] (const Func &to_apply, std::byte *arg) {
                return to_apply(reinterpret_cast<IthTypeImpl<ind, Types...>&>(*arg));
            };
        } (std::index_sequence<Inds>()), ...);
    } (std::make_index_sequence<sizeof...(Types)>());
    return ans;
}


template<typename Func, typename... WrapperT>
auto GetFunctionTable() {
    std::array<std::function<std::invoke_result_t<Func, typename IthTypeImpl<0, Types...>::type>(Func, std::byte*)>, sizeof...(Types)> ans;
    [&ans] <size_t... Inds> (std::index_sequence<Inds...>) {
        ([&ans] <size_t ind> (std::index_sequence<ind>) {
            ans[ind] = [] (const Func &to_apply, std::byte *arg) {
                return to_apply(reinterpret_cast<IthTypeImpl<ind, Types...>&>(*arg));
            };
        } (std::index_sequence<Inds>()), ...);
    } (std::make_index_sequence<sizeof...(Types)>());
    return ans;
}
} // namespace impl
