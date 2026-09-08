#include "hook_mgr.hpp"

#include <Windows.h>

namespace
{
    // SafetyHook can encounter structured Windows exceptions while decoding or
    // allocating trampoline code. Keep those failures local to the optional
    // hook and retain the native exception code for useful diagnostics.
    bool ApplyHookWithSeh(Hook* hook, DWORD& exceptionCode)
    {
        __try
        {
            return hook->apply();
        }
        __except (exceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
}

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

        hook->is_active_ = false;
        hook->has_error_ = false;

        try
        {
            spdlog::info("Hook {}/{} ({}): declaring settings", index + 1, registeredHooks.size(), label);
            hook->declare_settings();

            spdlog::info("Hook {}/{} ({}): validating", index + 1, registeredHooks.size(), label);
            if (hook->validate())
            {
                spdlog::info("Hook {}/{} ({}): applying", index + 1, registeredHooks.size(), label);
                DWORD exceptionCode = 0;
                hook->is_active_ = ApplyHookWithSeh(hook, exceptionCode);

                if (exceptionCode != 0)
                {
                    hook->has_error_ = true;
                    spdlog::error("Hook {}/{} ({}): skipped after Windows exception 0x{:08X}",
                        index + 1, registeredHooks.size(), label, exceptionCode);
                    continue;
                }

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
        catch (const std::exception& error)
        {
            hook->has_error_ = true;
            spdlog::error("Hook {}/{} ({}): skipped after exception: {}",
                index + 1, registeredHooks.size(), label, error.what());
        }
        catch (...)
        {
            hook->has_error_ = true;
            spdlog::error("Hook {}/{} ({}): skipped after unknown exception",
                index + 1, registeredHooks.size(), label);
        }
    }
}
