#include "ffb_device_resolver.hpp"
#include "ffb_test_policy.hpp"

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
	assert((FFBDeviceResolver::candidate_order(devices, "second") == std::vector<size_t>{ 1, 0, 2 }));
	assert((FFBDeviceResolver::candidate_order(devices, "first") == std::vector<size_t>{ 0, 1, 2 }));
	assert((FFBDeviceResolver::candidate_order(devices, "missing") == std::vector<size_t>{ 0, 1, 2 }));
	assert((FFBDeviceResolver::candidate_order(devices, "") == std::vector<size_t>{ 0, 1, 2 }));
	assert(FFBDeviceResolver::candidate_order(std::vector<Device>{}, "").empty());
	assert(!FFBDeviceResolver::should_persist_choice(devices, "second", "second"));
	assert(FFBDeviceResolver::should_persist_choice(devices, "first", "second"));
	assert(!FFBDeviceResolver::should_persist_choice(devices, "temporarily absent", "first"));
	assert(FFBDeviceResolver::should_persist_choice(devices, "", "first"));
	static_assert(FFBTestPolicy::DirectionTestMagnitude == 2000);
	static_assert(FFBTestPolicy::reverse_direction(-1.0f, false));
	static_assert(!FFBTestPolicy::reverse_direction(1.0f, false));
	static_assert(!FFBTestPolicy::reverse_direction(-1.0f, true));
	static_assert(FFBTestPolicy::reverse_direction(1.0f, true));
}
