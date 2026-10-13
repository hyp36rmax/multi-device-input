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
	assert(FFBDeviceResolver::exact_identity_index(devices, "second") == 1);
	assert(FFBDeviceResolver::exact_identity_index(
		std::vector<Device>{ { "third", "Wheel B" }, { "second", "Wheel A" }, { "first", "Wheel A" } },
		"second") == 1);
	assert(!FFBDeviceResolver::exact_identity_index(devices, "missing"));
	assert(!FFBDeviceResolver::exact_identity_index(devices, ""));
	assert(!FFBDeviceResolver::should_persist_choice(devices, "second", "second"));
	assert(FFBDeviceResolver::should_persist_choice(devices, "first", "second"));
	assert(!FFBDeviceResolver::should_persist_choice(devices, "temporarily absent", "first"));
	assert(FFBDeviceResolver::should_persist_choice(devices, "", "first"));
	FFBTestPolicy::QuickSetupEndpointRefresh refresh;
	assert(refresh.state == FFBTestPolicy::QuickSetupRefreshState::Pending);
	assert(refresh.begin());
	assert(!refresh.begin());
	assert(refresh.invocationCount == 1);
	refresh.complete(true);
	assert(refresh.ready());
	assert(!refresh.begin());
	refresh.reset();
	assert(refresh.begin());
	refresh.complete(false);
	assert(refresh.state == FFBTestPolicy::QuickSetupRefreshState::Failed);
	refresh.retry();
	assert(refresh.state == FFBTestPolicy::QuickSetupRefreshState::Pending);
	assert(refresh.begin());
	assert(refresh.invocationCount == 2);
	refresh.complete(false);
	refresh.skip();
	assert(refresh.state == FFBTestPolicy::QuickSetupRefreshState::Skipped);
	static_assert(FFBTestPolicy::DirectionTestMagnitude == 2000);
	static_assert(FFBTestPolicy::ShakeTestMagnitude == 2000);
	static_assert(FFBTestPolicy::ShakeHalfPeriodMilliseconds == 50);
	static_assert(FFBTestPolicy::ShakeDurationMilliseconds == 1200);
	static_assert(FFBTestPolicy::ShakeEffectWatchdogMilliseconds == 250);
	static_assert(FFBTestPolicy::ShakeMaximumRuntimeMilliseconds == 1500);
	static_assert(FFBTestPolicy::shake_direction(0) == 1.0f);
	static_assert(FFBTestPolicy::shake_direction(49) == 1.0f);
	static_assert(FFBTestPolicy::shake_direction(50) == -1.0f);
	static_assert(FFBTestPolicy::shake_direction(100) == 1.0f);
	static_assert(!FFBTestPolicy::shake_finished(1199));
	static_assert(FFBTestPolicy::shake_finished(1200));
	static_assert(FFBTestPolicy::shake_must_abort(false, true, true, 0));
	static_assert(FFBTestPolicy::shake_must_abort(true, false, true, 0));
	static_assert(FFBTestPolicy::shake_must_abort(true, true, false, 0));
	static_assert(FFBTestPolicy::shake_must_abort(true, true, true, 1500));
	static_assert(!FFBTestPolicy::shake_must_abort(true, true, true, 1499));
	FFBTestPolicy::ShakeRuntimeLifecycle runtime;
	assert(!runtime.active());
	runtime.started();
	assert(runtime.active());
	runtime.interrupted();
	assert(runtime.result == FFBTestPolicy::ShakeRuntimeResult::Interrupted);
	runtime.reset();
	runtime.started();
	runtime.failed(-7);
	assert(runtime.result == FFBTestPolicy::ShakeRuntimeResult::Failed);
	assert(runtime.lastApiResult == -7);
	runtime.reset();
	runtime.started();
	runtime.completed();
	assert(runtime.result == FFBTestPolicy::ShakeRuntimeResult::Completed);
	static_assert(FFBTestPolicy::reverse_direction(-1.0f, false));
	static_assert(!FFBTestPolicy::reverse_direction(1.0f, false));
	static_assert(!FFBTestPolicy::reverse_direction(-1.0f, true));
	static_assert(FFBTestPolicy::reverse_direction(1.0f, true));

	FFBTestPolicy::QuickSetupShakeVerification verification;
	assert(!verification.may_complete());
	assert(!verification.start(false));
	assert(verification.start(true));
	assert(!verification.start(true));
	verification.fail();
	assert(verification.state == FFBTestPolicy::QuickSetupShakeState::Failed);
	assert(!verification.may_complete());
	verification.reset();
	assert(verification.start(true));
	verification.test_stopped();
	verification.confirm(FFBTestPolicy::PhysicalResponse::No);
	assert(!verification.may_complete());
	verification.reset();
	assert(verification.start(true));
	verification.test_stopped();
	verification.confirm(FFBTestPolicy::PhysicalResponse::Unsure);
	assert(!verification.may_complete());
	verification.reset();
	assert(verification.start(true));
	verification.test_stopped();
	verification.confirm(FFBTestPolicy::PhysicalResponse::Yes);
	assert(verification.may_complete());
	verification.reset();
	verification.skip();
	assert(verification.may_complete());
}
