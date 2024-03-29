#include "opt_order.h"

#include "../task_utils/causal_graph.h"
#include "debug_macros.h"
#include "../globals.h"
#include "../leaf_state_id.h"
#include "../utils/rng.h"

#include <iostream>
#include <cassert>
#include <map>

using namespace std;


namespace symbolic {
//Returns a optimized variable ordering that reorders the variables
//according to the standard causal graph criterion
vector <int> InfluenceGraph::compute_gamer_ordering() {

    const causal_graph::CausalGraph &cg = causal_graph::get_causal_graph();

    vector<int> var_order;
    for (size_t v = 0; v < g_variable_domain.size(); v++) {
        var_order.push_back(v);
    }

    InfluenceGraph ig_partitions(g_variable_domain.size());
    for (size_t v = 0; v < g_variable_domain.size(); v++) {
        for (int v2 : cg.get_successors(v)) {
            if ((int)v != v2) {
                ig_partitions.set_influence(v, v2);
            }
        }
    }

    //Putting leaves by default at the end helped in some nomystery instance
    //TODO: It could be worth re-factoring variable ordering strategies and performing some optimizations
    std::reverse(var_order.begin(), var_order.end());
    ig_partitions.get_ordering(var_order);

    // cout << "Var ordering: ";
    // for(int v : var_order) cout << v << " ";
    // cout  << endl;

    return var_order;
}

vector<int> InfluenceGraph::compute_gamer_ordering_respecting_factors() {
        if(!g_factoring && g_leaves.empty()) {
            // we have the second condition for cases where we want to run explicit search with the
            // respecting ordering, i.e. g_factoring is null, but the leaves are set
            cout << "Warning: trying to use compute_gamer_ordering_respecting_factors without computing a factoring" << endl;
            return compute_gamer_ordering();
        }

        std::vector <int> factor_order; //Factor order contains an entry for each leaf factor, plus
        std::map <int,int> factor_to_center_variable;
        std::map <int,int> center_variable_to_factor;

        for (int factor = 0; factor < (int)(g_leaves.size()); ++ factor) {
            factor_order.push_back(factor);
        }

        for (size_t v = 0; v < g_variable_domain.size(); v++) {
            if(g_belongs_to_factor[v]  == LeafFactorID::CENTER) {
                int id = factor_order.size();
                factor_order.push_back(id);
                factor_to_center_variable[id] = v;
                center_variable_to_factor [v] = id;
            }
        }
        const causal_graph::CausalGraph &cg = causal_graph::get_causal_graph();

        InfluenceGraph ig_partitions(factor_order.size());
        for (size_t v = 0; v < g_variable_domain.size(); v++) {
            int factor1 =  g_belongs_to_factor[v] == LeafFactorID::CENTER ? center_variable_to_factor[v] : g_belongs_to_factor[v];
            for (int v2 : cg.get_successors(v)) {
                int factor2 =  g_belongs_to_factor[v2] == LeafFactorID::CENTER ? center_variable_to_factor[v2] : g_belongs_to_factor[v2];
                if (factor1 != factor2) {
                    ig_partitions.set_influence(factor1, factor2);
                }
            }
        }

        ig_partitions.get_ordering(factor_order);

        std::vector <int> var_order;

        for (int factor : factor_order) {
            if (factor < (int)(g_leaves.size())) {
                for (int var : g_leaves[factor]){
                    var_order.push_back(var);
                }
            } else {
                var_order.push_back(factor_to_center_variable[factor]);
            }
        }


        // cout << "Var ordering: ";
        // for(int v : var_order) cout << v << " ";
        // cout  << endl;

        return var_order;
}



//Returns a optimized variable ordering that reorders the variables
//according to the standard causal graph criterion
vector<vector <int>> InfluenceGraph::compute_factored_gamer_ordering() {
    assert(g_factoring); 

    vector<vector <int>> res = g_leaves;

    const causal_graph::CausalGraph &cg = causal_graph::get_causal_graph();

    for (LeafFactorID factor(0); factor < g_leaves.size(); ++factor) {
	InfluenceGraph ig_partitions(g_variable_domain.size());

	for (int v : g_leaves[factor]) {
	    for (int v2 : cg.get_successors(v)) {
		if (g_belongs_to_factor[v2] == factor) {
		    ig_partitions.set_influence(v, v2);
		}
	    }
	}

	ig_partitions.get_ordering(res[factor]);
    }
    return res;
    // cout << "Var ordering: ";
    // for(int v : var_order) cout << v << " ";
    // cout  << endl;
}




void InfluenceGraph::get_ordering(vector <int> &ordering) const {
    long value_optimization_function = optimize_variable_ordering_gamer(ordering, 50000);
    DEBUG_MSG(cout << "Value: " << value_optimization_function << endl;);

    for (int counter = 0; counter < 20; counter++) {
        vector <int> new_order;
        randomize(ordering, new_order); //Copy the order randomly
        long new_value = optimize_variable_ordering_gamer(new_order, 50000);

        if (new_value < value_optimization_function) {
            value_optimization_function = new_value;
            ordering.swap(new_order);
            DEBUG_MSG(cout << "New value: " << value_optimization_function << endl;);
        }
    }
}


void InfluenceGraph::randomize(const vector <int> &ordering, vector<int> &new_order) const {
    for (size_t i = 0; i < ordering.size(); i++) {
        int rnd_pos = rng.random(ordering.size() - i);
        int pos = -1;
        do {
            pos++;
            bool found;
            do {
                found = false;
                for (size_t j = 0; j < new_order.size(); j++) {
                    if (new_order[j] == ordering[pos]) {
                        found = true;
                        break;
                    }
                }
                if (found)
                    pos++;
            } while (found);
        } while (rnd_pos-- > 0);
        new_order.push_back(ordering[pos]);
    }
}



long InfluenceGraph::optimize_variable_ordering_gamer(vector <int> &order,
                                                      int iterations) const {
    long totalDistance = compute_function(order);

    long oldTotalDistance = totalDistance;
    //Repeat iterations times
    for (int counter = 0; counter < iterations; counter++) {
        //Swap variable
        int swapIndex1 = rng.random(order.size());
        int swapIndex2 = rng.random(order.size());
        if (swapIndex1 == swapIndex2)
            continue;

        //Compute the new value of the optimization function
        for (size_t i = 0; i < order.size(); i++) {
            if ((int)i == swapIndex1 || (int)i == swapIndex2)
                continue;

            if (influence(order[i], order[swapIndex1]))
                totalDistance += (-(i - swapIndex1) * (i - swapIndex1)
                                  + (i - swapIndex2) * (i - swapIndex2));

            if (influence(order[i], order[swapIndex2]))
                totalDistance += (-(i - swapIndex2) * (i - swapIndex2)
                                  + (i - swapIndex1) * (i - swapIndex1));
        }

        //Apply the swap if it is worthy
        if (totalDistance < oldTotalDistance) {
            int tmp = order[swapIndex1];
            order[swapIndex1] = order[swapIndex2];
            order[swapIndex2] = tmp;
            oldTotalDistance = totalDistance;

            /*if(totalDistance != compute_function(order)){
              cerr << "Error computing total distance: " << totalDistance << " " << compute_function(order) << endl;
              exit(-1);
            }else{
              cout << "Bien: " << totalDistance << endl;
            }*/
        } else {
            totalDistance = oldTotalDistance;
        }
    }
//  cout << "Total distance: " << totalDistance << endl;
    return totalDistance;
}



long InfluenceGraph::compute_function(const vector <int> &order) const {
    long totalDistance = 0;
    for (size_t i = 0; i < order.size() - 1; i++) {
        for (size_t j = i + 1; j < order.size(); j++) {
            if (influence(order[i], order[j])) {
                totalDistance += (i - j) * (i - j);
            }
        }
    }
    return totalDistance;
}


InfluenceGraph::InfluenceGraph(int num)  : rng(2023) //TODO: Allow setting random seed as parameter.
{
    influence_graph.resize(num);
    for (auto &i : influence_graph) {
        i.resize(num, 0);
    }
}



void InfluenceGraph::optimize_variable_ordering_gamer(vector <int> &order,
                                                      vector <int> &partition_begin,
                                                      vector <int> &partition_sizes,
                                                      int iterations) const {
    long totalDistance = compute_function(order);

    long oldTotalDistance = totalDistance;
    //Repeat iterations times
    for (int counter = 0; counter < iterations; counter++) {
        //Swap variable
        int partition = rng.random(partition_begin.size());
        if (partition_sizes[partition] <= 1)
            continue;
        int swapIndex1 = partition_begin[partition] + rng.random(partition_sizes[partition]);
        int swapIndex2 = partition_begin[partition] + rng.random(partition_sizes[partition]);
        if (swapIndex1 == swapIndex2)
            continue;

        //Compute the new value of the optimization function
        for (size_t i = 0; i < order.size(); i++) {
            if ((int)i == swapIndex1 || (int)i == swapIndex2)
                continue;

            if (influence(order[i], order[swapIndex1]))
                totalDistance += (-(i - swapIndex1) * (i - swapIndex1)
                                  + (i - swapIndex2) * (i - swapIndex2));

            if (influence(order[i], order[swapIndex2]))
                totalDistance += (-(i - swapIndex2) * (i - swapIndex2)
                                  + (i - swapIndex1) * (i - swapIndex1));
        }

        //Apply the swap if it is worthy
        if (totalDistance < oldTotalDistance) {
            int tmp = order[swapIndex1];
            order[swapIndex1] = order[swapIndex2];
            order[swapIndex2] = tmp;
            oldTotalDistance = totalDistance;

            /*if(totalDistance != compute_function(order)){
              cerr << "Error computing total distance: " << totalDistance << " " << compute_function(order) << endl;
              exit(-1);
            }else{
              cout << "Bien: " << totalDistance << endl;
            }*/
        } else {
            totalDistance = oldTotalDistance;
        }
    }
//  cout << "Total distance: " << totalDistance << endl;
}
}
