all:
	clang++ main.cpp -g -Wall -O0 -std=c++20 -fsanitize=leak -o hashtable.out
