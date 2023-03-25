#include "clustering_compliant_factoring.h"

#include "../globals.h"

#include "../algorithms/sccs.h"
#include "../task_utils/causal_graph.h"
#include "../task_proxy.h"

#include "../plugin.h"

using namespace std;

namespace merge_and_shrink {
ClusteringCompliantFactoring::ClusteringCompliantFactoring()
    : Clustering() {
}

vector<vector<int>> ClusteringCompliantFactoring::compute(const TaskProxy &task_proxy) const {
    if (g_factoring) {
        vector <vector<int>> factoring(g_leaves);
        factoring.reserve(g_leaves.size() + g_center.size());
        for (int center_var: g_center) {
            factoring.push_back({center_var});
        }
        return factoring;
    } else {
        cout << "WARNING: no factoring found, use CG-SCC clustering instead of factoring clustering." << endl;
        VariablesProxy vars = task_proxy.get_variables();
        int num_vars = vars.size();

        // Compute clustering of the causal graph.
        vector<vector<int>> cg;
        cg.reserve(num_vars);
        for (VariableProxy var : vars) {
            const vector<int> &successors =
                    task_proxy.get_causal_graph().get_successors(var.get_id());
            cg.push_back(successors);
        }
        return sccs::compute_maximal_sccs(cg);
    }
}

static shared_ptr<Clustering>_parse(options::OptionParser &parser) {
    if (parser.dry_run() || parser.help_mode()) {
        return nullptr;
    }
    return make_shared<ClusteringCompliantFactoring>();
}

static options::Plugin<Clustering> _plugin("clustering_compliant_factoring", _parse);
}
