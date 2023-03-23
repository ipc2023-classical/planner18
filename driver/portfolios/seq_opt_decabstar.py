OPTIMAL = True

CONFIGS = [
    (300, ["--decoupling",
           "fork(search_type=asda, pruning=cost_frontier(irrelevance=TRANSITIONS), max_leaf_size=1000000)",
           "--search",
           "astar(TODO, "
           "      pruning_heuristic=lmcut(search_type=STAR))"]),
    (800, ["--decoupling",
           "lp_general(factoring_time_limit=30, memory_limit=7500, add_cg_sccs=true, strategy=mm_approx, min_flexibility=0.8)",
           "--search",
           "astar(TODO)"]),
    (300, ["--search",
           "astar(TODO PDB config)"]),
    (300, ["--search",
           "astar(TODO M&S config)"]),
    (400, ["--search",
           "astar(blind)"])
     ]
