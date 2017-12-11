#include <cstdio>
#include "cpu.h"
#include "common.h"

namespace StreamCompaction {
namespace CPU {

using StreamCompaction::Common::PerformanceTimer;
PerformanceTimer& timer()
{
	static PerformanceTimer timer;
	return timer;
}

/**
 * CPU scan (prefix sum).
 */
void scan(int n, int *odata, const int *idata) {
    
	timer().startCpuTimer();

	odata[0] = 0;
	for (int i = 1; i < n; ++i)
		odata[i] = odata[i - 1] + idata[i - 1];

	timer().endCpuTimer();
}

/**
 * CPU stream compaction without using the scan function.
 *
 * @returns the number of elements remaining after compaction.
 */
int compactWithoutScan(int n, int *odata, const int *idata) {
	
	timer().startCpuTimer();
	
	int index = 0;
	for (int i = 0; i < n; ++i)
	{
		if (idata[i] != 0)
		{
			odata[index++] = idata[i];
		}
	}

	timer().endCpuTimer();
	
	return index;
}

/**
 * CPU stream compaction using scan and scatter, like the parallel version.
 *
 * @returns the number of elements remaining after compaction.
 */
int compactWithScan(int n, int *odata, const int *idata) {

	int* inputScan = new int[n];
	int* outputScan = new int[n];

	timer().startCpuTimer();

	for (int i = 0; i < n; ++i)
		inputScan[i] = (idata[i] == 0) ? 0 : 1;

	outputScan[0] = 0;
	for (int i = 1; i < n; ++i)
		outputScan[i] = outputScan[i - 1] + inputScan[i - 1];

	int sum = 0;
	for (int i = 0; i < n; ++i)
	{
		if (idata[i] != 0)
		{
			odata[outputScan[i]] = idata[i];
			sum++;
		}
	}

	timer().endCpuTimer();

	delete[] inputScan;
	delete[] outputScan;
	return sum;
}

}
}
