#ifndef SYMBOLIC_SYM_SOLUTION_H
#define SYMBOLIC_SYM_SOLUTION_H

#include "sym_variables.h"
#include <vector>

namespace symbolic {
class SymSearch;

#ifdef USE_CUDD

class SymSolution {
    SymSearch *exp_fw, *exp_bw;
    int g, h;
    BDD cut;
public:
    SymSolution() : exp_fw (nullptr), exp_bw (nullptr),  g(-1), h(-1) {} // No solution yet

    SymSolution(SymSearch *e_fw, int cost, BDD S) : exp_fw(e_fw), exp_bw (nullptr), g(cost), h(0), cut(S) {}

    SymSolution(SymSearch *e_fw, SymSearch *e_bw, int g_val, int h_val, BDD S) : exp_fw(e_fw), exp_bw(e_bw), g(g_val), h(h_val), cut(S) {}

    // SymSolution(int g_val, int h_val, BDD S) : g(g_val), h(h_val), cut(S) {}

    inline bool solved() const {
        return g + h >= 0;
    }

    inline int getCost() const {
        return g + h;
    }

    const BDD & getCut() const {
        return cut;
    }

    int getH() const {
        return h;
    }

    int getG() const {
        return g;
    }

    void getPlan(std::vector <const Operator *> &path) const;

    ADD getADD() const;

};

#endif

}
#endif
