#include "kernel_context.h"

#ifdef RAVEN_SKIA
#include "graphics/canvas.h"
#endif

#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

namespace raven {

#ifdef RAVEN_SKIA

// The host side of the Direct2D component: surfaces live here and are drawn into with Skia; the guest holds their numbers, sends one
// command record per drawing operation and moves pixels only when a program reads or presents them.
void register_canvas_service(ApiRegistry &registry, KernelContext &)
{
	registry.add("RavenCanvasCreate", 2, [](ApiCall &call) { call.ret(canvas::create(call.u32(0), call.u32(1))); });
	registry.add("RavenCanvasDestroy", 1, [](ApiCall &call) { canvas::destroy(call.u32(0)); });
	// RavenCanvasSize(surface, size): the width and height as two words; zero when there is no such surface.
	registry.add("RavenCanvasSize", 2, [](ApiCall &call) {
		uint32_t width = 0, height = 0;
		const bool found = canvas::size(call.u32(0), width, height);
		if (found && call[1]) {
			call.store<uint32_t>(call[1], width);
			call.store<uint32_t>(call[1] + 4, height);
		}
		call.ret(found);
	});
	// RavenCanvasPixels(surface, x, y, width, height, buffer, pitch, write).
	registry.add("RavenCanvasPixels", 8, [](ApiCall &call) {
		const uint32_t width = call.u32(3), height = call.u32(4), pitch = call.u32(6);
		if (!width || !height || !call[5] || pitch < width * 4 || uint64_t(pitch) * height > (512ull << 20)) {
			call.ret(0);
			return;
		}
		const size_t bytes = size_t(pitch) * (height - 1) + size_t(width) * 4;
		// Pixels written come straight from the guest's rows; pixels read are gathered before they go back.
		const bool write = call.u32(7) != 0;
		if (write) {
			auto source = call.read(call[5], bytes);
			call.ret(canvas::pixels(call.u32(0), call.i32(1), call.i32(2), width, height, source.data(), pitch, true));
			return;
		}
		// Rows that fill their pitch are read whole from the surface, so their buffer starts unset; gaps between rows start zeroed.
		const std::unique_ptr<uint8_t[]> buffer(pitch == width * 4 ? new uint8_t[bytes] : new uint8_t[bytes]());
		if (!canvas::pixels(call.u32(0), call.i32(1), call.i32(2), width, height, buffer.get(), pitch, false)) {
			call.ret(0);
			return;
		}
		call.write(call[5], std::span<const uint8_t>(buffer.get(), bytes));
		call.ret(1);
	});
	// RavenPathCombine(one, one floats, two, two floats, fills, mode, result, capacity): two paths in the command's float form combined;
	// the fill modes are bits 0 and 1 of `fills`, the result's fill mode comes back in bit 31 of the answer, which is the float count, or
	// 0x7fffffff when the paths cannot be combined. A result larger than the capacity is counted and not written.
	registry.add("RavenPathCombine", 8, [](ApiCall &call) {
		const auto floats = [&](uint64_t address, uint32_t count) {
			std::vector<float> values(count);
			if (count) {
				const auto bytes = call.read(address, size_t(count) * 4);
				std::memcpy(values.data(), bytes.data(), bytes.size());
			}
			return values;
		};
		if (call.u32(1) > (16u << 20) || call.u32(3) > (16u << 20)) {
			call.ret(0x7fffffff);
			return;
		}
		const auto one = floats(call[0], call.u32(1)), two = floats(call[2], call.u32(3));
		std::vector<float> result;
		uint32_t fill = 0;
		if (!canvas::combine(one, call.u32(4) & 1, two, (call.u32(4) >> 1) & 1, call.u32(5), result, fill) || result.size() >= 0x7fffffff) {
			call.ret(0x7fffffff);
			return;
		}
		if (call[6] && result.size() <= call.u32(7) && !result.empty())
			call.write(call[6], std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(result.data()), result.size() * 4));
		call.ret(uint32_t(result.size()) | (fill << 31));
	});
	// RavenPathWiden(path, floats, fill, stroke, stroke floats, result, capacity): the area a stroke covers, answered as RavenPathCombine
	// answers.
	registry.add("RavenPathWiden", 7, [](ApiCall &call) {
		const auto floats = [&](uint64_t address, uint32_t count) {
			std::vector<float> values(count);
			if (count) {
				const auto bytes = call.read(address, size_t(count) * 4);
				std::memcpy(values.data(), bytes.data(), bytes.size());
			}
			return values;
		};
		if (call.u32(1) > (16u << 20) || call.u32(4) > 128) {
			call.ret(0x7fffffff);
			return;
		}
		const auto path = floats(call[0], call.u32(1)), stroke = floats(call[3], call.u32(4));
		std::vector<float> result;
		uint32_t fill = 0;
		if (!canvas::widen(path, call.u32(2), stroke, result, fill) || result.size() >= 0x7fffffff) {
			call.ret(0x7fffffff);
			return;
		}
		if (call[5] && result.size() <= call.u32(6) && !result.empty())
			call.write(call[5], std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(result.data()), result.size() * 4));
		call.ret(uint32_t(result.size()) | (fill << 31));
	});
	// RavenCanvasDraw(surface, command, data, size): one command record and the variable parts after it.
	registry.add("RavenCanvasDraw", 4, [](ApiCall &call) {
		if (!call[1] || call.u32(3) > (300u << 20)) {
			call.ret(0);
			return;
		}
		canvas::Command command;
		const auto record = call.read(call[1], sizeof command);
		std::memcpy(&command, record.data(), sizeof command);
		const auto data = call.u32(3) ? call.read(call[2], call.u32(3)) : std::vector<uint8_t>();
		call.ret(canvas::draw(call.u32(0), command, data));
	});
	// RavenCanvasBatch(records, size): the commands d2d1 queued, run in order. A record is a header of four words (the surface, the size
	// of the variable parts, a status set to 1 for a command that ran, and the kind: 0 for a command, 1 for pixels written), the command,
	// its variable parts and padding to eight bytes. The result is how many failed; only then are the statuses written back.
	registry.add("RavenCanvasBatch", 2, [](ApiCall &call) {
		constexpr size_t header = 16;
		if (!call[0] || call.u32(1) > (64u << 20)) {
			call.ret(UINT32_MAX);
			return;
		}
		auto records = call.read(call[0], call.u32(1));
		uint32_t failed = 0;
		for (size_t at = 0; at + header + sizeof(canvas::Command) <= records.size();) {
			uint32_t words[2];
			std::memcpy(words, records.data() + at, sizeof words);
			const size_t parts = at + header + sizeof(canvas::Command), end = parts + words[1];
			if (end > records.size()) {
				++failed;
				break;
			}
			uint32_t status = 0, kind = 0;
			std::memcpy(&kind, records.data() + at + 12, 4);
			if (kind == 1) {
				// Pixels written: where they go in the command's place, the rows packed after it.
				int32_t place[4];
				std::memcpy(place, records.data() + at + header, sizeof place);
				const uint32_t width = uint32_t(place[2]), height = uint32_t(place[3]);
				if (width && height && uint64_t(width) * 4 * height <= words[1])
					status = canvas::pixels(words[0], place[0], place[1], width, height, records.data() + parts, width * 4, true) ? 1 : 0;
			} else {
				canvas::Command command;
				std::memcpy(&command, records.data() + at + header, sizeof command);
				status = canvas::draw(words[0], command, std::span<const uint8_t>(records.data() + parts, words[1])) ? 1 : 0;
			}
			std::memcpy(records.data() + at + 8, &status, 4);
			failed += 1 - status;
			at = (end + 7) & ~size_t(7);
		}
		if (failed)
			call.write(call[0], records);
		call.ret(failed);
	});
}

