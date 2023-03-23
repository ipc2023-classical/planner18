#include "gamer_pdbs_heuristic.h"

#include "sym_pdb.h"
#include "../symbolic/uniform_cost_search.h"
#include "../symbolic/original_state_space.h"
#include "../symbolic/prices_ADD.h"
#include "lookup_add_decoupled_heuristic.h"

#include "../plugin.h"
#include "../task_utils/causal_graph.h"
#include "../tasks/root_task.h"
#include "../task_utils/task_properties.h"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>



using namespace std;

namespace symbolic {

PDBSearch::PDBSearch(GamerPDBsHeuristic * spdbheuristic_,
		             shared_ptr<SymStateSpaceManager> originalStateSpace) :
        spdbheuristic(spdbheuristic_),
        state_space(std::move(originalStateSpace)),
        average_hval(-1) {
    for(size_t i = 0; i < g_variable_domain.size(); ++i) {
	    pattern.insert(i);
    }
}

PDBSearch::PDBSearch (const set<int> & pattern_,
		      GamerPDBsHeuristic * spdbheuristic_,
		      const shared_ptr<OriginalStateSpace> & originalStateSpace) :
        spdbheuristic(spdbheuristic_),
        pattern(pattern_),
        average_hval(-1) {
    if (pattern.size() != g_variable_domain.size ()) {
        state_space = make_shared<SymPDB>(*originalStateSpace, pattern_);
    } else {
        state_space = originalStateSpace;
    }
}

std::ostream & operator<<(std::ostream &os, const PDBSearch & pdb){
    for (int v : pdb.get_pattern()) os << " " << v;
    return os;
}

std::ostream & operator<<(std::ostream &os, const set<int> & pattern){
    for (int v : pattern) os << " " << v;
    return os;
}
std::ostream & operator<<(std::ostream &os, const vector<int> & pattern){
    for (int v : pattern) os << " " << v;
    return os;
}

void PDBSearch::search(const SymParamsSearch & params, const utils::Timer & timer_heuristic_generation,
		       int generationTime, double generationMemory) {
    if (state_space->isOriginal()){
        uc_search = make_unique<UniformCostSearch> (params, spdbheuristic); //Pass spdbheuristic to store the bound
    } else  {
        uc_search = make_unique<UniformCostSearch> (params);
    }
    uc_search->init(state_space, false);

    while (!uc_search->finished() &&
           (generationTime == 0 || timer_heuristic_generation() < generationTime) &&
           (generationMemory == 0 || (spdbheuristic->getVars()->totalMemory()) < generationMemory) &&
           !spdbheuristic->solved()) {

        if(!uc_search->step()) break;
    }


    if(uc_search->finished() && !spdbheuristic->solved() && !uc_search->isAbstracted()) {
        cout << "Detected unsolvable by backward search on the original state space" << endl;
    }

    average_hval = uc_search->getClosed()->average_hvalue();

    //cout << "Finished PDB: " << *this << flush << "   Average value: "  << average_hval << " g_time: " << utils::g_timer() << endl;
}

vector<int> PDBSearch::candidate_vars() const {
    TaskProxy task_proxy(*(tasks::g_root_task));
    const causal_graph::CausalGraph &cg = task_proxy.get_causal_graph();
    vector<int> candidates;
    for (size_t var = 0; var < g_variable_domain.size(); ++var) {
        if (pattern.count(var)) continue;

        for (int succ : cg.get_pre_to_eff(var)) {
            if (pattern.count(succ)) {
                candidates.push_back(var);
                break;
            }
        }
    }

    cout<<"\ttime:"<<utils::g_timer()<<",Gamer-current_pattern:"<<pattern<<",candidates:"<<candidates<<endl;
    return candidates;
}

double PDBSearch::average_value() {
    if(!uc_search) return 0;
    if (average_hval == -1) {
        utils::Timer t;
        average_hval = uc_search->getClosed()->average_hvalue();
        cout << "Time average: " << t() << endl;
    }

    return average_hval;
}

ADD PDBSearch::getHeuristic() const {
    assert(uc_search);
    return uc_search->getClosed()->getHeuristic();
}

GamerPDBsHeuristic::GamerPDBsHeuristic(const Options &opts)
  : Heuristic(opts), SymController(opts),
    generationTime (opts.get<int> ("generation_time")),
    generationMemory (opts.get<double> ("generation_memory")),
    perimeter (opts.get<bool> ("perimeter")),
    gamer (opts.get<bool> ("gamer")) {

    utils::Timer timer_heuristic_generation;
    SymController::initialize();
    utils::Timer timer;
    cout << "Initializing gamer pdb heuristic..." << endl;


    if (opts.contains("lookup")){
        lookup_decoupled_strategy = opts.get<shared_ptr<LookupAddDecoupledHeuristic>>("lookup");
        lookup_decoupled_strategy->initialize(vars.get());
    }   else if (g_factoring) {
       cerr << "Error: when using decoupled search, cgamer_pdbs and perimeter heuristics require a lookup parameter" << endl;
       utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
    dump_options();

    //Get mutex fw BDDs to detect spurious states as dead ends

    cout << "Initialize original search" << endl;
    auto originalStateSpace = make_shared<OriginalStateSpace>(vars.get(), mgrParams);

    notMutexBDDs = originalStateSpace->getNotMutexBDDs(true);

    cout << "Use perimeter: " << perimeter << endl;

    if (perimeter) {
        PDBSearch pdb_search (this, originalStateSpace);

        pdb_search.search(searchParams, timer_heuristic_generation, generationTime, generationMemory);

        if(solved()) {
            cout << "Problem solved during heuristic generation" << endl;
            perimeter_heuristic = make_unique<ADD>(solution.getADD());
            return;
        }

        if(perimeter) {
            UniformCostSearch * search = pdb_search.get_search();
            perimeter_heuristic = make_unique<ADD>(search->getClosed()->getHeuristic());
            max_perimeter_heuristic = search->getClosed()->getHNotClosed();
        }
    }

    if (!gamer) {
        return;
    }
    //1) Get initial abstraction
    set<int> pattern;
    for (auto goal : g_all_goals) {
        pattern.insert(goal.first);
    }

    cout << "time:" << timer_heuristic_generation << " Create first PDB with only goal variables" << endl;

    //Create first pdb with only goal variables
    auto best_pdb = make_unique<PDBSearch>(pattern, this, originalStateSpace);

    best_pdb->search(searchParams, timer_heuristic_generation, generationTime, generationMemory);
    cout << "initial PDB (only goals):"<<*best_pdb<<",avg_value:"<<best_pdb->average_value()<<endl;
    cout << "time:"<<timer_heuristic_generation<<",Finished Initialize initial abstraction" << endl;

    while((generationTime == 0 || timer_heuristic_generation() < generationTime) &&
          (generationMemory == 0 || vars->totalMemory() < generationMemory) &&
          !solved()) {

        vector<unique_ptr<PDBSearch>> new_bests;
        double new_best_value = -1;

        //2) For every possible child of the abstraction
        //For each element interface empty partitions influencing our
        //already chosen partitions we try to remove it and generate a new PDB
        for (int var : best_pdb->candidate_vars()) {
            // 2a) Search the child
            set <int> child_pattern (best_pdb->get_pattern());
            child_pattern.insert(var);

            // 2b) Check if it is the best child so far
            auto new_pdb = make_unique<PDBSearch>(child_pattern, this, originalStateSpace);

            new_pdb->search(searchParams, timer_heuristic_generation, generationTime, generationMemory);

            // DEBUG_MSG(cout << "Search ended. Solution found: " << solution.solved() << endl;);

            if (solved()) {
                best_pdb = std::move(new_pdb);
                //cout << "Best PDB after solution found: " << *best_pdb << endl;
                new_bests.clear();
                break;
            }

            assert(child_pattern.size () < g_variable_domain.size() || lower_bound >= new_pdb->get_search()->getF());

            if (new_pdb->average_value() > best_pdb->average_value()) {
                cout << "child_pattern:"<<*new_pdb<<",Adding to best,new avg_value:"<<new_pdb->average_value()<<",old_average_value:"<<best_pdb->average_value() << endl;
                new_best_value = max(new_best_value, new_pdb->average_value());
                new_bests.push_back(std::move(new_pdb));
            }
            //else{ DEBUG_MSG(cout << "child_pattern:"<<*new_pdb<<",Not-Adding to best,new avg_value:"<<new_pdb->average_value()<<",old_average_value:"<<best_pdb->average_value() << endl;);}
        }
        cout<<"Finished checking all possible single variable improvements on previous best pdb:"<<*best_pdb<<",new_bests.size before clean:"<<new_bests.size()<<endl;

        if (new_bests.empty()) {
            break;
        }
        new_bests.erase(std::remove_if(
                new_bests.begin(),
                new_bests.end(),
                [new_best_value](unique_ptr<PDBSearch> & x){
                    return x->average_value() < 0.999*new_best_value;
                }), new_bests.end());
        for(size_t i=0;i<new_bests.size();i++) {
            cout<<"GamerPDBs,selected new_bests["<<i<<"]:"<<*new_bests[i]<<endl;
        }

        if (!solved() && new_bests.size() > 1) {
            set<int> new_pattern;
            for (auto & pdb : new_bests) {
                new_pattern.insert(pdb->get_pattern().begin(), pdb->get_pattern().end());
            }
            best_pdb = make_unique<PDBSearch>(new_pattern, this, originalStateSpace);

            best_pdb->search(searchParams, timer_heuristic_generation, generationTime, generationMemory);

            assert(new_pattern.size () < g_variable_domain.size() || lower_bound >= best_pdb->get_search()->getF());

            if(!solved() && best_pdb->average_value() < new_best_value) {
                for (auto & pdb : new_bests) {
                    if (pdb->average_value() == new_best_value) {
                        best_pdb = std::move(pdb);

                        break;
                    }
                }
            }
        } else {
            best_pdb = std::move(new_bests[0]);
            cout << "New best PDB: " << *best_pdb << endl;
        }
    }

    cout << "Final pdb: " << *best_pdb << endl;

    if(solved()){
        cout << "Problem solved during heuristic generation" << endl;
        pdb_heuristic = make_unique<ADD>(solution.getADD());
    } else {
        // if (perimeter) heuristic.reset(new ADD(best_pdb->getHeuristic(max_perimeter_heuristic)));
        //else
        pdb_heuristic = make_unique<ADD>(best_pdb->getHeuristic());
    }

    cout << "Done initializing Gamer PDB heuristic [" << timer << "] total memory: " << vars->totalMemory() << endl << endl;

    if(!pdb_heuristic) {
        cout << "Warning: heuristic could not be computed" << endl;
    }
}


int GamerPDBsHeuristic::compute_heuristic(const GlobalState &state)  {
    int res = 0;

    if (g_factoring) {
        assert (lookup_decoupled_strategy);
        //TODO: This is very inneficient if both perimeter_heuristic and db_heuristic are defined. Also, one could take the maximum of the two sym ADDS to obtain a more informative value
        if (perimeter_heuristic) {
            res = max(res, lookup_decoupled_strategy->lookup(*perimeter_heuristic, state));
        }

        if (res == std::numeric_limits<int>::max()) {
            return DEAD_END;
        }

        if(pdb_heuristic){
            res = max(res, lookup_decoupled_strategy->lookup(*pdb_heuristic, state));
        }
        if (res == std::numeric_limits<int>::max()) {
            return DEAD_END;
        }
    } else {
        int *inputs = vars->getBinaryDescription(state);
        for (const BDD &bdd: notMutexBDDs) {
            if (bdd.Eval(inputs).IsZero()) {
                return DEAD_END;
            }
        }

        if (perimeter_heuristic) {
            ADD evalNode = perimeter_heuristic->Eval(inputs);
            res = Cudd_V(evalNode.getRegularNode());
            if (res == -1 || evalNode == vars->plusInfinity()) return DEAD_END;

            //cout << "Perimeter: " << res << endl;
            if (res < max_perimeter_heuristic) {
                return res;
            }
        }

        if (pdb_heuristic) {
            ADD evalNode = pdb_heuristic->Eval(inputs);
            int abs_cost = Cudd_V(evalNode.getRegularNode());
            if (abs_cost == -1|| evalNode == vars->plusInfinity()){
                return DEAD_END;
            } else if (abs_cost > res) {
                res = abs_cost;
            }
        }
    }

    return res;
}

void GamerPDBsHeuristic::dump_options() const {
  cout << "Generation time: " << generationTime << endl;
  cout << "Generation memory: " << generationMemory << endl;
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    SymController::add_options_to_parser(parser, 30e3, 1e7);

    parser.add_option<int>("generation_time",
                           "maximum time used in heuristic generation",
                           "900",
                           Bounds("1", "infinity"));

    parser.add_option<double>("generation_memory",
                              "maximum memory used in heuristic generation",
                              to_string(3e9),
                              Bounds("1", "infinity"));

    parser.add_option<bool>("perimeter", "construct perimeter PDBs", "false");

    parser.add_option<shared_ptr<LookupAddDecoupledHeuristic>>("lookup", "Options are: {explicit, recursive,  ADD}", OptionParser::NONE);

    Options opts = parser.parse();
    opts.set("gamer", true);
    if (parser.help_mode())
        return nullptr;

    if (parser.dry_run()) {
        return nullptr;
    } else {
         return make_shared<GamerPDBsHeuristic> (opts);
    }
}


static shared_ptr<Heuristic> _parse_perimeter(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    SymController::add_options_to_parser(parser, 30e3, 1e7);

    parser.add_option<int>("generation_time",
                           "maximum time used in heuristic generation",
                           "900",
                           Bounds("1", "infinity"));

    parser.add_option<double>("generation_memory",
                              "maximum memory used in heuristic generation",
                              to_string(3e9),
                              Bounds("1", "infinity"));

    parser.add_option<shared_ptr<LookupAddDecoupledHeuristic>>("lookup", "Options are: {explicit, recursive,  ADD}", OptionParser::NONE);



    Options opts = parser.parse();
    opts.set("perimeter", true);
    opts.set("gamer", false);


    if (parser.help_mode())
        return nullptr;

    if (parser.dry_run()) {
        return nullptr;
    } else {
        return make_shared<GamerPDBsHeuristic> (opts);
    }
}


static Plugin<Evaluator> _plugin("gamer_pdbs", _parse);

static Plugin<Evaluator> _plugin_perimeter("perimeter", _parse_perimeter);

}
