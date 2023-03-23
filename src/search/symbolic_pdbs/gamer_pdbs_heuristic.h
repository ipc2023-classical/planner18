#ifndef SYMBOLIC_PDBS_GAMER_PDBS_HEURISTIC_H
#define SYMBOLIC_PDBS_GAMER_PDBS_HEURISTIC_H

#include "../symbolic/sym_controller.h"
#include "../heuristic.h"
#include "../symbolic/sym_solution.h"


namespace symbolic {

class SymStateSpaceManager;
class SymParamsSearch;
class UniformCostSearch;
class GamerPDBsHeuristic;
class LookupAddDecoupledHeuristic;

class PDBSearch {
    GamerPDBsHeuristic * spdbheuristic;
    std::set<int> pattern;
    std::shared_ptr <SymStateSpaceManager> state_space;
    std::unique_ptr <UniformCostSearch> uc_search;
    double average_hval;

public:

    PDBSearch (GamerPDBsHeuristic * spdbheuristic,
	       std::shared_ptr<SymStateSpaceManager> originalStateSpace);

    PDBSearch (const std::set<int> & pattern,
	       GamerPDBsHeuristic * spdbheuristic,
	       const std::shared_ptr<OriginalStateSpace> & originalStateSpace);


    void search(const SymParamsSearch & searchParams, const utils::Timer & timer_heuristic_generation, int generationTime = 0, double generationMemory = 0);

    ADD getHeuristic() const;
    double average_value();

    const std::set<int>  & get_pattern() const {
	    return pattern;
    }

    std::vector<int> candidate_vars() const;

    UniformCostSearch * get_search() {
	    return uc_search.get();
    }
};

std::ostream & operator<<(std::ostream &os, const PDBSearch & pdb);

class GamerPDBsHeuristic : public Heuristic, public SymController {
    const int generationTime;
    const double generationMemory;
    const bool perimeter;
    const bool gamer;

    int max_perimeter_heuristic{};
    std::unique_ptr<ADD> perimeter_heuristic;
    std::unique_ptr<ADD> pdb_heuristic;
    std::vector<BDD> notMutexBDDs;

    std::shared_ptr<LookupAddDecoupledHeuristic> lookup_decoupled_strategy;

    void dump_options() const;

protected:
    virtual int compute_heuristic(const GlobalState &state) override;

public:
    GamerPDBsHeuristic(const options::Options &opts);
    virtual ~GamerPDBsHeuristic() = default;
};

}

#endif
