#include "nt/providers.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
	try {
		check(argc == 4, "bootstrap, CoreWindow fixture and isolated prefix required");
		check(setenv("RAVEN_PREFIX", argv[3], 1) == 0,
			"cannot configure CoreWindow presentation fixture");
		auto bytes = raven::ModuleBytes::map(argv[2]);
		check(bytes.has_value(), "cannot map CoreWindow fixture");
		raven::ModuleSet modules;
		auto &entry = modules.add("core-desktop.exe", std::move(*bytes));
		const auto components = std::filesystem::path(argv[1]).parent_path() / (entry.image.is_64 ? "" : "x86");
		for (const auto *name : {"raven-shell.dll", "raven-xaml.dll"}) {
			auto component = raven::ModuleBytes::map(components / name);
			check(component.has_value(), "CoreWindow runtime component missing");
			modules.add(name, std::move(*component));
		}
		if (entry.image.is_64) {
			auto heap = raven::ModuleBytes::map(components / "raven-rtl.dll");
			check(heap.has_value(), "CoreWindow heap component missing");
			modules.add("raven-rtl.dll", std::move(*heap));
		}
		raven::NativeLaunch launch;
		launch.image_path = u"C:\\core-desktop.exe";
		launch.command_line = launch.image_path;
		launch.environment = u"RAVEN_TEST_CORE_DESKTOP=1";
		launch.environment->append(2, u'\0');
		const auto source = [&](std::string_view name) {
			return raven::ModuleBytes::map(components / std::filesystem::path(name).filename());
		};
		uint64_t window = 0;
		unsigned frames = 0, removed = 0;
		bool green = false, closed = false;
		raven::DesktopPresentation presentation;
		presentation.present = [&](uint64_t handle, std::string_view title, uint32_t width, uint32_t height, std::span<const uint8_t> pixels) {
			check(handle && (!window || handle == window) && title == "Raven CoreWindow fixture" &&
				width && height && pixels.size() == size_t(width) * height * 4, "CoreWindow surface identity differs");
			window = handle;
			++frames;
			size_t colored = 0;
			for (size_t pixel = 0; pixel < pixels.size(); pixel += 4)
				if (pixels[pixel] == 0 && pixels[pixel + 1] == 255 && pixels[pixel + 2] == 0) ++colored;
			green |= colored >= 10000;
		};
		presentation.remove = [&](uint64_t handle) {
			check(handle == window, "CoreWindow destroyed the wrong host surface");
			++removed;
		};
		presentation.poll = [&](std::chrono::milliseconds) {
			std::vector<raven::DesktopEvent> events;
			if (green && !closed) {
				closed = true;
				events.push_back({window, 0x10});
			}
			return events;
		};
		const auto result = entry.image.is_64 ?
			raven::run_native_process(modules, entry, argv[1], std::chrono::seconds(15), source, {}, presentation, launch) :
			raven::run_native_process_x86(modules, entry, argv[1], std::chrono::seconds(15), launch, presentation, source);
		if (result.stop != raven::NativeStop::returned || result.value)
			std::cerr << "CoreWindow stop=" << int(result.stop) << " value=" << result.value << '\n';
		check(result.stop == raven::NativeStop::returned && !result.value, "CoreWindow desktop contract failed");
		if (!frames || !green || !closed || removed != 1)
			std::cerr << "frames=" << frames << " green=" << green << " closed=" << closed << " removed=" << removed << '\n';
		check(frames && green && closed && removed == 1, "CoreWindow did not present its client pixels and handle host close");
		std::cout << "CoreWindow HWND, keyboard events, visible client pixels and host close verified\n";
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
	return 0;
}
