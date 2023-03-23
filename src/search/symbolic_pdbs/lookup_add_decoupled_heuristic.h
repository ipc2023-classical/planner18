//
// Created by alvaro on 21/11/22.
//

#ifndef FAST_DOWNWARD_LOOKUP_ADD_DECOUPLED_HEURISTIC_H
#define FAST_DOWNWARD_LOOKUP_ADD_DECOUPLED_HEURISTIC_H


#include <cuddObj.hh>
#include <unordered_map>
#include <memory>
#include "../leaf_state_id.h"

class ExplicitStateCPG;
class GlobalState;

namespace options {
    class Options;
}

namespace symbolic {
    class SymVariables;
    class PricesADD;

    class LookupAddDecoupledHeuristic {
    protected:
        SymVariables * vars;
    public:
        void initialize(SymVariables * _vars) {
            vars = _vars;
            init();
        }

        virtual ~LookupAddDecoupledHeuristic() = default;

        virtual void init(){}

        [[nodiscard]] virtual int lookup(const ADD &heuristic, const GlobalState &state) const = 0;
    };

    class LookupAddDecoupledHeuristicADDOperations : public LookupAddDecoupledHeuristic{
        const bool debug;

        std::unique_ptr<PricesADD> prices_add_representation;
    public:
        explicit LookupAddDecoupledHeuristicADDOperations(const options::Options& opts);

        ~LookupAddDecoupledHeuristicADDOperations() override = default;
        void init() override;

        [[nodiscard]] int lookup(const ADD &heuristic, const GlobalState &state) const override;
    };

    class LookupAddDecoupledHeuristicExplicit : public LookupAddDecoupledHeuristic{

        // Caches all the leaves in the support of the ADD
        mutable std::unordered_map<DdNode *, std::vector<LeafFactorID>> covered_leaves_by_ADD;

    public:
        explicit LookupAddDecoupledHeuristicExplicit(const options::Options& opts);
        ~LookupAddDecoupledHeuristicExplicit() override = default;

        int lookup(const ADD &heuristic, const GlobalState &state) const override;

    protected:

        void compute_decoupled_value_explicit_member_states(const ADD &heuristic, const ExplicitStateCPG *prices,
                                                            const std::vector<LeafFactorID> &covered_leaves,
                                                            std::vector<int> &member_state, int &min_h, int sum_prices,
                                                            int index) const;
    };


    class LookupAddDecoupledHeuristicRecursive  : public LookupAddDecoupledHeuristic {
        std::vector<LeafFactorID> leaf_bdd_var; //For each bdd_variable indicates to what leaf it corresponds

        int lookup_recursive(DdNode *f, std::vector<int> &binary_assignment, const ExplicitStateCPG *prices,
                             std::unordered_map<DdNode *, int> &cache) const;
    public:
        explicit LookupAddDecoupledHeuristicRecursive(const options::Options& opts);
        ~LookupAddDecoupledHeuristicRecursive() override = default;

        void init() override;

        [[nodiscard]] int lookup(const ADD &heuristic, const GlobalState &state) const override;

        void set_binary_encoding_leaf(std::vector<int> &binary_assignment, LeafStateHash hash, LeafFactorID id) const;

        void set_binary_encoding_center(std::vector<int> &binary_assignment, const GlobalState &state) const;
    };

}

#endif //FAST_DOWNWARD_LOOKUP_ADD_DECOUPLED_HEURISTIC_H
