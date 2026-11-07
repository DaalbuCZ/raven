#pragma once

// The fiber-local state the guest's FlsAlloc/FlsSetValue builds: the indexes in use, the callbacks the guest
// registered for them, and the values each thread keeps. The loader runs those callbacks at thread exit with
// the thread's value for each slot, before the DLL_THREAD_DETACH notifications; a guest whose runtime keeps
// its per-thread state in FLS (the .NET Native runtime among them) needs that delivery to detach cleanly.

#include <functional>
#include <map>
#include <span>
#include <vector>
#include <iostream>

#include "cpu/native_cpu.h"

namespace raven {

struct GuestFls {
	std::vector<bool> used;
	std::vector<uint64_t> callbacks;
	std::map<uint64_t, std::vector<uint64_t>> values;

	using Call = std::function<NativeResult(uint64_t, std::span<const uint64_t>)>;

	// Runs each used slot's callback with the thread's value for it, then clears the thread's values, the way
	// LdrShutdownThread does. A callback that traps propagates its stop to the caller, as TLS callbacks do.
	void detach(uint64_t thread, const Call &call)
	{
		const auto found = values.find(thread);
		if (found == values.end())
			return;
		for (size_t index = 0; index < used.size(); ++index) {
			if (!used[index] || !callbacks[index])
				continue;
			const uint64_t value = index < found->second.size() ? found->second[index] : 0;
			const uint64_t arguments[] = {value};
			if (std::getenv("RAVEN_TRACE_DEBUG"))
				std::cerr << "raven-fls: callback " << index << " 0x" << std::hex << callbacks[index]
					<< " value 0x" << value << std::dec << "\n";
			const auto result = call(callbacks[index], std::span<const uint64_t>(arguments, 1));
			if (result.stop != NativeStop::returned)
				throw result;
		}
		values.erase(found);
	}
};

}
