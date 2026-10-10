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
	assert(parse_override("invalid") == Override::Invalid);

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
