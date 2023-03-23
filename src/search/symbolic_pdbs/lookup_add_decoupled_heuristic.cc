#include "lookup_add_decoupled_heuristic.h"

#include "cuddInt.h"
#include <unordered_map>
#include <vector>
#include <memory>

#include "../plugin.h"

#include "../symbolic/sym_variables.h"
#include "../symbolic/prices_ADD.h"

#include "../compliant_paths/cpg_storage.h"
using namespace std;

namespace symbolic {

    int LookupAddDecoupledHeuristicRecursive::lookup(const ADD &heuristic, const GlobalState &state) const {
        vector<int> binary_assignment(vars->getNumBDDVars()*2, -1);
        set_binary_encoding_center(binary_assignment, state);
        const auto *prices = dynamic_cast<const ExplicitStateCPG *>(CPGStorage::storage->get_cpg(state));
        std::unordered_map<DdNode *, int> cache;
        return lookup_recursive(heuristic.getNode(), binary_assignment, prices, cache);
    }

    int LookupAddDecoupledHeuristicRecursive::lookup_recursive(DdNode *f, vector<int> &binary_assignment,
                                                      const ExplicitStateCPG *prices,
                                                      std::unordered_map<DdNode *, int> &cache) const {
        // in cache for all nodes, we store the minimum heuristic + price of all the leafs below
        // prices gives us the price of each leaf
        // state gives us the value for center variables
        assert(Cudd_Regular(f) == f); //Check that all nodes are not complemented (as we are in an ADD)

        //Base case
        if (cuddIsConstant(f)) {
            if (f == vars->plusInfinity().getNode()) {
                return std::numeric_limits<int>::max();
            } else {
                return cuddV(f);
            }
        }

        //Recursive case
        int bdd_var = f->index;
        // If the variable has already an assigned value, then go down that path
        if (binary_assignment[bdd_var] != -1) {
            // just go down via this path
            DdNode *next = binary_assignment[bdd_var] == 1 ? cuddT(f) : cuddE(f);
            return lookup_recursive(next, binary_assignment, prices, cache);
        }

        if (cache[f]) {
            return cache[f];
        }

        // The assignment does not have a value because is related to a leaf.
        // We iterate over all leaf states and pick the minimum value
        LeafFactorID leaf = leaf_bdd_var[bdd_var];
        int result = std::numeric_limits<int>::max();
        size_t num_reached_lstates = prices->get_number_states(leaf);
        for (LeafStateHash id(0); id < g_state_registry->size(leaf); ++id) {
            if (prices->has_leaf_state(id, leaf)) {
                int cost = prices->get_cost_of_state(id, leaf);
                set_binary_encoding_leaf(binary_assignment, id, leaf);

                DdNode *next = binary_assignment[bdd_var] ? cuddT(f) : cuddE(f);

                int recursive_result = lookup_recursive(next, binary_assignment, prices, cache);

                if (recursive_result < std::numeric_limits<int>::max()) {
                    result = min(result, cost + recursive_result);
                }

                if (--num_reached_lstates == 0) {
                    break;
                }
            }
        }
        //reset binary_assignment[bdd_var] = -1 for all bdd_vars in leaf
        for (int fd_var : g_leaves[leaf]) {
            for (int bdd_v : vars->vars_index_pre(fd_var)) {
                binary_assignment[bdd_v] = -1;
            }
        }

        cache[f] = result;
        return result;
    }


    void set_binary_encoding (SymVariables * vars, vector<int> &binary_assignment, int fd_var, int value) {
        int pos = 0;
        for (int bdd_var :  vars->vars_index_pre(fd_var)) {
            binary_assignment[bdd_var] = ((value >> pos) % 2);
            pos++; //Skip interleaving variable
        }

    }

    void
    LookupAddDecoupledHeuristicRecursive::set_binary_encoding_leaf(vector<int> &binary_assignment, LeafStateHash leaf_state_id, LeafFactorID leaf_id) const {
        LeafState l_state = g_state_registry->lookup_leaf_state(leaf_state_id, leaf_id);
        for (int leaf_var : g_leaves[leaf_id]){
            set_binary_encoding(vars, binary_assignment, leaf_var, l_state[leaf_var]);
        }
    }

    void LookupAddDecoupledHeuristicRecursive::set_binary_encoding_center(vector<int> &binary_assignment,
                                                                 const GlobalState &state) const {
        for (int center_var : g_center) {
            set_binary_encoding(vars, binary_assignment, center_var, state[center_var]);
        }
    }

    LookupAddDecoupledHeuristicRecursive::LookupAddDecoupledHeuristicRecursive(const Options & ) {

    }


    void LookupAddDecoupledHeuristicRecursive::init() {
        leaf_bdd_var.reserve(vars->getNumBDDVars()*2);

        for(int var = 0; var < vars->getNumBDDVars()*2; ++var) {
            leaf_bdd_var.push_back(g_belongs_to_factor[vars->getFDVar(var)]);
        }
    }


