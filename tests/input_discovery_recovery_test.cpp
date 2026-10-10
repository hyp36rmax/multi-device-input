#include "input_discovery_recovery.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
	using namespace InputDiscovery;
	assert(resolve_backend(0, true, Override::Automatic) == Backend::DirectInput);
	assert(resolve_backend(0, false, Override::Automatic) == Backend::Wgi);
	assert(resolve_backend(1, true, Override::Automatic) == Backend::RawInput);
	assert(resolve_backend(2, false, Override::Automatic) == Backend::DirectInput);
	assert(resolve_backend(3, true, Override::Automatic) == Backend::XInput);
	assert(resolve_backend(0, false, parse_override("DIRECTINPUT")) == Backend::DirectInput);
	assert(resolve_backend(0, true, parse_override("wgi")) == Backend::Wgi);
	assert(resolve_backend(0, true, parse_override("rawinput")) == Backend::RawInput);
	assert(resolve_backend(0, true, parse_override("XINPUT")) == Backend::XInput);
	assert(parse_override("AUTOMATIC") == Override::Automatic);
	assert(parse_override("WINDOWS.GAMING.INPUT") == Override::Wgi);
	assert(parse_override("invalid") == Override::Invalid);
	assert(std::string_view(selection_reason(0, true, Override::Automatic)).find("native FFB") != std::string_view::npos);
	assert(std::string_view(selection_reason(0, false, Override::Automatic)).find("Windows.Gaming.Input") != std::string_view::npos);
	assert(std::string_view(selection_reason(1, false, Override::Automatic)).find("RawInput") != std::string_view::npos);
	assert(std::string_view(selection_reason(2, false, Override::Automatic)).find("DirectInput") != std::string_view::npos);
	assert(std::string_view(selection_reason(3, false, Override::Automatic)).find("XInput") != std::string_view::npos);
	assert(std::string_view(selection_reason(0, false, Override::Wgi)).find("Explicit developer override") != std::string_view::npos);
	assert(std::string_view(selection_reason(0, false, Override::Invalid)).find("Windows.Gaming.Input") != std::string_view::npos);
	assert(std::string_view(override_value(Override::Automatic)) == "AUTOMATIC");
	assert(std::string_view(override_value(Override::Wgi)) == "WGI");
	assert(std::string_view(override_value(Override::DirectInput)) == "DIRECTINPUT");
	assert(std::string_view(override_value(Override::RawInput)) == "RAWINPUT");
	assert(std::string_view(override_value(Override::XInput)) == "XINPUT");
	assert(!restart_required(Override::Automatic, Override::Automatic));
	assert(restart_required(Override::Automatic, Override::Wgi));
	assert(!restart_required(Override::Automatic, Override::Invalid));
	assert(std::string_view(discovery_summary(0, false, false)) == "Discovery In Progress");
	assert(std::string_view(discovery_summary(0, true, false)) == "No Devices Detected");
	assert(std::string_view(discovery_summary(2, true, false)) == "Delayed Discovery Completed");
	assert(std::string_view(discovery_summary(2, true, true)) == "Device Open Failed");

	RecoverySchedule recovery;
	assert(!recovery.due(std::chrono::milliseconds(999)));
	assert(recovery.due(std::chrono::seconds(1)) && recovery.consume() == 1);
	assert(!recovery.due(std::chrono::seconds(2)));
	assert(recovery.due(std::chrono::seconds(3)) && recovery.consume() == 3);
	assert(recovery.due(std::chrono::seconds(5)) && recovery.consume() == 5);
	assert(recovery.complete() && recovery.consume() == -1);

	RegistryModel devices;
	assert(devices.size() == 0);
	assert(devices.enumerate(10));
	assert(!devices.enumerate(10));
	devices.opened(10, false);
	assert(devices.state(10) == Readiness::OpenFailed);
	devices.opened(10, true); devices.registered(10); devices.available(10);
	assert(devices.state(10) == Readiness::AvailableForBinding);
	assert(devices.enumerate(11)); // separate pedals or another mixed USB device
	devices.opened(11, true); devices.registered(11);
	assert(devices.size() == 2);
	devices.removed(10);
	assert(devices.state(10) == Readiness::Removed);
	assert(devices.state(99) == Readiness::NotEnumerated);
}
