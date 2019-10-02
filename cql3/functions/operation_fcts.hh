#pragma once

#include "cql3/functions/native_scalar_function.hh"

namespace cql3 {
namespace functions {

namespace operation_fcts {

inline
shared_ptr<function>
make_add_fct() {
    return make_native_scalar_function<true>("add", long_type, { long_type, long_type },
            [] (cql_serialization_format sf, const std::vector<bytes_opt>& values) -> bytes_opt {
        auto& a = values[0];
        auto& b = values[1];

        auto desa = long_type->deserialize(*a); 
        auto desb = long_type->deserialize(*b);

        auto c = value_cast<int64_t>(desa) + value_cast<int64_t>(desb);

        return { long_type->decompose(c) };
    });
}

}
}
}

