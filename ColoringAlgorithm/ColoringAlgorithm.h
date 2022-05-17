#ifndef _COLORING_ALGORITHM_H
#define _COLORING_ALGORITHM_H

#include "configuration.h"
#include "GraphRepresentation.h"
#include "AdjacencyMatrix.h"
#include "CompressedSparseRow.h"
#include <thread>

class ColoringAlgorithm {
protected:
	constexpr static int INVALID_COLOR = -1;

	GRAPH_REPR_T* _adj = nullptr;
	std::vector<int> col;

public:
	static void printColorAlgorithmConfs();

	const GRAPH_REPR_T& adj() const;
	const std::vector<int> getColors() const;

	virtual const int startColoring() = 0;

	// Receive vertex index, number of colors used so far and pointer where to save the color.
	// Returns the number of colors used
	const int computeVertexColor(int const v, int const n_cols, int* targetCol) const;

	std::vector<std::pair<int, int>> checkCorrectColoring();

	virtual void printExecutionInfo() const = 0;
	virtual void printBenchmarkInfo() const;

	void printColors(std::ostream& os) const;
	void printDotFile(std::ostream& os) const;
};

#endif // !_COLORING_ALGORITHM_H