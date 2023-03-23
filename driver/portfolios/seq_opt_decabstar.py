OPTIMAL = True

CONFIGS = [
    (10, ["--decoupling", # decoupled PDBs: SCP, sys2hc, single leaf
           'lp_general(factoring_time_limit=30, memory_limit=7500, add_cg_sccs=true, strategy=mm_approx, min_flexibility=0.8)',
           "--search",
           "astar(max_scp_single_leaf(abstractions=[projections(patterns=multiple_cegar(total_max_time=100,pattern_filter=n_leaves_patterns(1))),projections(patterns=systematic(pattern_max_size=2,max_time=100,pattern_filter=n_leaves_patterns(1))),projections(patterns=hillclimbing(max_time=500,pattern_filter=n_leaves_patterns(1)))],max_time=200,saturator=all))"]),
    (10, ['--decoupling', # decoupled M&S: exact compliant sbMIASM
           'lp_general(factoring_time_limit=30, memory_limit=7500, add_cg_sccs=true, strategy=mm_approx, min_flexibility=0.8)',
           '--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_clustering(clustering_factory=clustering_compliant_factoring(),order_of_clusters=given,merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false,random_seed=2016)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,main_loop_max_time=900,decoupled_lookup=exact))']),
    (10, ['--decoupling', # decoupled symbolic PDBs
           'lp_general(factoring_time_limit=30, memory_limit=7500, add_cg_sccs=true, strategy=mm_approx, min_flexibility=0.8)',
           '--search',
           'astar(gamer_pdbs(respect_leaf_factoring=true, lookup=recursive))']),

    (10, ["--search", # explicit PDBs: SCP, sys2hc
           "astar(scp(abstractions=[projections(patterns=multiple_cegar(total_max_time=100)),projections(patterns=systematic(pattern_max_size=2,max_time=100)),projections(patterns=hillclimbing(max_time=500))],max_time=200,saturator=all))"]),
    (10, ["--search", # explicit M&S: sbMIASM
           "astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false,random_seed=2016)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,main_loop_max_time=900))"]),
    (10, ['--decoupling', # explicit compliant M&S: compliant sbMIASM
          'lp_general(factoring_time_limit=30, memory_limit=7500, add_cg_sccs=true, strategy=mm_approx, min_flexibility=0.8, abstain_type=COMPUTE_FACTORING_RUN_STANDARD)',
          '--search', 'astar(merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_clustering(clustering_factory=clustering_compliant_factoring(),order_of_clusters=given,merge_selector=score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false,random_seed=2016)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,main_loop_max_time=900))']),
    (10, ["--search", # explicit symbolic PDBs
           "astar(gamer_pdbs)"]),
    (10, ["--search", # blind search
           "astar(blind)"])
     ]
