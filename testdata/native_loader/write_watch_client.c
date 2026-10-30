#include <windows.h>

#define CHECK(test) do { if (!(test)) return __LINE__; } while (0)

static void *pages[16];
static ULONG_PTR count;
static DWORD granularity;
static volatile LONG finish;
static volatile BYTE *worker_page;

static DWORD WINAPI writer(void *unused)
{
	(void)unused;
	while (!finish) {
		worker_page[0]++;
		SwitchToThread();
	}
	return 0;
}

static UINT query(void *base, SIZE_T size, DWORD flags, ULONG_PTR capacity)
{
	count = capacity;
	return GetWriteWatch(flags, base, size, pages, &count, &granularity);
}

int raven_entry(void)
{
	volatile BYTE *base = VirtualAlloc(NULL, 8 * 4096, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
	CHECK(base);
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && !count && granularity == 4096);
	CHECK(VirtualAlloc((void *)base, 3 * 4096, MEM_COMMIT, PAGE_READWRITE) == base);
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && !count);
	CHECK(base[0] == 0 && base[8192] == 0);
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && !count);
	base[1] = 7;
	base[8193] = 9;
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && count == 2 && pages[0] == base && pages[1] == base + 8192);
	CHECK(!query((void *)base, 8 * 4096, WRITE_WATCH_FLAG_RESET, 1) && count == 1 && pages[0] == base);
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && count == 1 && pages[0] == base + 8192);
	CHECK(!ResetWriteWatch((void *)(base + 8193), 1));
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && !count);
	base[0] = 7; /* A same-value write still counts. */
	volatile BYTE *other = VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
	CHECK(other);
	other[0] = 2;
	CHECK(!ResetWriteWatch((void *)other, 4096));
	CHECK(VirtualAlloc((void *)(base + 3 * 4096), 4096, MEM_COMMIT, PAGE_READWRITE) == base + 3 * 4096);
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && count == 1 && pages[0] == base);
	DWORD old;
	CHECK(VirtualProtect((void *)base, 4096, PAGE_READONLY, &old));
	CHECK(VirtualProtect((void *)base, 4096, PAGE_READWRITE, &old));
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && count == 1 && pages[0] == base);
	CHECK(query((void *)base, 8 * 4096, 2, 16) == ~0U && GetLastError() == ERROR_INVALID_PARAMETER);
	CHECK(query((void *)base, 8 * 4096, 0, 0) == ~0U && GetLastError() == ERROR_INVALID_PARAMETER);
	CHECK(ResetWriteWatch((void *)base, 0) == ~0U && GetLastError() == ERROR_INVALID_PARAMETER);
	count = 16;
	CHECK(GetWriteWatch(WRITE_WATCH_FLAG_RESET, (void *)base, 8 * 4096, NULL, &count, &granularity) == ~0U);
	CHECK(GetLastError() == ERROR_NOACCESS);
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && count == 1);
	CHECK(!ResetWriteWatch((void *)base, 8 * 4096));
	GetSystemTimeAsFileTime((FILETIME *)(base + 4096)); /* A write made by an API handler counts too. */
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && count == 1 && pages[0] == base + 4096);
	CHECK(VirtualFree((void *)(base + 4096), 4096, MEM_DECOMMIT));
	CHECK(VirtualAlloc((void *)(base + 4096), 4096, MEM_COMMIT, PAGE_READWRITE) == base + 4096);
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && !count && base[4096] == 0);
	worker_page = base;
	HANDLE thread = CreateThread(NULL, 0, writer, NULL, 0, NULL);
	CHECK(thread);
	for (unsigned i = 0; i < 20; ++i) {
		SwitchToThread();
		CHECK(!ResetWriteWatch((void *)(base + 4096), 4096));
	}
	finish = 1;
	CHECK(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0);
	CHECK(CloseHandle(thread));
	CHECK(!query((void *)base, 8 * 4096, 0, 16) && count == 1 && pages[0] == base);
	CHECK(VirtualFree((void *)other, 0, MEM_RELEASE));
	CHECK(VirtualFree((void *)base, 0, MEM_RELEASE));
	CHECK(ResetWriteWatch((void *)base, 4096) == ~0U);
	CHECK(!VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE) && GetLastError() == ERROR_INVALID_PARAMETER);
	return 0;
}
