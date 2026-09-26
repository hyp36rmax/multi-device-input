#include "research_scenario_runner.hpp"

#include <cassert>
#include <string_view>

using namespace HYP36RResearchRunner;

int main()
{
	Runner runner{ R2AScenarios };
	assert(runner.phase() == Phase::Ready);
	assert(runner.start(10.0));
	assert(runner.update(12.99) == Action::None);
	assert(runner.update(13.0) == Action::StartCapture);
	assert(runner.update(32.99) == Action::None);
	assert(runner.update(33.0) == Action::StopCapture);
	assert(runner.phase() == Phase::Review);
	assert(runner.accept());
	assert(runner.scenario_index() == 1);

	assert(runner.start(40.0));
	assert(runner.cancel(false));
	assert(runner.phase() == Phase::Ready);
	assert(runner.scenario_index() == 1);
	assert(runner.attempt() == 1);

	assert(runner.start(50.0));
	assert(runner.update(53.0) == Action::StartCapture);
	assert(runner.update(71.0) == Action::StopCapture);
	assert(runner.retry(72.0));
	assert(runner.attempt() == 2);
	assert(runner.scenario_index() == 1);
	assert(runner.update(75.0) == Action::StartCapture);
	assert(runner.cancel(true));
	assert(runner.attempt() == 3);

	Runner surfaceRunner{ R2BScenarios };
	assert(surfaceRunner.scenarios().size() == 6);
	assert(std::string_view(surfaceRunner.scenario().id) == "R2_B01_LOCAL_ASPHALT_CONTROL");
	assert(surfaceRunner.scenario().durationSeconds == 20.0);
	assert(surfaceRunner.start(0.0));
	assert(surfaceRunner.update(3.0) == Action::StartCapture);
	assert(surfaceRunner.update(23.0) == Action::StopCapture);
	assert(surfaceRunner.accept());
	for (std::size_t index = 1; index < R2BScenarios.size(); ++index)
	{
		assert(surfaceRunner.start(30.0 * index));
		assert(surfaceRunner.update(30.0 * index + 3.0) == Action::StartCapture);
		assert(surfaceRunner.update(30.0 * index + 21.0) == Action::StopCapture);
		assert(surfaceRunner.accept());
	}
	assert(surfaceRunner.phase() == Phase::Finished);

	return 0;
}
