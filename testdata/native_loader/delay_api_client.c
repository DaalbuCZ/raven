#include <windows.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
	DWORD size;
	void *descriptor, *thunk;
	const char *dll;
	DWORD by_name;
	const char *name;
	void *module, *unused;
	DWORD error;
} failure_info;

typedef void *(WINAPI *resolve_fn)(void *, void *, void *, void *, void *, ULONG);
static int observed;
static int decline, system_decline;

static void *WINAPI dll_failure(ULONG reason, failure_info *info)
{
	if (reason == 4 && info->size == sizeof(*info) && info->descriptor && info->thunk &&
	    info->dll[0] == 'r' && info->by_name && info->name[0] == 'M' && !info->module &&
	    !info->unused && info->error == 0xc0000135U)
		observed++;
	return decline ? NULL : (void *)(uintptr_t)0x1234;
}

static void *WINAPI system_failure(const char *dll, const char *name)
{
	if (dll[0] == 'r' && name[0] == 'M')
		observed++;
	return system_decline ? NULL : (void *)(uintptr_t)0x5678;
}

int raven_entry(void)
{
	struct {
		DWORD descriptor[8];
		uintptr_t handle, thunk, import;
		char dll[40];
		WORD hint;
		char name[16];
	} image = {0};
#define RVA(field) ((DWORD)((char *)&image.field - (char *)&image))
	image.descriptor[0] = 1;
	image.descriptor[1] = RVA(dll);
	image.descriptor[2] = RVA(handle);
	image.descriptor[3] = RVA(thunk);
	image.descriptor[4] = RVA(import);
	image.import = RVA(hint);
	const char dll[] = "raven-missing-delay-test.dll", name[] = "Missing";
	for (unsigned i = 0; i < sizeof(dll); ++i) image.dll[i] = dll[i];
	for (unsigned i = 0; i < sizeof(name); ++i) image.name[i] = name[i];
	resolve_fn resolve = (resolve_fn)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ResolveDelayLoadedAPI");
	if (!resolve) return 1;
	if (resolve(&image, image.descriptor, dll_failure, system_failure, &image.thunk, 0) != (void *)(uintptr_t)0x1234 || observed != 1)
		return 2;
	if (resolve(&image, image.descriptor, NULL, system_failure, &image.thunk, 0) != (void *)(uintptr_t)0x5678 || observed != 2)
		return 3;
	if (image.thunk || image.handle) return 4;
	decline = 1;
	if (resolve(&image, image.descriptor, dll_failure, system_failure, &image.thunk, 0) != (void *)(uintptr_t)0x5678 || observed != 4)
		return 5;
	if (image.thunk || image.handle) return 6;
	if (resolve(&image, image.descriptor, dll_failure, NULL, &image.thunk, 0) || observed != 5)
		return 7;
	system_decline = 1;
	if (resolve(&image, image.descriptor, dll_failure, system_failure, &image.thunk, 0) || observed != 7)
		return 8;
	if (image.thunk || image.handle) return 9;
	return 0;
}
