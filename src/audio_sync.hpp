#pragma once

#include <string>

namespace AudioSync
{
	enum class Marker { Start, End };

	bool begin_session(const std::string& scenario, const std::string& telemetryFilename);
	bool emit_marker(Marker marker, unsigned long long frame, double telemetryElapsedSeconds);
	void finish_session(const char* status);
	bool active();
	const std::string& status_text();
}
