#ifndef ONIKIRI_FUNCTIONAL_H
#define ONIKIRI_FUNCTIONAL_H
namespace Onikiri {

// Base classes for introducing type aliases
// Actually, current Onikiri2 code uses 'result_type' only.

template<class ArgumentType, class ResultType>
struct unary_function {
    using result_type = ResultType;
    using argument_type = ArgumentType;
};

template<class Argument1Type, class Argument2Type, class ResultType>
struct binary_function {
    using result_type = ResultType;
    using first_argument_type = Argument1Type;
    using second_argument_type = Argument2Type;

};

} // namespace Onikiri

#endif // ONIKIRI_FUNCTIONAL_H
