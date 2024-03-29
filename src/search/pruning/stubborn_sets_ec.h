#ifndef STUBBORN_SETS_EC_H
#define STUBBORN_SETS_EC_H

#include "stubborn_sets.h"

class Operator;

namespace stubborn_sets_ec {
class StubbornSetsEC : public stubborn_sets::StubbornSets {
private:
    std::vector<std::vector<std::vector<bool>>> reachability_map;
    std::vector<std::vector<int>> op_preconditions_on_var;
    std::vector<bool> active_ops;
    std::vector<std::vector<OperatorID>> conflicting_and_disabling;
    std::vector<std::vector<OperatorID>> disabled;
    std::vector<bool> written_vars;
    std::vector<std::vector<bool>> nes_computed;

    void get_disabled_vars(OperatorID op1_no, OperatorID op2_no, std::vector<int> &disabled_vars);
    void build_reachability_map();
    void compute_operator_preconditions();
    void compute_conflicts_and_disabling();
    void compute_disabled_by_o();
    void add_conflicting_and_disabling(OperatorID op_no, const GlobalState &state);
    void compute_active_operators(const GlobalState &state);
    void mark_as_stubborn_and_remember_written_vars(OperatorID op_no, const GlobalState &state);
    void add_nes_for_fact(const std::pair<int, int> &fact, const GlobalState &state);
    void apply_s5(const Operator &op, const GlobalState &state);
protected:
    virtual void initialize_stubborn_set(const GlobalState &state) override;
    virtual void handle_stubborn_operator(const GlobalState &state, OperatorID op_no) override;
public:
    StubbornSetsEC(const options::Options &opts);
    virtual ~StubbornSetsEC() = default;

    virtual void initialize() override {}
};
}
#endif
