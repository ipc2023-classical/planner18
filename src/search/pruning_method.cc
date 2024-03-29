#include "pruning_method.h"

#include "plugin.h"

#include <cassert>

using namespace std;

static PluginTypePlugin<PruningMethod> _type_plugin(
    "PruningMethod",
    "Prune or reorder applicable operators.");
