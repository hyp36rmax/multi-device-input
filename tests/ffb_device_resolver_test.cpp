#include "ffb_device_resolver.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

struct Device { std::string id; std::string name; };

int main()
{
	const std::vector<Device> devices{
		{ "first", "Wheel A" }, { "second", "Wheel A" }, { "third", "Wheel B" }
	};
	assert((FFBDeviceResolver::candidate_order(devices, "second") == std::vector<size_t>{ 1, 0 }));
	assert((FFBDeviceResolver::candidate_order(devices, "first") == std::vector<size_t>{ 0, 1 }));
	assert((FFBDeviceResolver::candidate_order(devices, "missing") == std::vector<size_t>{ 0, 1, 2 }));
	assert((FFBDeviceResolver::candidate_order(devices, "") == std::vector<size_t>{ 0, 1, 2 }));
	assert(FFBDeviceResolver::candidate_order(std::vector<Device>{}, "").empty());
}
