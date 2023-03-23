#include "sym_solution.h"

#include <vector>       // std::vector
#include "../state_registry.h"
#include "sym_search.h"


using namespace std;

namespace symbolic {
    void SymSolution::getPlan(vector <const Operator *> &path) const {
	assert (path.empty()); //This code should be modified to allow appending things to paths
	// DEBUG_MSG(cout << "Extract path forward: " << g << endl; );
	if (exp_fw) {
	    exp_fw->getPlan(cut, g, h, path);
	}

	// DEBUG_MSG(cout << "Extract path backward: " << h << endl; );

	if (exp_bw) {
	    BDD newCut;
	    if (!path.empty()) {
		vector<int> s = g_initial_state_data;
		//Get state
		for (auto op : path) {
		  vector<int> new_s = s;
		  for (const Effect &eff : op->get_effects()) {
		    if (eff.does_fire(s)) {
		      new_s[eff.var] = eff.val;
		    }
		  }
		  s.swap(new_s);
		}
		newCut = exp_bw->getStateSpace()->getVars()->getStateBDD(s);
	    } else {
		newCut = cut;
	    }

	    exp_bw->getPlan(newCut, g, h, path);
	}
	// DEBUG_MSG(cout << "Path extracted" << endl;
	  // State s2 (*g_initial_state);
	  // //Get state
	  // for(auto op : path){
	  // cout << op->get_name() << endl;
	  // if(!op->is_applicable(s2)){
	  // cout << "ERROR: bad plan reconstruction" << endl;
	  // cout << op->get_name() << " is not applicable" << endl;
	  // exit(-1);
	  // }
	  // s2 = State(s2, *op);
	  // }
	  // if(!test_goal(s2)){
	  // cout << "ERROR: bad plan reconstruction" << endl;
	  // cout << "The plan ends on a non-goal state" << endl;
	  // exit(-1);
	  // //}
	  //   );
    }

    ADD SymSolution::getADD() const {
	// if(!solved()) {
	//     return vars->getADD(-1);;
	// }

	assert(exp_fw || exp_bw);
	vector <const Operator *> path;
	getPlan(path);

	SymVariables *vars = nullptr;
	if(exp_fw) vars = exp_fw->getStateSpace()->getVars();
	else if(exp_bw) vars = exp_bw->getStateSpace()->getVars();

    map<int, BDD> heuristicBDDs;
	int h_val = g + h;

	vector<int> s = g_initial_state_data;
	BDD sBDD = vars->getStateBDD(s);
	heuristicBDDs[h_val] = sBDD;
	for (auto op : path) {
	    h_val -= op->get_cost();
	    vector<int> new_s = s;
	    for (const Effect &eff : op->get_effects()) {
	      if (eff.does_fire(s)) {
		new_s[eff.var] = eff.val;
	      }
	    }
	    s.swap(new_s);
	    sBDD = vars->getStateBDD(s);
        if (heuristicBDDs.count(h_val)) {
            heuristicBDDs[h_val] += sBDD;
        } else {
            heuristicBDDs[h_val] = sBDD;
        }
    }
	return vars->getADD(heuristicBDDs);
    }
}
