#include "CompressedSparseRow.h"

#include "benchmark.h"

#include <climits>
#include <string>

CompressedSparseRow::CompressedSparseRow() : CompressedSparseRow(0, 0) { }

CompressedSparseRow::CompressedSparseRow(const CompressedSparseRow& to_copy) {
	this->rows = to_copy.rows;
	this->cols = to_copy.cols;

	this->col_idxs = std::vector<int>(to_copy.col_idxs);
	delete[] this->row_ptrs;
	this->row_ptrs = new int[static_cast<size_t>(this->rows) + 1];

	std::memcpy(this->row_ptrs, to_copy.row_ptrs, (static_cast<size_t>(this->rows) + 1) * sizeof(*this->row_ptrs));
}

CompressedSparseRow::CompressedSparseRow(int n_rows, int n_cols) {
	init(n_rows, n_cols);
}

CompressedSparseRow::CompressedSparseRow(int n) : CompressedSparseRow(n, n) { }

CompressedSparseRow::~CompressedSparseRow() {
	delete[] this->row_ptrs;
}

const int* CompressedSparseRow::getColIndexes() const {
	return this->col_idxs.data();
}
const int* CompressedSparseRow::getRowPointers() const {
	return this->row_ptrs;
}

void CompressedSparseRow::init(int n_rows, int n_cols) {
	this->rows = n_rows;
	this->cols = n_cols;

	this->col_idxs = std::vector<int>();
	//delete[] this->row_ptrs;
	this->row_ptrs = new int[static_cast<size_t>(this->rows) + 1];

	std::fill(this->row_ptrs, this->row_ptrs + this->rows, 0);
}

std::istream& operator>>(std::istream& is, CompressedSparseRow& m) {
#ifdef COMPUTE_ELAPSED_TIME
	Benchmark& bm = *Benchmark::getInstance();
	bm.sampleTime();
#endif

	int n;
	int row_idx;
	std::string buffer;
	std::vector<size_t> cols;

	is >> n;

	m.init(n, n);

	for (row_idx = 0; row_idx < n; ++row_idx) {
		is >> buffer;
		int actual_row_idx = atoi(buffer.c_str());
		cols.clear();

		is >> buffer;
		while (buffer.c_str()[0] != '#') {
			int col_idx = atoi(buffer.c_str());
			cols.push_back(col_idx);

			is >> buffer;
		}

		m.populateRow(actual_row_idx, cols.begin(), cols.end());
	}

#ifdef COMPUTE_ELAPSED_TIME
	bm.sampleTimeToFlag(0);
#endif

	return is;
}

const bool CompressedSparseRow::get(int row_idx, int col_idx) const {
	if (0 > row_idx || row_idx >= this->rows) {
		throw std::out_of_range("row index out of range: " + row_idx);
	}
	if (0 > col_idx || col_idx >= this->cols) {
		throw std::out_of_range("col index out of range: " + col_idx);
	}

	int col_ptr = this->row_ptrs[row_idx];
	int next_col_ptr = this->row_ptrs[row_idx + 1];

	while (col_ptr < next_col_ptr) {
		if (this->col_idxs[col_ptr] == col_idx) {
			return true;
		}

		++col_ptr;
	}

	return false;
}

template<typename Iterator>
void CompressedSparseRow::populateRow(int row_idx, Iterator begin, const Iterator end) {
	static size_t expected_row = 0;

	if (row_idx != expected_row) {
		throw std::invalid_argument("Expected row_idx = " + std::to_string(expected_row) + " but got " + std::to_string(row_idx));
	}
	if (0 > row_idx || row_idx >= this->rows) {
		throw std::out_of_range("row index out of range: " + std::to_string(row_idx));
	}

	int curr_row_ptr = this->row_ptrs[row_idx];
	int* next_row_ptrs_ptr = &this->row_ptrs[row_idx + 1];

	*next_row_ptrs_ptr = curr_row_ptr;
	while (begin != end) {
		int col_idx = *begin;
		this->col_idxs.push_back(col_idx);
		++(*next_row_ptrs_ptr);
		++begin;
	}

	++expected_row;
}

const ::std::vector<int>::const_iterator CompressedSparseRow::beginNeighs(int row_idx) const {
	if (0 > row_idx || row_idx >= this->rows) {
		throw std::out_of_range("row index out of range: " + row_idx);
	}

	return this->col_idxs.begin() + this->row_ptrs[row_idx];
}

const ::std::vector<int>::const_iterator CompressedSparseRow::endNeighs(int row_idx) const {
	if (0 > row_idx || row_idx >= this->rows) {
		throw std::out_of_range("row index out of range: " + row_idx);
	}

	return this->col_idxs.begin() + this->row_ptrs[row_idx + 1];
}

const size_t CompressedSparseRow::nE() const {
	return this->row_ptrs[this->rows];
}

const int CompressedSparseRow::nV() const {
	return this->rows;
}