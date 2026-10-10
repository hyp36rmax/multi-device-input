#include "device_diagnostics/device_diagnostics_model.hpp"

#include <cassert>

int main()
{
	using namespace DeviceDiagnostics;
	SafetyController safety;
	const auto now = std::chrono::steady_clock::now();
	assert(!safety.begin(true, true, now));
	safety.authorized = true;
	assert(!safety.begin(false, true, now));
	assert(safety.begin(true, true, now));
	assert(safety.bounded_magnitude(100) == 2000);
	assert(safety.bounded_magnitude(10) == 1000);
	assert(safety.must_stop(false, true, true, now));
	safety.stop();
	assert(safety.begin(true, true, now));
	assert(safety.must_stop(true, false, true, now));
	safety.stop();
	assert(safety.begin(true, true, now));
	assert(safety.must_stop(true, true, false, now));
	safety.stop();
	assert(safety.begin(true, true, now));
	assert(safety.must_stop(true, true, true, now + MaximumRunTime));
	return 0;
}
