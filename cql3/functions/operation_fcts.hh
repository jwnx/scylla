#pragma once

#include "cql3/functions/function.hh"
#include "concrete_types.hh"

#include <type_traits>

namespace cql3::functions::operation_fcts {

template<typename Type>
constexpr bool is_number() {
    return std::is_same_v<Type, byte_type_impl>
        || std::is_same_v<Type, short_type_impl>
        || std::is_same_v<Type, int32_type_impl>
        || std::is_same_v<Type, long_type_impl>
        || std::is_same_v<Type, float_type_impl>
        || std::is_same_v<Type, double_type_impl>
        || std::is_same_v<Type, decimal_type_impl>
        ;
}

shared_ptr<function>
make_add_fct(data_type out_type, data_type in1_type, data_type in2_type);

shared_ptr<function>
make_subtract_fct(data_type out_type, data_type in1_type, data_type in2_type);

/*
-============= =========== ========== ========== ========== ========== ========== ========== ========== ==========
- left/right   tinyint      smallint   int        bigint     counter    float      double     varint     decimal
-============= =========== ========== ========== ========== ========== ========== ========== ========== ==========
- **tinyint**   tinyint     smallint   int        bigint     bigint     float      double     varint     decimal
- **smallint**  smallint    smallint   int        bigint     bigint     float      double     varint     decimal
- **int**       int         int        int        bigint     bigint     float      double     varint     decimal
- **bigint**    bigint      bigint     bigint     bigint     bigint     double     double     varint     decimal
- **counter**   bigint      bigint     bigint     bigint     bigint     double     double     varint     decimal
- **float**     float       float      float      double     double     float      double     decimal    decimal
- **double**    double      double     double     double     double     double     double     decimal    decimal
- **varint**    varint      varint     varint     decimal    decimal    decimal    decimal    decimal    decimal
- **decimal**   decimal     decimal    decimal    decimal    decimal    decimal    decimal    decimal    decimal
-============= =========== ========== ========== ========== ========== ========== ========== ========== ==========
*/
template<typename Func>
void declare_operations(Func& declare) {
    auto types = { decimal_type, double_type, float_type, long_type, int32_type, short_type, byte_type };
    for (auto bigger_type = types.begin(); bigger_type != types.end(); ++bigger_type) {
        declare(make_add_fct(*bigger_type, *bigger_type, *bigger_type));
        declare(make_subtract_fct(*bigger_type, *bigger_type, *bigger_type));
        for (auto smaller_type = bigger_type + 1; smaller_type != types.end(); ++smaller_type) {
            auto out_type = *bigger_type;
            if (*bigger_type == float_type && *smaller_type == long_type) {
                out_type = double_type;
            }

            declare(make_add_fct(out_type, *bigger_type, *smaller_type));
            declare(make_add_fct(out_type, *smaller_type, *bigger_type));

            declare(make_subtract_fct(out_type, *bigger_type, *smaller_type));
            declare(make_subtract_fct(out_type, *smaller_type, *bigger_type));
        }
    }
}

}
