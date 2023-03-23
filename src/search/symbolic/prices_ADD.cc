#include "prices_ADD.h"

#include <cassert>
#include <cuddObj.hh>
#include "../globals.h"
#include "../compliant_paths/explicit_state_cpg.h"
#include "../compliant_paths/cpg_storage.h"
#include "sym_variables.h"


using namespace std;


namespace symbolic {

    PricesADD::PricesADD(SymVariables * _vars) : vars(_vars){
        assert(g_factoring);

        precomputed_leaf_state_ADDs.resize(g_leaves.size());

        const auto & variable_order = vars->get_variable_order();
        vector<bool> considered_leaves (g_leaves.size(), false);

        for(int i = variable_order.size() - 1; i >= 0; i--) {
            int var = variable_order [i];
            LeafFactorID leaf_factor = g_belongs_to_factor[var];

            if (leaf_factor != LeafFactorID::CENTER){
                if (!considered_leaves[leaf_factor]) {
                    considered_leaves[leaf_factor] = true;
                    factor_order.push_back(Factor(leaf_factor));
                }
            } else {
                factor_order.push_back(Factor(var));
            }
        }
    }


            //Return an ADD representing the price for each leaf state
    ADD PricesADD::get_leaf_prices_ADD(const ExplicitStateCPG *prices, LeafFactorID leaf) {
        ADD result = vars->plusInfinity();
        int num_reached_lstates = prices->get_number_states(leaf);
        for (LeafStateHash id(0); id < g_state_registry->size(leaf); ++id) {
            if (prices->has_leaf_state(id, leaf)) {
                int cost = prices->get_cost_of_state(id, leaf);

                ADD leaf_state_ADD = get_leaf_state_ADD(id, leaf) + vars->getADD(cost);
                assert(Cudd_V(leaf_state_ADD.FindMin().getRegularNode()) == cost );
                assert(isinf(Cudd_V(leaf_state_ADD.FindMax().getRegularNode()) ));

                result = result.Minimum(leaf_state_ADD);
                if (--num_reached_lstates == 0) {
                    break;
                }
            }
        }

        //cout << "Min: " << Cudd_V(result.FindMin().getRegularNode()) << endl;
        //cout << "Max: " << Cudd_V(result.FindMax().getRegularNode()) << endl;

        assert(Cudd_V(result.FindMin().getRegularNode()) >= 0 );

        return result;
    }


    //Return an ADD where leaf state -> 0 else -> infinity (we can then set any cost for the leaf state by adding a constant)
    ADD PricesADD::get_leaf_state_ADD(LeafStateHash id, LeafFactorID factor) {

        assert (factor < precomputed_leaf_state_ADDs.size());

        //precompute an ADD where leaf state -> 0 else -> infinity (we can then set any cost for the leaf state by adding a constant)
        while (precomputed_leaf_state_ADDs[factor].size() <= id) {
            LeafStateHash new_id {precomputed_leaf_state_ADDs[factor].size()};

            LeafState lstate = g_state_registry->lookup_leaf_state(new_id, factor);
            BDD res = vars->oneBDD();
            for (int leaf_var: g_leaves[factor]) {
                res *= vars->preBDD(leaf_var, lstate[leaf_var]);
            }

            precomputed_leaf_state_ADDs[factor].push_back((!res).Add() * vars->plusInfinity());
            assert(Cudd_V(precomputed_leaf_state_ADDs[factor][new_id].FindMin().getRegularNode()) == 0);
            assert(isinf(Cudd_V(precomputed_leaf_state_ADDs[factor][new_id].FindMax().getRegularNode())));
        }

        return precomputed_leaf_state_ADDs[factor][id];
    }

    ADD PricesADD::get_member_states_price(const GlobalState &state) {
        // Compute all reached facts (list of values by var) for both leaf and center vars.
        const ExplicitStateCPG *prices = dynamic_cast<const ExplicitStateCPG*>(CPGStorage::storage->get_cpg(state));

        ADD result = vars->getADD(0);


        for(const auto & factor : factor_order) {
            if (factor.is_leaf()) {
                result += get_leaf_prices_ADD(prices, factor.get_leaf_factor());
            } else {
                BDD center_bdd  =   vars->preBDD(factor.get_center_variable(), state[factor.get_center_variable()]);
                result += ((!center_bdd).Add())*(vars->plusInfinity());
            }
        }

        return result;
    }

} // symbolic
