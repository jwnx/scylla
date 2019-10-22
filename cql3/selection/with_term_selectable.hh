#pragma once

namespace cql3 {

namespace selection {

class selectable::with_term : public selectable {
    ::shared_ptr<term::raw> _raw_term;
public:
    with_term(::shared_ptr<term::raw> raw_term)
        : _raw_term(std::move(raw_term)) {
    }

    virtual sstring to_string() const override;
    virtual data_type get_exact_type_if_known(database& db, const sstring& keyspace) const override;
    virtual shared_ptr<selector::factory> new_selector_factory(database& db, schema_ptr schema, data_type expected_type, std::vector<const column_definition*>& defs, variable_specifications& bound_names) override;

    class raw : public selectable::raw {
        ::shared_ptr<term::raw> _raw_term;
    public:
        raw(shared_ptr<term::raw> raw_term)
            : _raw_term(std::move(raw_term)) {
        }
        virtual shared_ptr<selectable> prepare(database& db, schema_ptr s) override;
        virtual bool processes_selection() const override;
    };
};

}

}