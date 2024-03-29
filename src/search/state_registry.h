#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include "algorithms/segmented_vector.h"
#include "algorithms/int_packer.h"
#include "globals.h"
#include "leaf_state.h"
#include "leaf_state_id.h"
#include "state.h"
#include "state_id.h"
#include "utils/hash.h"

#include <chrono>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>

/*
  Overview of classes relevant to storing and working with registered states.

  GlobalState
    This class is used for manipulating states.
    It contains the (uncompressed) variable values for fast access by the heuristic.
    A GlobalState is always registered in a StateRegistry and has a valid ID.
    GlobalStates can be constructed from a StateRegistry by factory methods for the
    initial state and successor states.
    They never own the actual state data which is borrowed from the StateRegistry
    that created them.

  StateID
    StateIDs identify states within a state registry.
    If the registry is known, the ID is sufficient to look up the state, which
    is why IDs are intended for long term storage (e.g. in open lists).
    Internally, a StateID is just an integer, so it is cheap to store and copy.

  PackedStateBin (currently the same as unsigned int)
    The actual state data is internally represented as a PackedStateBin array.
    Each PackedStateBin can contain the values of multiple variables.
    To minimize allocation overhead, the implementation stores the data of many
    such states in a single large array (see SegmentedArrayVector).
    PackedStateBin arrays are never manipulated directly but through
    a global IntPacker object.

  -------------

  StateRegistry
    The StateRegistry allows to create states giving them an ID. IDs from
    different state registries must not be mixed.
    The StateRegistry also stores the actual state data in a memory friendly way.
    It uses the following class:

  SegmentedArrayVector<PackedStateBin>
    This class is used to store the actual (packed) state data for all states
    while avoiding dynamically allocating each state individually.
    The index within this vector corresponds to the ID of the state.

  PerStateInformation<T>
    Associates a value of type T with every state in a given StateRegistry.
    Can be thought of as a very compactly implemented map from GlobalState to T.
    References stay valid forever. Memory usage is essentially the same as a
    vector<T> whose size is the number of states in the registry.


  ---------------
  Usage example 1
  ---------------
  Problem:
    A search node contains a state together with some information about how this
    state was reached and the status of the node. The state data is already
    stored and should not be duplicated. Open lists should in theory store search
    nodes but we want to keep the amount of data stored in the open list to a
    minimum.

  Solution:

    SearchNodeInfo
      Remaining part of a search node besides the state that needs to be stored.

    SearchNode
      A SearchNode combines a StateID, a reference to a SearchNodeInfo and
      OperatorCost. It is generated for easier access and not intended for long
      term storage. The state data is only stored once an can be accessed
      through the StateID.

    SearchSpace
      The SearchSpace uses PerStateInformation<SearchNodeInfo> to map StateIDs to
      SearchNodeInfos. The open lists only have to store StateIDs which can be
      used to look up a search node in the SearchSpace on demand.

  ---------------
  Usage example 2
  ---------------
  Problem:
    In the LMcount heuristic each state should store which landmarks are
    already reached when this state is reached. This should only require
    additional memory, when the LMcount heuristic is used.

  Solution:
    The heuristic object uses a field of type PerStateInformation<std::vector<bool> >
    to store for each state and each landmark whether it was reached in this state.
 */

namespace symmetries {
class DecoupledPermutation;
class Permutation;
class SymmetryCPG;
}

class PerStateInformationBase;

class StateRegistry {
    friend class SearchEngine; // to permute initial state

    struct StateIDSemanticHash {
        const segmented_vector::SegmentedArrayVector<PackedStateBin> &state_data_pool;
        int state_size;
        StateIDSemanticHash(
                const segmented_vector::SegmentedArrayVector<PackedStateBin> &state_data_pool,
                int state_size)
        : state_data_pool(state_data_pool),
          state_size(state_size) {
        }

        size_t operator()(StateID id) const {
            const PackedStateBin *data = state_data_pool[id.value];
            utils::HashState hash_state;
            for (int i = 0; i < state_size; ++i) {
                hash_state.feed(data[i]);
            }
            return hash_state.get_hash32();
        }
    };

    struct StateIDSemanticEqual {
        const segmented_vector::SegmentedArrayVector<PackedStateBin> &state_data_pool;
        int state_size;
        StateIDSemanticEqual(
                const segmented_vector::SegmentedArrayVector<PackedStateBin> &state_data_pool,
                int state_size)
        : state_data_pool(state_data_pool),
          state_size(state_size) {
        }

        bool operator()(StateID lhs, StateID rhs) const {
            const PackedStateBin *lhs_data = state_data_pool[lhs.value];
            const PackedStateBin *rhs_data = state_data_pool[rhs.value];
            return std::equal(lhs_data, lhs_data + state_size, rhs_data);
        }
    };

    struct LeafStateIDSemanticHash {
        const std::vector<segmented_vector::SegmentedArrayVector<PackedStateBin>* > &data_pool;
        std::vector<int> state_sizes;
        LeafStateIDSemanticHash(const std::vector<segmented_vector::SegmentedArrayVector<PackedStateBin>* > &state_data_pool_)
        : data_pool(state_data_pool_) {
            if (g_factoring){
                // we need the guard for abstain_type COMPUTE_RUN_STANDARD, where
                // g_leaves is set, but we run explicit-state search
                state_sizes.resize(g_leaves.size());
                for (size_t i = 0; i < g_leaves.size(); ++i){
                    state_sizes[i] = g_leaf_state_packers[i]->get_num_bins();
                }
            }
        }
        size_t operator() (const LeafStateID &id) const {
            const PackedStateBin *data = (*data_pool[id.factor])[id.value];
            utils::HashState hash_state;
            for (int i = 0; i < state_sizes[id.factor]; ++i) {
                hash_state.feed(data[i]);
            }
            return hash_state.get_hash32();
        }
    };

