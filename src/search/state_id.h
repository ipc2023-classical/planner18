#ifndef STATE_ID_H
#define STATE_ID_H

#include <iostream>

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.

class StateID {
    friend class StateRegistry;
    friend std::ostream &operator<<(std::ostream &os, StateID id);
    template<typename>
    friend class PerStateInformation;

    size_t value;
    explicit StateID(size_t value)
        : value(value) {
    }

    // No implementation to prevent default construction
    StateID();
public:
    ~StateID() {
    }

    static const StateID no_state;

    bool operator==(const StateID &other) const {
        return value == other.value;
    }

    bool operator!=(const StateID &other) const {
        return !(*this == other);
    }

    size_t hash() const {
        return value;
    }
};

std::ostream &operator<<(std::ostream &os, StateID id);

#endif
