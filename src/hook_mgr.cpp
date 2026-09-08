#include "hook_mgr.hpp"

#include <Windows.h>

namespace
{
    struct HookExceptionDetails
    {
        DWORD code = 0;
        void* instruction = nullptr;
        ULONG_PTR operation = 0;
        ULONG_PTR target = 0;
    };

    LONG CaptureHookException(EXCEPTION_POINTERS* exception, HookExceptionDetails* details)
    {
        const auto* record = exception->ExceptionRecord;
        details->code = record->ExceptionCode;
        details->instruction = record->ExceptionAddress;

        if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
        {
            details->operation = record->ExceptionInformation[0];
            details->target = record->ExceptionInformation[1];
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    // SafetyHook can encounter structured Windows exceptions while decoding or
    // allocating trampoline code. Keep those failures local to the optional
    // hook and retain the native exception code for useful diagnostics.
    bool ApplyHookWithSeh(Hook* hook, HookExceptionDetails& details)
    {
        __try
        {
            return hook->apply();
        }
        __except (CaptureHookException(GetExceptionInformation(), &details))
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
    size_t activeCount = 0;
    size_t disabledCount = 0;
    size_t failedCount = 0;
    for (size_t index = 0; index < registeredHooks.size(); ++index)
    {
        const auto& hook = registeredHooks[index];
        const auto desc = hook->description();
        const auto label = desc.empty() ? std::string_view("unnamed hook") : desc;

        hook->is_active_ = false;
        hook->has_error_ = false;

        try
        {
            hook->declare_settings();

            if (hook->validate())
            {
                HookExceptionDetails exception;
                hook->is_active_ = ApplyHookWithSeh(hook, exception);

                if (exception.code != 0)
                {
                    hook->has_error_ = true;
                    ++failedCount;
                    const char* operation = exception.operation == 0 ? "read" :
                        exception.operation == 1 ? "write" :
                        exception.operation == 8 ? "execute" : "unknown operation";

                    HMODULE faultModule = nullptr;
                    GetModuleHandleExA(
                        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                        reinterpret_cast<LPCSTR>(exception.instruction),
                        &faultModule);

                    const auto moduleBase = reinterpret_cast<uintptr_t>(faultModule);
                    const auto instruction = reinterpret_cast<uintptr_t>(exception.instruction);
                    const auto moduleOffset = faultModule ? instruction - moduleBase : 0;
                    char modulePath[MAX_PATH] = {};
                    if (faultModule)
                        GetModuleFileNameA(faultModule, modulePath, MAX_PATH);

                    spdlog::error(
                        "Hook {}/{} ({}): Windows exception 0x{:08X} at {:p} "
                        "(module '{}' {:p}+0x{:X}); {} access at 0x{:X}",
                        index + 1, registeredHooks.size(), label, exception.code,
                        exception.instruction, modulePath[0] ? modulePath : "unknown",
                        static_cast<void*>(faultModule), moduleOffset,
                        operation, exception.target);
                    continue;
                }

                // Separates a hook that failed to apply from one the user turned
                // off, which the overlay's hook list shows differently.
                hook->has_error_ = !hook->is_active_;

				if (hook->is_active_)
					++activeCount;
				else
				{
					++failedCount;
					spdlog::error("Hook {}/{} ({}): failed to apply", index + 1, registeredHooks.size(), label);
				}
            }
            else
            {
				++disabledCount;
            }
        }
        catch (const std::exception& error)
        {
            hook->has_error_ = true;
            ++failedCount;
            spdlog::error("Hook {}/{} ({}): skipped after exception: {}",
                index + 1, registeredHooks.size(), label, error.what());
        }
        catch (...)
        {
            hook->has_error_ = true;
            ++failedCount;
            spdlog::error("Hook {}/{} ({}): skipped after unknown exception",
                index + 1, registeredHooks.size(), label);
        }
    }

	spdlog::info("Hook initialization complete: {} active, {} disabled, {} failed",
		activeCount, disabledCount, failedCount);
}
