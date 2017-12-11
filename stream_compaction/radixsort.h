#pragma once

#include "common.h"

namespace StreamCompaction {
namespace RadixSort {
	StreamCompaction::Common::PerformanceTimer& timer();

	void RadixSort(int n, int* data, int maxValue);
}
}
