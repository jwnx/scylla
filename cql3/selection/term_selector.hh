#pragma once

#include "selector.hh"
#include "selection.hh"


namespace cql3 {

namespace selection {

class term_selector : public selector {
    cql3::raw_value_view _value;
    uint32_t _idx;
    data_type _type;
public:
    static shared_ptr<selector::factory> new_factory(sstring column_name, ::shared_ptr<cql3::term> term, data_type type) {
        class term_selector_factory : public selector::factory {
            sstring _column_name;
            ::shared_ptr<cql3::term> _term;
            data_type _type;
        public:
            term_selector_factory(sstring column_name, ::shared_ptr<cql3::term> term, data_type type)
                : _column_name(std::move(column_name)), _term(std::move(term)), _type(type) {
            }

            virtual sstring column_name() override {
                return format("{}({})", _column_name, _type->name());
            }

            virtual data_type get_return_type() override {
                return _type;
            }

            virtual shared_ptr<selector> new_instance(const query_options& options) {
                return make_shared<term_selector>(_term->bind_and_get(options), _type);
            }

            virtual shared_ptr<selector> new_instance() override {
                return make_shared<term_selector>(cql3::raw_value_view::make_null(), _type);
            }

            void add_column_mapping(column_mapping mapping, column_specification spec);
        };
        
        return make_shared<term_selector_factory>(column_name, term, type);
    }

    virtual void reset() {
    }

    virtual data_type get_type() override {
        return _type;
    }

    virtual sstring assignment_testable_source_context() const override {
        return "";
    }

    virtual void add_input(cql_serialization_format sf, result_set_builder& rs) {
        return;
    }

    virtual bytes_opt get_output(cql_serialization_format sf) override {
        return data_value(int64_t(2)).serialize(); // to_bytes(_value);
    }

    term_selector(cql3::raw_value_view value, data_type type)
            : _value(std::move(value)), _type(type) {
    }
};

}

}
