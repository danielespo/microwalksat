# microwalksat

Compile using:

gcc -Wall -Wextra -std=c11 -c microwsat.c -o microwsat.o

OR just use the following command in the main directory:

make 

Use as follows:

./microwsat -t maxTries -f maxFlips FILE

in which FILE is a SAT problem in the DIMACS format
