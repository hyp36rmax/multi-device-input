#include "research_scenario_runner.hpp"

#include <cassert>

using namespace HYP36RResearchRunner;

int main()
{
	Runner runner;
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
	assert(runner.cancel());
	assert(runner.phase() == Phase::Ready);
	assert(runner.scenario_index() == 1);

	assert(runner.start(50.0));
	assert(runner.update(53.0) == Action::StartCapture);
	assert(runner.update(71.0) == Action::StopCapture);
	assert(runner.retry(72.0));
	assert(runner.attempt() == 2);
	assert(runner.scenario_index() == 1);
	assert(runner.update(75.0) == Action::StartCapture);

	return 0;
}
