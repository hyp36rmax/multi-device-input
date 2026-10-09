#include "sound_request_trace.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
	using SoundRequestTrace::decode_command;
	const auto plain = decode_command(0x123);
	assert(plain.soundId == 0x123);
	assert(!plain.loop && !plain.stop);

	const auto loop = decode_command(0x800 | 0x8D);
	assert(loop.soundId == 0x8D);
	assert(loop.loop && !loop.stop);

	const auto stopped = decode_command(0x8000 | 0x8D);
	assert(stopped.soundId == 0x8D);
	assert(stopped.stop && !stopped.loop);

	const auto routed = decode_command(0x1000 | 0x2000 | 0x4000 | 0x44);
	assert(routed.soundId == 0x44);
	assert(routed.panLeft && routed.panRight && routed.panLeftRight);
}
