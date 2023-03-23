#ifndef GLOBALS_H
#define GLOBALS_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#ifndef NDEBUG
// #define DEBUG_SEARCH    // general debugging
// #define DEBUG_FACTORING
// #define DEBUG_PRUNING
// #define DEBUG_PLAN_EXTRACTION
//#define DEBUG_SUCCESSOR_GENERATION
// #define DEBUG_NEW_SG
//#define DEBUG_ACTION_SPLITTING        // factoring of operators in SuccessorGenerator
// #define DEBUG_PRECOMPUTE_GOAL_COST
// #define DEBUG_POR
#endif

class Axiom;
class AxiomEvaluator;
class DomainTransitionGraph;
class Factoring;
namespace symmetries {
class GraphCreator;
}
namespace int_packer {
class IntPacker;
}
struct LeafFactorID;
class Operator;
class OperatorID;
class GlobalState;
class StateID;
namespace successor_generator {
class SuccessorGenerator;
}
namespace utils {
class Timer;
}
class StateRegistry;
class FactPair;

class MutexGroup;
bool test_goal(const GlobalState &state);
void save_plan(const std::vector<OperatorID> &plan, bool generates_multiple_plan_files);
int calculate_plan_cost(const std::vector<OperatorID> &plan);

void read_everything(std::istream &in);
void dump_everything();

bool is_unit_cost();
bool has_axioms();
void verify_no_axioms();
bool has_conditional_effects();
void verify_no_conditional_effects();
void verify_no_axioms_no_conditional_effects();

void check_magic(std::istream &in, std::string magic);

bool are_mutex(const FactPair &a, const FactPair &b);
void set_mutex(const FactPair &a, const FactPair &b);
int id_mutex(const FactPair &a, const FactPair &b);

//Alvaro: Substituted previous mutex data structures by two list of
//mutex groups (to iterate over invariants) and a vector of bools to
//implement are_mutex (with an auxiliary vector to get the fluent id)
//and the number of problem fluents
extern std::vector<MutexGroup> g_mutex_groups;
extern std::vector<bool> g_inconsistent_facts;
extern int g_num_facts;
extern std::vector<int> g_id_first_fact;

extern bool g_use_metric;
extern int g_min_action_cost;
extern int g_max_action_cost;

extern int MAX_DUPLICATE_COUNTER;

extern std::shared_ptr<Factoring> g_factoring;

// HACK this is used to increase the g-value of a node generated by the search
// that was created by a center operator that has preconditions on the leaf
// g-value is increased by the sum of the minimum precondition cost of each
// leaf factor that puts preconditions on the center operator
extern int g_inc_g_by;

extern std::vector<std::string> g_variable_name;
extern std::vector<int> g_variable_domain;

extern std::vector<int> g_center;
extern std::vector<std::vector<int> > g_leaves;
extern std::vector<LeafFactorID> g_belongs_to_factor;
// stores the index in g_center/g_leaves[factor] for each variable
extern std::vector<size_t> g_new_index;

extern std::vector<std::vector<std::string> > g_fact_names;
extern std::vector<int> g_axiom_layers;
extern std::vector<int> g_default_axiom_values;

extern int_packer::IntPacker *g_state_packer;
extern std::vector<int_packer::IntPacker*> g_leaf_state_packers;

// This vector holds the initial values *before* the axioms have been evaluated.
// Use the state registry to obtain the real initial state.
extern std::vector<int> g_initial_state_data;
// TODO The following function returns the initial state that is registered
//      in g_state_registry. This is only a short-term solution. In the
//      medium term, we should get rid of the global registry.
// in decoupled search, the state returned is a center state only.
extern const GlobalState &g_initial_state();

// in decoupled search, this only contains the center goals (if any)
extern std::vector<std::pair<int, int> > g_goal;
extern std::vector<std::vector<std::pair<int, int> > > g_goals_per_factor;
extern std::vector<std::pair<int, int> > g_all_goals;


extern std::vector<Operator> g_operators;
extern std::vector<Operator> g_axioms;
extern AxiomEvaluator *g_axiom_evaluator;
extern successor_generator::SuccessorGenerator *g_successor_generator;
extern std::vector<DomainTransitionGraph *> g_transition_graphs;
extern std::string g_plan_filename;
extern bool g_is_part_of_anytime_portfolio;
extern int g_num_previously_generated_plans;

// TODO move this to the search engine
extern std::shared_ptr<symmetries::GraphCreator> g_symmetry_graph;

// Only one global object for now. Could later be changed to use one instance
// for each problem in this case the method GlobalState::get_id would also have to be
// changed.
extern StateRegistry *g_state_registry;



#endif
