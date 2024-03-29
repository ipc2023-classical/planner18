#ifndef PDBS_PDB_HEURISTIC_H
#define PDBS_PDB_HEURISTIC_H

#include "../heuristic.h"

#include "../leaf_state_id.h"

namespace options {
class Options;
}

namespace pdbs {
class PatternDatabase;

// Implements a heuristic for a single PDB.
class PDBHeuristic : public Heuristic {
    const bool polynomial_decoupled_pdb;
    std::shared_ptr<PatternDatabase> pdb;
    std::vector<LeafFactorID> affected_leaves;
    std::vector<std::vector<int>> affected_pattern_var_indices_by_leaf;
protected:
    virtual int compute_heuristic(const GlobalState &ancestor_state) override;
public:
    /*
      Important: It is assumed that the pattern (passed via Options) is
      sorted, contains no duplicates and is small enough so that the
      number of abstract states is below numeric_limits<int>::max()
      Parameters:
       operator_costs: Can specify individual operator costs for each
       operator. This is useful for action cost partitioning. If left
       empty, default operator costs are used.
    */
    PDBHeuristic(const options::Options &opts);
    virtual ~PDBHeuristic() override = default;
};
}

#endif