    struct LeafStateIDSemanticEqual {
        const std::vector<segmented_vector::SegmentedArrayVector<PackedStateBin>* > &data_pool;
        std::vector<int> state_sizes;
        LeafStateIDSemanticEqual(const std::vector<segmented_vector::SegmentedArrayVector<PackedStateBin>* > &state_data_pool_)
        : data_pool(state_data_pool_) {
            if (g_factoring){
                // we need the guard for abstain_type COMPUTE_RUN_STANDARD, where
                // g_leaves is set, but we run explicit-state search
                state_sizes.resize(g_leaves.size());
                for (size_t i = 0; i < g_leaves.size(); ++i){
                    state_sizes[i] = g_leaf_state_packers[i]->get_num_bins();
                }
            }
        }

        bool operator() (const LeafStateID &lhs, const LeafStateID &rhs) const {
            assert(lhs.factor == rhs.factor);
            const PackedStateBin *lhs_data = (*data_pool[lhs.factor])[lhs.value];
            const PackedStateBin *rhs_data = (*data_pool[rhs.factor])[rhs.value];
            return std::equal(lhs_data, lhs_data + state_sizes[lhs.factor], rhs_data);
        }
    };

    /*
      Hash set of StateIDs used to detect states that are already registered in
      this registry and find their IDs. GlobalStates are compared/hashed semantically,
      i.e. the actual state data is compared, not the memory location.
     */

    using StateIDSet = std::unordered_set<StateID, StateIDSemanticHash, StateIDSemanticEqual>;

    using LeafStateIDSet = std::unordered_set<LeafStateID,
            LeafStateIDSemanticHash,
            LeafStateIDSemanticEqual>;

    using DupCounterTable = std::unordered_map<size_t, size_t>;


    std::chrono::duration<double> dominance_pruning_timer;

    segmented_vector::SegmentedArrayVector<PackedStateBin> state_data_pool;

    std::vector<segmented_vector::SegmentedArrayVector<PackedStateBin>* > leaf_state_data_pool;

    StateIDSet registered_states;

    std::vector<LeafStateIDSet> registered_leaf_states;

    GlobalState *cached_initial_state;

    /*
      maps the StateID of the base state to the number of different decoupled
      states that have the same center
     */
    DupCounterTable state_duplicate_counter;

    mutable std::set<PerStateInformationBase *> subscribers;

    StateID insert_id_or_pop_state();

    LeafStateHash insert_id_or_pop_leaf_state(LeafFactorID factor);

    void permute_initial_state();

public:
    StateRegistry();
    ~StateRegistry();

    /*
      Returns the state that was registered at the given ID. The ID must refer
      to a state in this registry. Do not mix IDs from from different registries.
     */
    GlobalState lookup_state(StateID id) const;

    /*
      Returns the redFact that was registered at the given ID. The ID must refer
      to a redFact in this registry. Do not mix IDs from from different registries.
     */
    LeafState lookup_leaf_state(LeafStateHash id, LeafFactorID factor) const;

    /*
      Returns a reference to the initial state and registers it if this was not
      done before. The result is cached internally so subsequent calls are cheap.
     */
    const GlobalState &get_initial_state();

    GlobalState get_state_permutation(const GlobalState &state, const symmetries::Permutation &permutation);

    GlobalState get_decoupled_state_permutation(const GlobalState &state,
            symmetries::SymmetryCPG &cpg,
            const symmetries::DecoupledPermutation &permutation);

    /*
      Returns the state that results from applying op to predecessor and
      registers it if this was not done before. This is an expensive operation
      as it includes duplicate checking.
     */
    GlobalState get_successor_state(const GlobalState &predecessor, const Operator &op, bool compute_canonical = true);

    GlobalState get_state(const std::vector<int> &facts);

    GlobalState get_center_state(const std::vector<int> &facts);

    /*
      returns the state resulting from setting the last variable (dup counter)
      to dup_counter
     */
    GlobalState get_decoupled_state(const GlobalState &base_state, int dup_counter);

    GlobalState get_center_successor(const GlobalState &center, const Operator &op);

    /**
     * NOTE: At least one leaf state in this factor has to exist
     */
    LeafStateHash get_leaf_state_hash(const std::vector<int> &facts, LeafFactorID factor);

    LeafStateHash get_successor_leaf_state_hash(const LeafState &predecessor, const Operator &op);

    /*
      Returns the number of states registered so far.
     */
    size_t size() const {
        return registered_states.size();
    }

    /*
     * Returns the number of states registered for the specified leaf factor
     */
    size_t size(LeafFactorID factor) const {
        return registered_leaf_states[factor].size();
    }

    void print_decoupled_search_statistics() const;

    /*
      Remembers the given PerStateInformation. If this StateRegistry is
      destroyed, it notifies all subscribed PerStateInformation objects.
      The information stored in them that relates to states from this
      registry is then destroyed as well.
     */
    void subscribe(PerStateInformationBase *psi) const;
    void unsubscribe(PerStateInformationBase *psi) const;
};

#endif
