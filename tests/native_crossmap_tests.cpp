#include "game/enhanced/NativeCrossmap.hpp"

#include <array>
#include <cassert>

int main()
{
    using namespace Sick::Game;
    using namespace Sick::Game::Enhanced;

    auto& crossmap = NativeCrossmap::Get();
    crossmap.Clear();

    constexpr BuildId build = 9001;
    constexpr NativeHash originalA = 0x1111222233334444ULL;
    constexpr NativeHash mappedA = 0xAAAABBBBCCCCDDDDULL;
    constexpr NativeHash originalB = 0x5555666677778888ULL;
    constexpr NativeHash mappedB = 0xEEEEFFFF00001111ULL;

    assert(crossmap.Translate(originalA, build) == originalA);
    assert(!crossmap.Reverse(mappedA, build).has_value());

    assert(crossmap.Register(build, originalA, mappedA));
    assert(crossmap.Contains(build, originalA));
    assert(crossmap.Size(build) == 1);
    assert(crossmap.Translate(originalA, build) == mappedA);
    assert(crossmap.Reverse(mappedA, build) == originalA);

    assert(!crossmap.Register(build, originalB, mappedA));

    const std::array entries{
        NativeCrossmapEntry{originalA, mappedA},
        NativeCrossmapEntry{originalB, mappedB},
    };

    assert(crossmap.Load(build, entries));
    assert(crossmap.Size(build) == entries.size());
    assert(crossmap.Translate(originalB, build) == mappedB);
    assert(crossmap.Reverse(mappedB, build) == originalB);

    assert(crossmap.Remove(build, originalA));
    assert(crossmap.Translate(originalA, build) == originalA);
    assert(!crossmap.Reverse(mappedA, build).has_value());

    crossmap.Clear(build);
    assert(crossmap.Size(build) == 0);
    return 0;
}
