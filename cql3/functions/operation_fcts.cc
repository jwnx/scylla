#include "cql3/functions/operation_fcts.hh"

#include "cql3/functions/native_scalar_function.hh"

namespace cql3::functions::operation_fcts {

template<typename T>
struct unwrap {
    using type = T;
};

template<typename T>
struct unwrap<emptyable<T>> {
    using type = T;
};

template<typename Number>
using unwrap_num = typename unwrap<typename Number::native_type>::type;

struct visitor_base {
    data_value arg1;
    data_value arg2;

    template<typename Number>
    unwrap_num<Number> do_cast(const Number& lt, const data_value& dv) const {
        auto fun = get_castas_fctn(lt.shared_from_this(), dv.type());
        auto casted_arg = fun(dv);
        return value_cast<unwrap_num<Number>>(casted_arg);
    }
};

struct add_visitor : public visitor_base {
    // Can't be in the base class due to SFINAE
    bytes_opt operator()(const abstract_type&) const {
        throw std::logic_error("unreachable");
    }

    template<typename Number>
    GCC6_CONCEPT(requires is_number<Number>())
    bytes_opt operator()(const Number& lt) const {
        return lt.decompose(unwrap_num<Number>(do_cast(lt, arg1) + do_cast(lt, arg2)));
    }
};

struct subtract_visitor : public visitor_base {
    // Can't be in the base class due to SFINAE
    bytes_opt operator()(const abstract_type&) const {
        throw std::logic_error("unreachable");
    }

    template<typename Number>
    GCC6_CONCEPT(requires is_number<Number>())
    bytes_opt operator()(const Number& lt) const {
        return lt.decompose(unwrap_num<Number>(do_cast(lt, arg1) - do_cast(lt, arg2)));
    }
};

template<typename Visitor>
bytes_opt visit_with(data_type out_type, data_type in1_type, data_type in2_type, const std::vector<bytes_opt>& values) {
    auto& a = values[0];
    auto& b = values[1];

    auto desa = in1_type->deserialize(*a);
    auto desb = in2_type->deserialize(*b);

    Visitor av{std::move(desa), std::move(desb)};
    return visit(*out_type, av);
}

shared_ptr<function>
make_add_fct(data_type out_type, data_type in1_type, data_type in2_type) {
    return make_native_scalar_function<true>("add", out_type, { in1_type, in2_type },
            [=] (cql_serialization_format sf, const std::vector<bytes_opt>& values) -> bytes_opt {
        return visit_with<add_visitor>(out_type, in1_type, in2_type, values);
    });
}

shared_ptr<function>
make_subtract_fct(data_type out_type, data_type in1_type, data_type in2_type) {
    return make_native_scalar_function<true>("subtract", out_type, { in1_type, in2_type },
            [=] (cql_serialization_format sf, const std::vector<bytes_opt>& values) -> bytes_opt {
        return visit_with<subtract_visitor>(out_type, in1_type, in2_type, values);
    });
}

}
