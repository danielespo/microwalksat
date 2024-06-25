/*********************************************************************[microwsat.c]***

  The MIT License

  Copyright (c) 2024 Daniel Alberto Espinosa Gonzalez (wsat algo plus updated datastructs)
  Copyright (c) 2014-2018 Marijn Heule (parser in microsat.c)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

enum {UNSAT = 0, SAT = 1};

typedef struct {int **clauses;int numClauses;int numVariables;
} CNF;

typedef struct {CNF cnf; int *model; int maxTries; int maxFlips; float noise;
} solver;

// memory
int* getMemory(int size) {
    int* ptr = (int*)malloc(sizeof(int) * size);
    if (ptr == NULL) {fprintf(stderr, "Memory allocation failed\n");exit(1);}
    return ptr;
}

void freeSolver(solver* S) { //prevent memory leaks
    for (int i = 0; i < S->cnf.numClauses; i++) {
        free(S->cnf.clauses[i]);
    }
    free(S->cnf.clauses);
    free(S->model);
}

// prealloc solver memory and structs
void initWSAT(solver* S, int n, int m) {
    S->cnf.numVariables = n;
    S->cnf.numClauses = m;
    S->model = getMemory(n + 1);
    S->maxTries = 5;
    S->maxFlips = 100;
    S->noise = 0.4;
    for (int i = 1; i <= n; i++) {
        S->model[i] = rand() % 2; //random assignment to start with (phase)
    }
}

// parser from microsat, thanks Marijn Heule :)
int parse(solver* S, char* filename) {
    FILE* input = fopen(filename, "r");
    if (!input) {
        printf("Error opening file\n");
        return 0;
    }

    int nVars, nClauses;
    fscanf(input, " p cnf %d %d \n", &nVars, &nClauses);

    initWSAT(S, nVars, nClauses);

    S->cnf.clauses = (int**)malloc(nClauses * sizeof(int*));

    for (int i = 0; i < nClauses; i++) {
        S->cnf.clauses[i] = (int*)malloc((nVars + 1) * sizeof(int));
        int lit, size = 0;
        while (fscanf(input, " %d ", &lit) && lit != 0) {
            S->cnf.clauses[i][size++] = lit;
        }
        S->cnf.clauses[i][size] = 0; // End of clause marker
    }

    fclose(input);
    return 1;
}

int evaluateClause(int* clause, int* model) { // eval a literal against the model
    for (int i = 0; clause[i] != 0; i++) {
        int lit = clause[i];
        if ((lit > 0 && model[abs(lit)] == 1) || (lit < 0 && model[abs(lit)] == 0)) {
            return 1;
        }
    }
    return 0;
}

int pickRandomUnsatisfiedClause(solver* S) {
    int* unsatisfiedClauses = (int*)malloc(S->cnf.numClauses * sizeof(int));
    int count = 0;
    for (int i = 0; i < S->cnf.numClauses; i++) {
        if (!evaluateClause(S->cnf.clauses[i], S->model)) {
            unsatisfiedClauses[count++] = i;
        }
    }

    if (count == 0) {
        free(unsatisfiedClauses);
        return -1;
    }

    int clauseIndex = unsatisfiedClauses[rand() % count];
    free(unsatisfiedClauses);
    return clauseIndex;
}

int pickVariableToFlip(solver* S, int* clause) { 
    if ((float)rand() / RAND_MAX < S->noise) {
        int i = 0;
        // random walk: flip a random variable in the clause
        while (clause[i] != 0) i++;
        return clause[rand() % i];
    } else { // greedy move: pick best variable to flip based on break count (how many clauses falsified iff flip)
        int bestVar = clause[0];
        int bestBreakCount = S->cnf.numClauses;

        for (int i = 0; clause[i] != 0; i++) {
            int var = abs(clause[i]);
            int breakCount = 0;

            for (int j = 0; j < S->cnf.numClauses; j++) {
                if (S->cnf.clauses[j] != clause) {
                    int satisfied = 0;
                    for (int k = 0; S->cnf.clauses[j][k] != 0; k++) {
                        if (S->model[abs(S->cnf.clauses[j][k])] == (S->cnf.clauses[j][k] > 0)) {
                            satisfied = 1;
                            break;
                        }
                    }
                    if (!satisfied) breakCount++;
                }
            }

            if (breakCount < bestBreakCount) {
                bestBreakCount = breakCount;
                bestVar = var;
            }
        }
        return bestVar;
    }
}

void flipVariable(solver* S, int var) {
    S->model[var] = !S->model[var];
}

int solve(solver* S) {

    for (int i = 0; i < S->maxTries; i++) {
        for (int j = 1; j <= S->cnf.numVariables; j++) { //rephase everything each try
            S->model[j] = rand() % 2;
        }
        for (int k = 0; k < S->maxFlips; k++) {
            
            // Check if the current model satisfies the CNF
            int satisfied = 1;
            for (int c = 0; c < S->cnf.numClauses; c++) {
                if (!evaluateClause(S->cnf.clauses[c], S->model)) {
                    satisfied = 0;
                    break;
                }
            }
            if (satisfied) {
                return SAT;
            }
            int clauseIndex = pickRandomUnsatisfiedClause(S);
            if (clauseIndex == -1) return SAT; //if no unsat clauses, return SAT

            int* clause = S->cnf.clauses[clauseIndex];
            int varToFlip = pickVariableToFlip(S, clause);
            flipVariable(S, abs(varToFlip));
        }
    }

    return UNSAT; //else, return UNSAT
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("No input file provided\n");
        return 1;
    }

    solver S;
    if (!parse(&S, argv[1])) {
        printf("Failed to parse input file\n");
        return 1;
    }

    if (solve(&S)) {
        printf("s SATISFIABLE\n");
    } else {
        printf("s UNSATISFIABLE\n");
    }

    freeSolver(&S);// & is the address-of operator in C
    return 0;
}
