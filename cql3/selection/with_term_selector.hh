#pragma once

namespace cql3 {

namespace selection {

class selectable::with_term : public selectable {
    static inline thread_local const column_identifier _bind_marker_name_in_selection = column_identifier("[selection]", true);
    shared_ptr<term::raw> _raw_term;
public:
    with_term(shared_ptr<term::raw> t)
        : _raw_term(t) {
    }

    virtual sstring to_string() const override;
    virtual shared_ptr<selector::factory> new_selector_factory(database& db, schema_ptr s, std::vector<const column_definition*>& defs) override;

    class raw : public selectable::raw {
        shared_ptr<term::raw> _raw_term;
    public:
        raw(shared_ptr<term::raw> t)
                : _raw_term(std::move(t)) {
        }
        virtual shared_ptr<selectable> prepare(schema_ptr s) override;
        virtual bool processes_selection() const override;
    };
};

}

}
