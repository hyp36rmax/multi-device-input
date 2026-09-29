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

	for (auto protocol : { SurfaceProtocol::Amplitude, SurfaceProtocol::Waveform, SurfaceProtocol::Frequency })
	{
		SurfaceSweep surface;
		surface.start(protocol);
		const std::size_t expected = protocol == SurfaceProtocol::Amplitude ? 7 : protocol == SurfaceProtocol::Waveform ? 3 : 4;
		assert(surface.stage_count() == expected);
		for (std::size_t i = 0; i < expected; ++i)
		{
			const int required = surface.required_value();
			assert(!surface.begin_stage(required, false, 1.0 + i));
			assert(surface.begin_stage(required, true, 2.0 + i));
			assert(surface.warmup_remaining(12.0 + i) == 5.0);
			assert(surface.start_capture_now(required, 17.0 + i));
			surface.update(required, true, 18.0 + i);
			assert(surface.capture_complete(53.0 + i));
			surface.finish_capture(53.0 + i);
			surface.assess(2);
		}
		assert(surface.phase() == Phase::Complete);
		surface.set_preference(surface.required_value());
		assert(surface.preferred_value() >= 0);
		surface.finish(); assert(!surface.active());
	}
	request_surface_sweep(SurfaceProtocol::Frequency);
	assert(consume_surface_sweep_request() == SurfaceProtocol::Frequency);
	assert(consume_surface_sweep_request() == SurfaceProtocol::None);
	assert(resolve_amplitude_preference(30).value == 30 && resolve_amplitude_preference(30).inherited);
	assert(resolve_amplitude_preference(0).value == 24 && !resolve_amplitude_preference(0).inherited);
	assert(resolve_amplitude_preference(99).value == 24 && !resolve_amplitude_preference(99).inherited);
	assert(resolve_waveform_preference(2, true).value == 2 && resolve_waveform_preference(2, true).inherited);
	assert(resolve_waveform_preference(2, false).value == 0 && !resolve_waveform_preference(2, false).inherited);
}
