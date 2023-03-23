#ifndef FAST_DOWNWARD_PRICES_ADD_H
#define FAST_DOWNWARD_PRICES_ADD_H

#include "../leaf_state_id.h"
#include "../compliant_paths/explicit_state_cpg.h"

class ADD;
class GlobalState;

namespace symbolic {
    class SymVariables;

    class PricesADD {
        // Internal data structure used to identify the order in which the price ADD needs to be constructed.
        // Factors are a pair of bool and int
        class Factor {
            bool _is_leaf;
            int _leaf_id_or_center_var;

        public:
            Factor(LeafFactorID leaf_id) : _is_leaf (true), _leaf_id_or_center_var(leaf_id) {
            }

            Factor(int center_var) : _is_leaf (false), _leaf_id_or_center_var(center_var) {
            }

            bool is_leaf () const {
                return _is_leaf;
            }

            LeafFactorID get_leaf_factor() const {
                assert(is_leaf());
                return LeafFactorID(_leaf_id_or_center_var);
            }

            int get_center_variable() const {
                assert(!is_leaf());
                return _leaf_id_or_center_var;
            }

        };


        SymVariables * vars;

        std::vector<std::vector<ADD>> precomputed_leaf_state_ADDs;

        std::vector<Factor> factor_order;

        //Return an ADD where leaf state -> 0 else -> infinity (we can then set any cost for the leaf state by adding a constant)
        ADD get_leaf_state_ADD(LeafStateHash id, LeafFactorID factor);

    public:
        explicit PricesADD (SymVariables * vars);

        ADD get_member_states_price(const GlobalState &state);

        ADD get_leaf_prices_ADD(const ExplicitStateCPG *prices, LeafFactorID leaf);
    };

} // symbolic

#endif //FAST_DOWNWARD_PRICES_ADD_H
