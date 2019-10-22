/*
 * Copyright (C) 2015 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "selectable.hh"
#include "selectable_with_field_selection.hh"
#include "field_selector.hh"
#include "writetime_or_ttl.hh"
#include "selector_factories.hh"
#include "cql3/functions/functions.hh"
#include "cql3/functions/castas_fcts.hh"
#include "cql3/functions/aggregate_fcts.hh"
#include "abstract_function_selector.hh"
#include "writetime_or_ttl_selector.hh"

namespace cql3 {

namespace selection {

assignment_testable::test_result
selectable::test_assignment(database& db, const sstring& keyspace, shared_ptr<column_specification> receiver) {
    data_type type = get_exact_type_if_known(db, keyspace);

    if (type == nullptr) {
        return assignment_testable::test_result::NOT_ASSIGNABLE;
    }
    if (type->is_reversed()) {
        type = type->underlying_type();
    }
    if (type->is_compatible_with(*(receiver->type))) {
        return assignment_testable::test_result::EXACT_MATCH;
    }
    if (type->is_value_compatible_with(*(receiver->type))) {
        return assignment_testable::test_result::WEAKLY_ASSIGNABLE;
    }

    return assignment_testable::test_result::NOT_ASSIGNABLE;
}

shared_ptr<selector::factory>
selectable::writetime_or_ttl::new_selector_factory(database& db, schema_ptr s, data_type expected_type, std::vector<const column_definition*>& defs, variable_specifications& bound_names) {
    auto&& def = s->get_column_definition(_id->name());
    if (!def || def->is_hidden_from_cql()) {
        throw exceptions::invalid_request_exception(format("Undefined name {} in selection clause", _id));
    }
    if (def->is_primary_key()) {
        throw exceptions::invalid_request_exception(
                format("Cannot use selection function {} on PRIMARY KEY part {}",
                              _is_writetime ? "writeTime" : "ttl",
                              def->name()));
    }
    if (def->type->is_multi_cell()) {
        throw exceptions::invalid_request_exception(format("Cannot use selection function {} on non-frozen collections",
                                                        _is_writetime ? "writeTime" : "ttl"));
    }

    return writetime_or_ttl_selector::new_factory(def->name_as_text(), add_and_get_index(*def, defs), _is_writetime);
}

sstring
selectable::writetime_or_ttl::to_string() const {
    return format("{}({})", _is_writetime ? "writetime" : "ttl", _id->to_string());
}

shared_ptr<selectable>
selectable::writetime_or_ttl::raw::prepare(database& db, schema_ptr s) {
    return make_shared<writetime_or_ttl>(_id->prepare_column_identifier(s), _is_writetime);
}

bool
selectable::writetime_or_ttl::raw::processes_selection() const {
    return true;
}

data_type
selectable::writetime_or_ttl::get_exact_type_if_known(database& db, const sstring& keyspace) const {
    return _is_writetime ? long_type : int32_type;
}

shared_ptr<selector::factory>
selectable::with_function::new_selector_factory(database& db, schema_ptr s, data_type expected_type, std::vector<const column_definition*>& defs, variable_specifications& bound_names) {
    auto&& factories = selector_factories::create_factories_and_collect_column_definitions(_args, _function->arg_types(), db, s, defs, bound_names);
    return abstract_function_selector::new_factory(_function, std::move(factories));
}

sstring
selectable::with_function::to_string() const {
    return format("{}({})", _function->name().name, join(", ", _args));
}

data_type
selectable::with_function::get_exact_type_if_known(database& db, const sstring& keyspace) const {
    return _function->return_type();
}

shared_ptr<selectable>
selectable::with_function::raw::prepare(database& db, schema_ptr s) {
    std::vector<shared_ptr<selectable>> prepared_args;
    prepared_args.reserve(_args.size());
    for (auto&& arg : _args) {
        prepared_args.push_back(arg->prepare(db, s));
    }
    auto&& fun = functions::functions::get(db, s->ks_name(), _function_name, prepared_args, s->ks_name(), s->cf_name());
    if (!fun) {
        throw exceptions::invalid_request_exception(format("Unknown function '{}'", _function_name));
    }
    if (!fun->return_type()) {
        throw exceptions::invalid_request_exception(format("Unknown function {} called in selection clause", _function_name));
    }

    return ::make_shared<with_function>(fun, std::move(prepared_args));
}

bool
selectable::with_function::raw::processes_selection() const {
    return true;
}

shared_ptr<selectable::with_function::raw>
selectable::with_function::raw::make_count_rows_function() {
    return ::make_shared<cql3::selection::selectable::with_function::raw>(
            cql3::functions::function_name::native_function(cql3::functions::aggregate_fcts::COUNT_ROWS_FUNCTION_NAME),
                    std::vector<shared_ptr<cql3::selection::selectable::raw>>());
}

shared_ptr<selector::factory>
selectable::with_anonymous_function::new_selector_factory(database& db, schema_ptr s, data_type expected_type, std::vector<const column_definition*>& defs, variable_specifications& bound_names) {
    auto&& factories = selector_factories::create_factories_and_collect_column_definitions(_args, _function->arg_types(), db, s, defs, bound_names);
    return abstract_function_selector::new_factory(_function, std::move(factories));
}

data_type
selectable::with_anonymous_function::get_exact_type_if_known(database& db, const sstring& keyspace) const {
    return _function->return_type();
}

sstring
selectable::with_anonymous_function::to_string() const {
    return format("{}({})", _function->name().name, join(", ", _args));
}

shared_ptr<selectable>
selectable::with_anonymous_function::raw::prepare(database& db, schema_ptr s) {
        std::vector<shared_ptr<selectable>> prepared_args;
        prepared_args.reserve(_args.size());
        for (auto&& arg : _args) {
            prepared_args.push_back(arg->prepare(db, s));
        }
        return ::make_shared<with_anonymous_function>(_function, std::move(prepared_args));
    }

bool
selectable::with_anonymous_function::raw::processes_selection() const {
    return true;
}

shared_ptr<selector::factory>
selectable::with_field_selection::new_selector_factory(database& db, schema_ptr s, data_type expected_type, std::vector<const column_definition*>& defs, variable_specifications& bound_names) {
    auto&& factory = _selected->new_selector_factory(db, s, expected_type, defs, bound_names);
    auto&& type = factory->get_column_specification(s)->type;
    auto&& ut = dynamic_pointer_cast<const user_type_impl>(type->underlying_type());
    if (!ut) {
        throw exceptions::invalid_request_exception(
                format("Invalid field selection: {} of type {} is not a user type",
                       _selected->to_string(), type->as_cql3_type()));
    }
    for (size_t i = 0; i < ut->size(); ++i) {
        if (ut->field_name(i) != _field->bytes_) {
            continue;
        }
        return field_selector::new_factory(std::move(ut), i, std::move(factory));
    }
    throw exceptions::invalid_request_exception(format("{} of type {} has no field {}",
                                                       _selected->to_string(), ut->as_cql3_type(), _field));
}

data_type
selectable::with_field_selection::get_exact_type_if_known(database& db, const sstring& keyspace) const {
    data_type selected_type = _selected->get_exact_type_if_known(db, keyspace);
    if (!selected_type) {
        return nullptr;
    }
    auto&& ut = dynamic_pointer_cast<const user_type_impl>(selected_type->underlying_type());
    if (!ut) {
        return nullptr;
    }

    auto column_names = ut->field_names();
    int i = 0;
    for (auto col : column_names) {
        if (col == _field->bytes_)
            break;
        i = i + 1;
    }
    return ut->field_type(i);
}

sstring
selectable::with_field_selection::to_string() const {
    return format("{}.{}", _selected->to_string(), _field->to_string());
}

shared_ptr<selectable>
selectable::with_field_selection::raw::prepare(database& db, schema_ptr s) {
    // static_pointer_cast<> needed due to lack of covariant return type
    // support with smart pointers
    return make_shared<with_field_selection>(_selected->prepare(db, s),
            static_pointer_cast<column_identifier>(_field->prepare(db, s)));
}

bool
selectable::with_field_selection::raw::processes_selection() const {
    return true;
}

shared_ptr<selector::factory>
selectable::with_cast::new_selector_factory(database& db, schema_ptr s, data_type expected_type, std::vector<const column_definition*>& defs, variable_specifications& bound_names) {
    std::vector<shared_ptr<selectable>> args{_arg};
    auto&& factories = selector_factories::create_factories_and_collect_column_definitions(args, std::vector<data_type>{}, db, s, defs, bound_names);
    auto&& fun = functions::castas_functions::get(db, _type.get_type(), args, s);

    return abstract_function_selector::new_factory(std::move(fun), std::move(factories));
}

sstring
selectable::with_cast::to_string() const {
    return format("cast({} as {})", _arg->to_string(), _type.to_string());
}

data_type
selectable::with_cast::get_exact_type_if_known(database& db, const sstring& keyspace) const {
    return _type.get_type();
}

shared_ptr<selectable>
selectable::with_cast::raw::prepare(database& db, schema_ptr s) {
    return ::make_shared<selectable::with_cast>(_arg->prepare(db, s), _type);
}

bool
selectable::with_cast::raw::processes_selection() const {
    return true;
}

std::ostream & operator<<(std::ostream &os, const selectable& s) {
    return os << s.to_string();
}

}

}
