#include "canonical_pdbs_heuristic.h"

#include "dominance_pruning.h"
#include "pattern_generator.h"
#include "utils.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../tasks/root_task.h"

#include "../utils/logging.h"
#include "../utils/timer.h"

#include <iostream>
#include <limits>
#include <memory>

using namespace std;

namespace pdbs {
static unique_ptr<CanonicalPDBs> get_canonical_pdbs_from_options(
    const shared_ptr<AbstractTask> &task, const Options &opts, utils::LogProxy &log) {
    shared_ptr<PatternCollectionGenerator> pattern_generator =
        opts.get<shared_ptr<PatternCollectionGenerator>>("patterns");

    utils::Timer timer;
    if (log.is_at_least_normal()) {
        log << "Initializing canonical PDB heuristic..." << endl;
    }
    PatternCollectionInformation pattern_collection_info =
        pattern_generator->generate(task);
    shared_ptr<PatternCollection> patterns =
        pattern_collection_info.get_patterns();
    /*
      We compute PDBs and pattern cliques here (if they have not been
      computed before) so that their computation is not taken into account
      for dominance pruning time.
    */
    shared_ptr<PDBCollection> pdbs = pattern_collection_info.get_pdbs();
    shared_ptr<vector<PatternClique>> pattern_cliques =
        pattern_collection_info.get_pattern_cliques();

    double max_time_dominance_pruning = opts.get<double>("max_time_dominance_pruning");
    if (max_time_dominance_pruning > 0.0) {
        int num_variables = TaskProxy(*task).get_variables().size();
        /*
          NOTE: Dominance pruning could also be computed without having access
          to the PDBs, but since we want to delete patterns, we also want to
          update the list of corresponding PDBs so they are synchronized.

          In the long term, we plan to have patterns and their PDBs live
          together, in which case we would only need to pass their container
          and the pattern cliques.
        */
        prune_dominated_cliques(
            *patterns,
            *pdbs,
            *pattern_cliques,
            num_variables,
            max_time_dominance_pruning,
            log);
    }

    dump_pattern_collection_generation_statistics(
        "Canonical PDB heuristic", timer(), pattern_collection_info, log);

    return utils::make_unique_ptr<CanonicalPDBs>(pdbs, pattern_cliques);
}

CanonicalPDBsHeuristic::CanonicalPDBsHeuristic(
    const Options &opts, unique_ptr<CanonicalPDBs> &&canonical_pdbs)
    : Heuristic(opts), canonical_pdbs(move(canonical_pdbs)) {
}

int CanonicalPDBsHeuristic::compute_heuristic(const GlobalState &ancestor_state) {
    // TODO for all decoupled search variants: for all leaves that are not affected
    // by any pattern, we need to increase h by the minimum price of any reached leaf state
    // This is only relevant when g-value adaptation is disabled, which is enabled by default.
    int h = canonical_pdbs->get_value(ancestor_state);
    if (h == numeric_limits<int>::max()) {
        return DEAD_END;
    } else {
        return h;
    }
}

void add_canonical_pdbs_options_to_parser(options::OptionParser &parser) {
    parser.add_option<shared_ptr<PatternCollectionGenerator>>(
        "patterns",
        "pattern generation method",
        "systematic(1)");
    parser.add_option<double>(
        "max_time_dominance_pruning",
        "The maximum time in seconds spent on dominance pruning. Using 0.0 "
        "turns off dominance pruning. Dominance pruning excludes patterns "
        "and additive subsets that will never contribute to the heuristic "
        "value because there are dominating subsets in the collection.",
        "infinity",
        Bounds("0.0", "infinity"));
    Heuristic::add_options_to_parser(parser);
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Canonical PDB",
        "The canonical pattern database heuristic is calculated as follows. "
        "For a given pattern collection C, the value of the "
        "canonical heuristic function is the maximum over all "
        "maximal additive subsets A in C, where the value for one subset "
        "S in A is the sum of the heuristic values for all patterns in S "
        "for a given state.");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    add_canonical_pdbs_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    utils::LogProxy log = utils::get_log_from_options(opts);

    return make_shared<CanonicalPDBsHeuristic>(
        opts, get_canonical_pdbs_from_options(tasks::g_root_task, opts, log));
}

static Plugin<Evaluator> _plugin("cpdbs", _parse, "heuristics_pdb");
}
