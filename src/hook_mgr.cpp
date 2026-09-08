#include "hook_mgr.hpp"

Hook::Hook()
{
	HookManager::RegisterHook(this);
}

void HookManager::ApplyHooks()
{
    const auto& registeredHooks = hooks();
    for (size_t index = 0; index < registeredHooks.size(); ++index)
    {
        const auto& hook = registeredHooks[index];
        const auto desc = hook->description();
        const auto label = desc.empty() ? std::string_view("unnamed hook") : desc;

        spdlog::info("Hook {}/{} ({}): declaring settings", index + 1, registeredHooks.size(), label);
        hook->declare_settings();

        hook->is_active_ = false;
        spdlog::info("Hook {}/{} ({}): validating", index + 1, registeredHooks.size(), label);
        if (hook->validate())
        {
            spdlog::info("Hook {}/{} ({}): applying", index + 1, registeredHooks.size(), label);
            hook->is_active_ = hook->apply();

            // Separates a hook that failed to apply from one the user turned
            // off, which the overlay's hook list shows differently.
            hook->has_error_ = !hook->is_active_;

            if (!desc.empty())
            {
                spdlog::log(hook->is_active_ ?
                    spdlog::level::info : spdlog::level::err,
                    "{}: apply {}", desc, hook->is_active_ ? "successful" : "failed");
            }
        }
        else
        {
            spdlog::info("Hook {}/{} ({}): disabled", index + 1, registeredHooks.size(), label);
        }
    }
}
