#include "goal_count_heuristic.h"

#include "state.h"
#include "globals.h"
#include "option_parser.h"
#include "plugin.h"

using namespace std;


GoalCountHeuristic::GoalCountHeuristic(const Options &opts)
    : Heuristic(opts) {
}

GoalCountHeuristic::~GoalCountHeuristic() {
}

void GoalCountHeuristic::initialize() {
    cout << "Initializing goal count heuristic..." << endl;
}

int GoalCountHeuristic::compute_heuristic(const GlobalState &state) {
    int unsatisfied_goal_count = 0;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int var = g_goal[i].first, value = g_goal[i].second;
        if (state[var] != value)
            ++unsatisfied_goal_count;
    }
    return unsatisfied_goal_count;
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis("Goal count heuristic", "");
    parser.document_language_support("action costs", "ignored by design");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support("axioms", "supported");
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return make_shared<GoalCountHeuristic>(opts);
}

static Plugin<Evaluator> _plugin("goalcount", _parse);