#else

// Without the Skia rasterizer the canvas service still serves the surfaces themselves: the memory a render target draws into
// exists, its pixels can be read and written, and a drawing command is dropped loudly by name, the way a stub says what it
// cannot do. A Skia build (scripts/provision-text.py, configured as RAVEN_SKIA_SOURCE/RAVEN_SKIA_LIBRARY) replaces the
// dropped commands with real rasterization.
void register_canvas_service(ApiRegistry &registry, KernelContext &)
{
	static std::map<uint32_t, std::pair<uint32_t, std::vector<uint8_t>>> surfaces;
	static uint32_t next_surface = 1;
	static unsigned long dropped_draws = 0;
	registry.add("RavenCanvasCreate", 2, [](ApiCall &call) {
		const uint32_t width = call.u32(0), height = call.u32(1);
		// A surface of BGRA bytes premultiplied; a request beyond the memory the service keeps is refused.
		if (!width || !height || uint64_t(width) * height > (256ull << 20) / 4) {
			call.ret(0);
			return;
		}
		const uint32_t id = next_surface++;
		auto &surface = surfaces[id];
		surface.first = width;
		surface.second.assign(size_t(width) * height * 4, 0);
		call.ret(id);
	});
	registry.add("RavenCanvasDestroy", 1, [](ApiCall &call) { surfaces.erase(call.u32(0)); });
	// RavenCanvasSize(surface, size): the width and height as two words; zero when there is no such surface.
	registry.add("RavenCanvasSize", 2, [](ApiCall &call) {
		const auto found = surfaces.find(call.u32(0));
		if (found == surfaces.end()) {
			call.ret(0);
			return;
		}
		if (call[1]) {
			call.store<uint32_t>(call[1], found->second.first);
			call.store<uint32_t>(call[1] + 4, found->second.second.size() / 4 / found->second.first);
		}
		call.ret(1);
	});
	// RavenCanvasPixels(surface, x, y, width, height, buffer, pitch, write): rows move whole between the guest and the surface.
	registry.add("RavenCanvasPixels", 8, [](ApiCall &call) {
		const auto found = surfaces.find(call.u32(0));
		const uint32_t width = call.u32(3), height = call.u32(4), pitch = call.u32(6);
		const int32_t x = call.i32(1), y = call.i32(2);
		if (found == surfaces.end() || !width || !height || !call[5] || pitch < uint64_t(width) * 4 ||
			x < 0 || y < 0 || uint64_t(x) + width > found->second.first ||
			uint64_t(y) + height > found->second.second.size() / 4 / found->second.first ||
			uint64_t(pitch) * height > (512ull << 20)) {
			call.ret(0);
			return;
		}
		const bool write = call.u32(7) != 0;
		const size_t bytes = size_t(pitch) * (height - 1) + size_t(width) * 4;
		if (write) {
			const auto source = call.read(call[5], bytes);
			for (uint32_t row = 0; row < height; ++row)
				std::memcpy(found->second.second.data() + (size_t(y + row) * found->second.first + x) * 4,
					source.data() + size_t(row) * pitch, size_t(width) * 4);
		} else {
			std::vector<uint8_t> buffer(bytes, 0);
			for (uint32_t row = 0; row < height; ++row)
				std::memcpy(buffer.data() + size_t(row) * pitch,
					found->second.second.data() + (size_t(y + row) * found->second.first + x) * 4, size_t(width) * 4);
			call.write(call[5], std::span<const uint8_t>(buffer.data(), bytes));
		}
		call.ret(1);
	});
	// RavenCanvasDraw(surface, command, data, size) and RavenCanvasBatch(records, size): the rasterizer they need is not in this
	// build, so a command is dropped and said so; the surfaces stay what they were, and a program that reads pixels back reads
	// the zeros a never-drawn surface carries. A batch that ran nothing failed nothing, which is the answer the queue's caller
	// takes to keep going.
	registry.add("RavenCanvasDraw", 4, [](ApiCall &call) {
		++dropped_draws;
		if (dropped_draws <= 20 || dropped_draws % 1000 == 0)
			std::fprintf(stderr, "raven: canvas draw dropped, no Skia rasterizer in this build (%lu so far, %u bytes)\n",
				dropped_draws, call.u32(3));
		call.ret(1);
	});
	registry.add("RavenCanvasBatch", 2, [](ApiCall &call) {
		++dropped_draws;
		if (dropped_draws <= 20 || dropped_draws % 1000 == 0)
			std::fprintf(stderr, "raven: canvas batch dropped, no Skia rasterizer in this build (%lu commands so far, %u bytes)\n",
				dropped_draws, call.u32(1));
		call.ret(0);
	});
}

#endif

}
