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

	QuickSetupController quick;
	quick.saved[0] = CapturedInput{ "old", "Existing Wheel", "Axis 0" };
	quick.start(now);
	assert(quick.active && quick.step == 0);
	assert(quick.deadline - now == std::chrono::seconds(6));
	assert(!quick.continue_step(now));
	quick.observe({ "wheel-a", "Wheel interface A", "Axis 0", true });
	assert(!quick.continue_step(now));
	quick.retry(now);
	assert(!quick.candidate && !quick.timedOut);
	quick.observe({ "wheel-b", "Wheel interface B", "Axis 1", false });
	assert(quick.continue_step(now));
	assert(quick.saved[0]->deviceId == "wheel-b");
	quick.skip(now);
	assert(quick.step == 2 && !quick.saved[1]);
	quick.back(now);
	assert(quick.step == 1);
	quick.update(now + std::chrono::seconds(7));
	assert(quick.timedOut && !quick.candidate);
	quick.cancel();
	assert(!quick.active && quick.saved[0]->deviceId == "old");

	assert(sanitize_filename_component("Fanatec Podium Wheel Base DD2") == "Fanatec_Podium_Wheel_Base_DD2");
	assert(sanitize_filename_component("Thrustmaster: T300RS / Racing Wheel") == "Thrustmaster_T300RS_Racing_Wheel");
	assert(sanitize_filename_component("<>:\"/\\|?*") == "No_Wheel_Detected");
	return 0;
}
