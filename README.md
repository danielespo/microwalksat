# microwalksat

(note, currently buggy and WIP ! gave a false positive result with one of the test CNFs that I have :/)

Compile using:

gcc -Wall -Wextra -std=c11 -c microwsat.c -o microwsat.o

Use as follows:

./microwsat FILE

in which FILE is a SAT problem in the DIMACS format
