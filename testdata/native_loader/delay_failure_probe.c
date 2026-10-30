/* Observes the public delay-load failure ABI without inspecting implementation code. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

typedef void *(WINAPI *hook_fn)(const char *, const char *);
typedef void *(WINAPI *resolve_fn)(void *, void *, void *, void *, void *, ULONG);
static const char missing_dll[] = "raven-missing-delay-test.dll";
static const char missing_proc[] = "RavenMissingDelayExport";
static const char *selected_dll, *selected_proc;
static unsigned selected_case;
static void write_text(const char *text)
{
	DWORD size = 0, written;
	while (text[size]) ++size;
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text, size, &written, NULL);
}
static void write_value(const char *label, ULONG_PTR value)
{
	char digits[2 * sizeof(value) + 2];
	const char hex[] = "0123456789abcdef";
	for (unsigned i = 0; i < 2 * sizeof(value); ++i)
		digits[i] = hex[(value >> ((2 * sizeof(value) - i - 1) * 4)) & 15];
	digits[2 * sizeof(value)] = '\n';
	digits[2 * sizeof(value) + 1] = 0;
	write_text(label);
	write_text(digits);
}
static LONG WINAPI observe(PEXCEPTION_POINTERS pointers)
{
	const EXCEPTION_RECORD *record = pointers->ExceptionRecord;
	write_value("exception_code=", record->ExceptionCode);
	write_value("exception_flags=", record->ExceptionFlags);
	write_value("parameter_count=", record->NumberParameters);
	for (unsigned i = 0; i < record->NumberParameters && i < EXCEPTION_MAXIMUM_PARAMETERS; ++i) {
		write_value("parameter_index=", i);
		write_value("parameter_value=", record->ExceptionInformation[i]);
		write_value("matches_dll=", record->ExceptionInformation[i] == (ULONG_PTR)selected_dll);
		write_value("matches_proc=", record->ExceptionInformation[i] == (ULONG_PTR)selected_proc);
	}
	/* Only decode the documented DelayLoadInfo data structure, never arbitrary memory or executable bytes. */
	if (record->NumberParameters == 1 && record->ExceptionInformation[0]) {
		struct {
			DWORD size;
			void *descriptor, *thunk;
			const char *dll;
			DWORD by_name;
			const char *name;
			void *module, *unused;
			DWORD error;
		} info;
		SIZE_T read = 0;
		if (ReadProcessMemory(GetCurrentProcess(), (void *)record->ExceptionInformation[0], &info, sizeof(info), &read) &&
			read == sizeof(info) && info.size == sizeof(info)) {
			write_value("delay_info_size=", info.size);
			write_value("delay_info_by_name=", info.by_name);
			write_value("delay_info_error=", info.error);
			write_value("delay_info_matches_dll=", info.dll == selected_dll);
			write_value("delay_info_matches_proc=", info.name == selected_proc);
			write_value("delay_info_has_descriptor=", info.descriptor != NULL);
			write_value("delay_info_has_thunk=", info.thunk != NULL);
			write_value("delay_info_has_module=", info.module != NULL);
		}
	}
	ExitProcess(0);
	return EXCEPTION_CONTINUE_SEARCH;
}
int raven_entry(void)
{
	char mode[8] = {0};
	GetEnvironmentVariableA("RAVEN_DELAY_PROBE_CASE", mode, sizeof(mode));
	selected_case = mode[0] >= '1' && mode[0] <= '4' ? (unsigned)(mode[0] - '0') : 1;
	write_value("case=", selected_case);
	write_value("pointer_size=", sizeof(void *));
	HMODULE kernel = GetModuleHandleA("kernel32.dll");
	hook_fn hook = (hook_fn)GetProcAddress(kernel, "DelayLoadFailureHook");
	resolve_fn resolve = (resolve_fn)GetProcAddress(kernel, "ResolveDelayLoadedAPI");
	if (!hook || !resolve || !AddVectoredExceptionHandler(1, observe)) return 2;
	selected_dll = missing_dll;
	selected_proc = selected_case == 2 ? (const char *)(ULONG_PTR)123 : missing_proc;
	if (selected_case <= 2) {
		SetLastError(selected_case == 1 ? ERROR_MOD_NOT_FOUND : ERROR_PROC_NOT_FOUND);
		write_value("returned=", (ULONG_PTR)hook(selected_dll, selected_proc));
	} else {
		struct {
			DWORD descriptor[8];
			uintptr_t handle, thunk, import;
			char dll[40];
			WORD hint;
			char name[32];
		} image = {0};
#define RVA(field) ((DWORD)((char *)&image.field - (char *)&image))
		image.descriptor[0] = 1;
		image.descriptor[1] = RVA(dll);
		image.descriptor[2] = RVA(handle);
		image.descriptor[3] = RVA(thunk);
		image.descriptor[4] = RVA(import);
		image.import = RVA(hint);
		const char *dll = selected_case == 3 ? missing_dll : "kernel32.dll";
		for (unsigned i = 0; dll[i]; ++i) image.dll[i] = dll[i];
		for (unsigned i = 0; missing_proc[i]; ++i) image.name[i] = missing_proc[i];
		selected_dll = image.dll;
		selected_proc = image.name;
		write_value("returned=", (ULONG_PTR)resolve(&image, image.descriptor, NULL, (void *)hook, &image.thunk, 0));
		write_value("thunk=", image.thunk);
	}
	return 0;
}
