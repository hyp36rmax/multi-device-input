#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace HYP36RResearchReplay
{
	struct Channels
	{
		float directional = 0.0f;
		float road = 0.0f;
		float impact = 0.0f;
	};

	struct Result
	{
		Channels postGain{};
		float composerInput = 0.0f;
		float postTanh = 0.0f;
		float preDrive = 0.0f;
	};

	inline Result replay(Channels raw, Channels gains, float outputRamp)
	{
		const Result result{
			{ raw.directional * gains.directional,
				raw.road * gains.road, raw.impact * gains.impact },
		};
		Result completed = result;
		completed.composerInput = completed.postGain.directional +
			completed.postGain.road + completed.postGain.impact;
		completed.postTanh = std::tanh(completed.composerInput);
		completed.preDrive = completed.postTanh *
			(std::clamp)(outputRamp, 0.0f, 1.0f);
		return completed;
	}

	inline std::array<bool, 4> surface_changes(
		const std::array<unsigned, 4>& previous,
		const std::array<unsigned, 4>& current)
	{
		return { previous[0] != current[0], previous[1] != current[1],
			previous[2] != current[2], previous[3] != current[3] };
	}
}