    int LookupAddDecoupledHeuristicADDOperations::lookup(const ADD &heuristic, const GlobalState &state) const {
        ADD prices = prices_add_representation->get_member_states_price(state);

        ADD sum = prices + heuristic;

        if (debug) {

            cout << "Heuristic: "; heuristic.print(vars->getNumBDDVars(), 1);
            cout << "\nMin heuristic: " << Cudd_V(heuristic.FindMin().getRegularNode()) << "\nMax heuristic: " << Cudd_V(heuristic.FindMax().getRegularNode()) << "\n\n";

            cout << "Prices: "; prices.print(vars->getNumBDDVars(), 1);
            cout << "\nMin price: " << Cudd_V(prices.FindMin().getRegularNode()) << "\nMax price: " << Cudd_V(prices.FindMax().getRegularNode()) << "\n\n";

            cout << "Sum: "; sum.print(vars->getNumBDDVars(), 1);
            cout << "\nMin sum: " << Cudd_V(sum.FindMin().getRegularNode()) << "\nMax sum: " << Cudd_V(sum.FindMax().getRegularNode()) << "\n\n";
        }

        ADD evalNode = sum.FindMin();
        if (evalNode == vars->plusInfinity()) {
            return std::numeric_limits<int>::max();
        }
        int abs_cost = (int)(Cudd_V(evalNode.getRegularNode()));
        assert (abs_cost >= 0);
        return abs_cost;
    }

    LookupAddDecoupledHeuristicADDOperations::LookupAddDecoupledHeuristicADDOperations(const Options &opts) : debug (opts.get<bool>("debug")){
    }

    void LookupAddDecoupledHeuristicADDOperations::init() {
        prices_add_representation = std::make_unique<PricesADD>(vars);
    }


    int LookupAddDecoupledHeuristicExplicit::lookup(const ADD &heuristic, const GlobalState &state) const {

        auto & covered_leaves = covered_leaves_by_ADD [heuristic.getNode()];

        if (covered_leaves.empty()) { //TODO: Possible performance issue
            std::vector <bool> support_fd_vars (g_variable_domain.size(), false);
            for (unsigned int bdd_var : heuristic.SupportIndices()) {
                int fd_var = vars->getFDVar((int)bdd_var);
                support_fd_vars[fd_var] = true;
            }

            for (LeafFactorID leaf_id(0); leaf_id < g_leaves.size(); ++leaf_id){
                for (int var : g_leaves[leaf_id]){
                    if (support_fd_vars[var]){
                        covered_leaves.push_back(leaf_id);
                        break;
                    }
                }
            }
        }

        const auto *prices = dynamic_cast<const ExplicitStateCPG*>(CPGStorage::storage->get_cpg(state));
        vector<int> member_state(g_variable_domain.size(), -1);
        for (int var : g_center){
            member_state[var] = state[var];
        }
        int min_h = std::numeric_limits<int>::max();

        compute_decoupled_value_explicit_member_states(heuristic, prices, covered_leaves, member_state, min_h, 0, 0);
        return min_h;
    }


    void LookupAddDecoupledHeuristicExplicit::compute_decoupled_value_explicit_member_states(
            const ADD &heuristic,
            const ExplicitStateCPG *prices,
            const vector<LeafFactorID> &covered_leaves,
            vector<int> &member_state,
            int &min_h,
            int sum_prices,
            int index) const {

        if (sum_prices >= min_h){
            // cannot possibly improve heuristic
            return;
        }

        if (index == static_cast<int>(covered_leaves.size())){
            int *inputs = vars->getBinaryDescription(member_state);

            ADD evalNode = heuristic.Eval(inputs);
            int abs_cost = (int)(Cudd_V(evalNode.getRegularNode()));
            if (abs_cost >= 0 && evalNode != vars->plusInfinity()) {
                min_h = min(min_h, sum_prices + abs_cost);
            }

            return;
        }

        LeafFactorID leaf = covered_leaves[index];
        size_t num_reached_l_states = prices->get_number_states(leaf);
        for (LeafStateHash l_id(0); l_id < g_state_registry->size(leaf); ++l_id){
            if (prices->has_leaf_state(l_id, leaf)){
                LeafState l_state = g_state_registry->lookup_leaf_state(l_id, leaf);
                for (int var : g_leaves[leaf]){
                    member_state[var] = l_state[var];
                }
                int price = prices->get_cost_of_state(l_id, leaf);
                compute_decoupled_value_explicit_member_states(heuristic,
                                                               prices,
                                                               covered_leaves,
                                                               member_state,
                                                               min_h,
                                                               sum_prices + price,
                                                               index + 1);
                if (--num_reached_l_states == 0){
                    break;
                }
            }
        }
    }

    LookupAddDecoupledHeuristicExplicit::LookupAddDecoupledHeuristicExplicit(const Options &) {

    }



    static options::PluginTypePlugin<LookupAddDecoupledHeuristic> _type_plugin(
            "LookupAddDecoupledHeuristic",
            "Doc");

    template<typename T>
    static shared_ptr<LookupAddDecoupledHeuristic> _parse(OptionParser &parser) {
        parser.add_option<bool>("debug", "Debug options", "false");
        Options opts = parser.parse();

        if (parser.help_mode() || parser.dry_run()) {
            return nullptr;
        } else {
            return make_shared<T> (opts);
        }
    }

    static Plugin<LookupAddDecoupledHeuristic> _plugin_1("explicit", _parse<LookupAddDecoupledHeuristicExplicit>);
    static Plugin<LookupAddDecoupledHeuristic> _plugin_2("add_ops", _parse<LookupAddDecoupledHeuristicADDOperations>);
    static Plugin<LookupAddDecoupledHeuristic> _plugin_3("recursive", _parse<LookupAddDecoupledHeuristicRecursive>);
}