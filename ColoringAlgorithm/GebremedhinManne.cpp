#include "configuration.h"
#include "GebremedhinManne.h"

#ifdef COMPUTE_ELAPSED_TIME
#include "benchmark.h"
#endif

#include <algorithm>
#include <fstream>

GebremedhinManne::GebremedhinManne(std::string const filepath) {
	Benchmark& bm = *Benchmark::getInstance();
	bm.clear(0);

	this->_adj = new GRAPH_REPR_T();

	std::ifstream fileIS;
	fileIS.open(filepath);
	std::istream& is = fileIS;
	is >> *this->_adj;

	if (fileIS.is_open()) {
		fileIS.close();
	}

	this->col = std::vector<int>(this->adj().nV());
	this->recolor = std::vector<int>(this->adj().nV());
	for (int i = 0; i < this->adj().nV(); ++i) {
		this->col[i] = GebremedhinManne::INVALID_COLOR;
		this->recolor[i] = i;
	}

#ifdef PARALLEL_GRAPH_COLOR
	this->nConflicts = 0;
	this->nIterations = 0;
#endif
}

const int GebremedhinManne::startColoring() {
	Benchmark& bm = *Benchmark::getInstance();
	bm.clear(1);
	bm.clear(2);
#ifdef PARALLEL_GRAPH_COLOR
	bm.clear(3);
#endif
	return this->solve();
}

int GebremedhinManne::colorGraph(int n_cols) {
#ifdef COMPUTE_ELAPSED_TIME
	Benchmark& bm = *Benchmark::getInstance();
	bm.sampleTime();
#endif

#ifdef PARALLEL_GRAPH_COLOR
	std::vector<std::thread> threadPool;
	int parallelIdx = 0;
	for (int i = 0; i < this->MAX_THREADS_SOLVE; ++i) {
		threadPool.emplace_back([&, n_cols] { this->colorGraphParallel(n_cols, parallelIdx); });
	}

	for (auto& t : threadPool) {
		t.join();
	}

	n_cols = *std::max_element(this->col.begin(), this->col.end()) + 1;
#endif
#ifdef SEQUENTIAL_GRAPH_COLOR
	for (auto it = this->recolor.begin();
		it != this->recolor.end(); ++it) {
		n_cols = this->computeVertexColor(*it, n_cols, &this->col[*it]);
	}
#endif

#ifdef COMPUTE_ELAPSED_TIME
	bm.sampleTimeToFlag(2);
#endif

	return n_cols;
}

#ifdef PARALLEL_GRAPH_COLOR
int GebremedhinManne::colorGraphParallel(int n_cols, int& i) {
	this->mutex.lock();
	while (i < this->recolor.size()) {
		int v = this->recolor[i];
		++i;
		this->mutex.unlock();

		n_cols = this->computeVertexColor(v, n_cols, &this->col[v]);

		this->mutex.lock();
	}
	this->mutex.unlock();

	return n_cols;
}

int GebremedhinManne::detectConflicts() {
#ifdef COMPUTE_ELAPSED_TIME
	Benchmark& bm = *Benchmark::getInstance();
	bm.sampleTime();
#endif

	this->recolor.erase(this->recolor.begin(), this->recolor.end());
	std::vector<std::thread> threadPool;
	for (int i = 0; i < this->MAX_THREADS_SOLVE; ++i) {
		threadPool.emplace_back([&, i] { this->detectConflictsParallel(i); });
	}

	for (auto& t : threadPool) {
		t.join();
	}

	int recolorSize = this->recolor.size();

#ifdef COMPUTE_ELAPSED_TIME
	bm.sampleTimeToFlag(3);
#endif

	return recolorSize;
}

void GebremedhinManne::detectConflictsParallel(const int i) {
	for (size_t v = i; v < this->adj().nV(); v += this->MAX_THREADS_SOLVE) {
		if (this->col[v] == GebremedhinManne::INVALID_COLOR) {
			this->mutex.lock();
			this->recolor.push_back(v);
			this->mutex.unlock();
			continue;
		}

		for (
			auto neighIt = this->adj().beginNeighs(v);
			neighIt != this->adj().endNeighs(v);
			++neighIt
			) {
			size_t w = *neighIt;
			if (v < w) continue;

			if (this->col[v] == this->col[w]) {
				this->mutex.lock();
				this->recolor.push_back(v);
				this->mutex.unlock();
				break;
			}
		}
	}

	return;
}
#endif

void GebremedhinManne::sortGraphVerts() {
#ifdef SORT_LARGEST_DEGREE_FIRST
	auto sort_lambda = [&](const int v, const int w) { return this->adj().countNeighs(v) > this->adj().countNeighs(w); };
#endif
#ifdef SORT_SMALLEST_DEGREE_FIRST
	auto sort_lambda = [&](const int v, const int w) { return this->adj().countNeighs(v) < this->adj().countNeighs(w); };
#endif
#ifdef SORT_VERTEX_ORDER
	auto sort_lambda = [&](const int v, const int w) { return v < w; };
#endif
#ifdef SORT_VERTEX_ORDER_REVERSED
	auto sort_lambda = [&](const int v, const int w) { return v > w; };
#endif

#ifdef COMPUTE_ELAPSED_TIME
	Benchmark& bm = *Benchmark::getInstance();
	bm.sampleTime();
#endif

	std::sort(this->recolor.begin(), this->recolor.end(), sort_lambda);

#ifdef COMPUTE_ELAPSED_TIME
	bm.sampleTimeToFlag(1);
#endif
}

const int GebremedhinManne::solve() {
	int n_cols = 0;

#ifdef SEQUENTIAL_GRAPH_COLOR
	this->sortGraphVerts();
	n_cols = this->colorGraph(n_cols);
#endif
#ifdef PARALLEL_GRAPH_COLOR
#ifdef PARALLEL_RECOLOR
	int partial_confs;
	do {
		this->sortGraphVerts();
		n_cols = this->colorGraph(n_cols);

		++this->nIterations;

		partial_confs = this->detectConflicts();
		this->nConflicts += partial_confs;
	} while (partial_confs > 0);
#endif
#ifdef SEQUENTIAL_RECOLOR
	this->sortGraphVerts();
	n_cols = this->colorGraph(n_cols);
	++this->nIterations;
	this->nConflicts = this->detectConflicts();

	if (this->nConflicts > 0) {
		this->sortGraphVerts();
		int index = 0;

#ifdef COMPUTE_ELAPSED_TIME
		Benchmark& bm = *Benchmark::getInstance();
		bm.sampleTime();
#endif

		n_cols = this->colorGraphParallel(n_cols, index);

#ifdef COMPUTE_ELAPSED_TIME
		bm.sampleTimeToFlag(1);
#endif
		++this->nIterations;
	}
#endif
#endif

	return n_cols;
}

#ifdef PARALLEL_GRAPH_COLOR
const int GebremedhinManne::getConflicts() const {
	return this->nConflicts;
}

const int GebremedhinManne::getIterations() const {
	return this->nIterations;
}
#endif
