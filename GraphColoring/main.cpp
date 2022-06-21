#include "GraphColoringConfig.h"

#include "ColoringAlgorithm.h"
#ifdef COLORING_ALGORITHM_GREEDY
#include "Greedy.h"
#endif
#ifdef COLORING_ALGORITHM_GM
#include "GebremedhinManne.h"
#endif
#ifdef COLORING_ALGORITHM_JP
#include "JonesPlassmann.h"
#endif
#ifdef COLORING_ALGORITHM_CUSPARSE
#include "CusparseColoring.h"
#endif

#include "GraphRepresentation.h"
#ifdef GRAPH_REPRESENTATION_ADJ_MATRIX
#include "AdjacencyMatrix.h"
#endif
#ifdef GRAPH_REPRESENTATION_CSR
#include "CompressedSparseRow.h"
#endif

#include "benchmark.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

bool print_colors = false;
char* print_color_file = nullptr;
unsigned int num_runs = 1;

void printUsage(int const argc, char** const argv) {
	std::cout << "Usage: " << argv[0] << " [OPTIONS] <graph_path>" << std::endl;
	std::cout << "OPTIONS:" << std::endl;
	std::cout << "\t-c[=FOUT]\tPrint assigned colors to FOUT after last run (default: stdout)" << std::endl;
	std::cout << "\t-r <int>\tRun algorithm <int> times (default: 1)" << std::endl;
}

char* getNextArgument(int* argc, char*** argv) {
	if (*argc == 0) {
		return nullptr;
	}

	char* const ret = *argv[0];
	--*argc;
	++*argv;
	return ret;
}

void analyzeArgs(int const argc, char** const argv, std::string* graph_path) {
	int rem = argc - 1;
	char** args = argv + 1;

	while (rem > 0) {
		char* curr_arg = getNextArgument(&rem, &args);
		if (curr_arg == nullptr) {
			std::cout << "An error occurred while parsing command line arguments" << std::endl;
			exit(1);
		}

		if (0 == strncmp(curr_arg, "-c", 2)) {
			print_colors = true;

			if (curr_arg[2] == '=') {	// Custom output file
				char* fOut = &curr_arg[3];
				if (strlen(fOut) > 0) {
					print_color_file = fOut;
				}
			} else {	// Output to stdout
				// Do nothing
			}
		} else if (0 == strcmp(curr_arg, "-r")) {
			char* str_n_runs = getNextArgument(&rem, &args);
			if (str_n_runs == nullptr) {
				std::cout << "Expected positive int argument for option -r" << std::endl;
				exit(1);
			}
			int n_runs = strtol(str_n_runs, nullptr, 10);
			if (n_runs <= 0) {
				std::cout << "Expected positive int argument for option -r, found '" << str_n_runs << "'" << std::endl;
				exit(1);
			}

			num_runs = n_runs;
		} else if (graph_path->empty()) {
			// Assume graph name
			*graph_path = std::string(curr_arg);
		} else {
			// Unrecognized
			std::cout << "Unrecognized argument '" << curr_arg << "'" << std::endl;
		}
	}
}


int main(int argc, char** argv) {
	COLORING_ALGO_T* _G;
	std::string graph_path;

	std::cout << "Graph Coloring - v" << GraphColoring_VERSION_MAJOR << "." << GraphColoring_VERSION_MINOR << "." << GraphColoring_VERSION_PATCH << std::endl;
	GraphRepresentation::printGraphRepresentationConfs();
	ColoringAlgorithm::printColorAlgorithmConfs();

	std::cout << std::endl;

	analyzeArgs(argc, argv, &graph_path);
	if (graph_path.empty()) {
		printUsage(argc, argv);
		exit(0);
	}

	std::cout << "Loading graph from " << graph_path << std::endl;

	_G = new COLORING_ALGO_T(graph_path);
	COLORING_ALGO_T& G = *_G;

	std::cout << "Graph succesfully loaded from file." << std::endl;
	G.adj().printGraphInfo();

	G.init();

#if defined(PARALLEL_GRAPH_COLOR) && !defined(USE_CUDA_ALGORITHM) && !defined(COLORING_ALGORITHM_CUSPARSE)
	std::cout << "Performing computation using " << G.MAX_THREADS_SOLVE << " threads." << std::endl;
#endif

	std::vector<int> n_colors(num_runs, -1);

	for (int i = 0; i < num_runs; ++i) {

		std::cout << "========================================================" << std::endl;

		G.reset();

		std::cout << i + 1 << "\t";
		
		n_colors[i] = G.startColoring();

		if (n_colors[i] < 0) {
			std::cout << "ERROR" << std::endl;
			continue;
		}

		Benchmark& bm = *Benchmark::getInstance(i);
		std::cout << "num-cols: " << n_colors[i] << " \t";
		std::cout << "time: " << bm.getTimeOfFlag(2) << " \t";

		std::cout << std::endl;
	}

	std::cout << "========================================================" << std::endl;
	
	Benchmark& bm = *Benchmark::getInstance(0);
	std::cout << "Load time: " << bm.getTimeOfFlag(0) << std::endl;
	std::cout << "Avg preprocess time: " << Benchmark::getAvgOfFlag(1) << std::endl;
	std::cout << "Avg process time: " << Benchmark::getAvgOfFlag(2) << std::endl;
	std::cout << "Avg postprocess time: " << Benchmark::getAvgOfFlag(3) << std::endl;
	std::cout << "Avg total time: " << Benchmark::getEffectiveAvg() << std::endl;
	std::cout << "Avg number colors: " << (int)round(std::accumulate(n_colors.begin(), n_colors.end(), 0.0f) / num_runs) << std::endl;

	//std::cout << "Load time: " << 
	//G.printExecutionInfo();

	//std::cout << std::endl;

	//G.printBenchmarkInfo();

	//std::vector<std::pair<int, int>> incorrectPairs = G.checkCorrectColoring();
	//if (!incorrectPairs.empty()) {
	//	std::cout <<
	//		"*****************************************************************************" << std::endl <<
	//		"There was an error while assigning colors. " << (incorrectPairs.size() >> 1) << " pairs of verteces have the same color." << std::endl <<
	//		"*****************************************************************************" << std::endl;
	//	if (print_colors) {
	//		for (auto& p : incorrectPairs) {
	//			if (p.first < p.second) {
	//				std::cout << "v: " << p.first << " w: " << p.second << "  COLOR: " << G.getColors()[p.first] << std::endl;
	//			}
	//		}
	//	}
	//}

	if (print_colors) {
		// Select ostream
		std::ostream* os = &std::cout;
		if (print_color_file != nullptr) {
			os = new std::ofstream(print_color_file, std::ios::out);
		}

		G.printColors(*os);
	}
	//G.printDotFile(std::ofstream("output.txt"));

	return 0;
}
