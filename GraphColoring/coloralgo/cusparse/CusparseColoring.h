#ifndef _CUSPARSE_COLORING_
#define _CUSPARSE_COLORING_

//#include "configuration.h"
#include "ColoringAlgorithm.h"

class CusparseColoring : public ColoringAlgorithm {

	define_super(ColoringAlgorithm);

private:

public:
	CusparseColoring(std::string const filepath);
	void init() override;
	void reset() override;
	const int startColoring() override;

	void printExecutionInfo() const override;
	void printBenchmarkInfo() const override;
};

#endif // !_CUSPARSE_COLORING_

