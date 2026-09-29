#include "guided_uat.hpp"

#include <cassert>

int main()
{
	using namespace GuidedUat;
	static_assert(RoadCalibrationStages == std::array<int, 6>{ 8, 10, 15, 20, 25, 30 });
	static_assert(WarmupSeconds == 15.0);
	static_assert(CaptureSeconds == 36.0);

	RoadCalibrationSweep sweep;
	sweep.start();
	assert(sweep.phase() == Phase::Instructions);
	assert(!sweep.begin_stage(10, true, 1.0));
	assert(sweep.phase() == Phase::WaitingForConfiguration);
	sweep.update(8, 2.0);
	assert(sweep.begin_stage(8, true, 3.0));
	assert(sweep.phase() == Phase::Warmup);
	assert(sweep.warmup_remaining(13.0) == 5.0);
	sweep.update(8, 18.0);
	assert(sweep.phase() == Phase::Warmup);
	assert(sweep.start_capture_now(8, 18.0));
	sweep.update(10, 19.0);
	assert(sweep.current_mismatch());
	assert(sweep.capture_complete(54.0));
	sweep.finish_capture(54.0);
	assert(sweep.phase() == Phase::Assessment);
	assert(sweep.results()[0].measuredDuration == 36.0);
	sweep.retry();
	assert(sweep.phase() == Phase::WaitingForConfiguration);
	assert(sweep.begin_stage(8, true, 60.0));
	assert(sweep.start_capture_now(8, 61.0));
	assert(sweep.phase() == Phase::Recording);
	sweep.finish_capture(97.0);
	sweep.assess(Assessment::Good);
	assert(sweep.stage_index() == 1);
	assert(sweep.required_multiplier() == 10);

	for (std::size_t stage = 1; stage < RoadCalibrationStages.size(); ++stage)
	{
		const int gain = RoadCalibrationStages[stage];
		assert(sweep.begin_stage(gain, true, 100.0 + stage));
		assert(sweep.start_capture_now(gain, 101.0 + stage));
		sweep.finish_capture(137.0 + stage);
		sweep.assess(Assessment::Legible);
	}
	assert(sweep.phase() == Phase::Complete);
	sweep.set_preference(20);
	assert(sweep.preferred_multiplier() == 20);
	sweep.finish();
	assert(!sweep.active());

	request_road_calibration_sweep();
	assert(consume_road_calibration_sweep_request());
	assert(!consume_road_calibration_sweep_request());

	RoadCalibrationSweep cancelled;
	cancelled.start();
	cancelled.cancel();
	assert(!cancelled.active());
}
