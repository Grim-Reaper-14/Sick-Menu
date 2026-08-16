#include "core/hooking/HookManager.hpp"

#include <cassert>

namespace
{
    using TestFn = int (*)(int);

    __declspec(noinline) int TargetFunction(int value)
    {
        return value + 1;
    }

    int DetourFunction(int value)
    {
        return value + 10;
    }
}

int main()
{
    auto& hooks = Sick::Hooking::HookManager::Get();
    hooks.Shutdown();
    hooks.ClearDiagnostics();

    assert(hooks.Initialize());
    assert(hooks.Ready());
    assert(hooks.AddDetour("TargetFunction", reinterpret_cast<void*>(&TargetFunction), &DetourFunction));
    assert(hooks.Count() == 1);

    const auto original = hooks.Original<TestFn>("TargetFunction");
    assert(original != nullptr);
    assert(original(5) == 6);

    assert(hooks.EnableAll());
    assert(hooks.Enabled());

    volatile TestFn target = &TargetFunction;
    assert(target(5) == 15);
    assert(original(5) == 6);

    assert(hooks.DisableAll());
    assert(!hooks.Enabled());
    assert(target(5) == 6);

    hooks.Shutdown();
    assert(!hooks.Ready());
    assert(hooks.Count() == 0);
    return 0;
}
