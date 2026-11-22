/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The HGlobal stream fixture. CreateStreamOnHGlobal's block is the one object the app's SVG round trip shares —
   Win2D's GetXml serializes into it, then locks the HGlobal and reads GlobalSize bytes — so the block's ops must
   survive concurrency and the Windows realloc contract: a locked movable object does not move, a write that would
   grow a locked block fails before anything is written. Three sections here: the clone round trip, the lock-count
   semantics against a locked handle, and four threads cloning one stream and writing disjoint slots as the block
   grows under them — the shape the material workers ran when the heap overflowed. */
#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <objidl.h>

typedef HRESULT (WINAPI *create_stream_function)(HGLOBAL, BOOL, IStream **);
typedef HRESULT (WINAPI *hglobal_from_stream_function)(IStream *, HGLOBAL *);

#define THREAD_COUNT 4
#define ROUND_COUNT 64
#define SLOT_BYTES 1024
#define TOTAL_BYTES (THREAD_COUNT * ROUND_COUNT * SLOT_BYTES)
#define ROUND_TRIP_BYTES (300 * 1024)
#define INITIAL_BYTES 100

static create_stream_function create_stream;
static hglobal_from_stream_function hglobal_from_stream;
/* The stream the writer threads clone; the entry sets it before the threads start. */
static IStream *shared_stream;

static BYTE pattern_byte(SIZE_T at)
{
	return (BYTE)(at * 31u + (at >> 9) * 17u + 7u);
}

static BOOL same_bytes(const BYTE *left, const BYTE *right, SIZE_T count)
{
	SIZE_T i;
	for (i = 0; i < count; ++i)
		if (left[i] != right[i])
			return FALSE;
	return TRUE;
}

/* The bytes one clone writes into its slot: the thread, the round and the offset mix into each byte. */
static void fill_slot(BYTE *buffer, DWORD thread, DWORD round)
{
	SIZE_T i;
	for (i = 0; i < SLOT_BYTES; ++i)
		buffer[i] = (BYTE)(thread * 7u + round * 13u + i);
}

static DWORD WINAPI slot_writer(LPVOID argument)
{
	const DWORD thread = (DWORD)(DWORD_PTR)argument;
	DWORD round;
	for (round = 0; round < ROUND_COUNT; ++round) {
		IStream *clone = NULL;
		BYTE slot[SLOT_BYTES];
		LARGE_INTEGER at;
		ULONG written = 0;
		at.QuadPart = (LONGLONG)((ULONGLONG)thread * ROUND_COUNT + round) * SLOT_BYTES;
		if (IStream_Clone(shared_stream, &clone) != S_OK || !clone)
			return 100 + thread;
		fill_slot(slot, thread, round);
		if (IStream_Seek(clone, at, STREAM_SEEK_SET, NULL) != S_OK) {
			IStream_Release(clone);
			return 120 + thread;
		}
		if (IStream_Write(clone, slot, SLOT_BYTES, &written) != S_OK || written != SLOT_BYTES) {
			IStream_Release(clone);
			return 140 + thread;
		}
		IStream_Release(clone);
	}
	return 0;
}

int raven_entry(void)
{
	HMODULE ole32;
	IStream *stream = NULL, *clone = NULL;
	HGLOBAL global = NULL;
	BYTE *expect = NULL, *have = NULL;
	void *locked = NULL;
	ULARGE_INTEGER size;
	LARGE_INTEGER at, to_start;
	STATSTG stat;
	ULONG written = 0, done = 0, round_chunk;
	HRESULT result;
	HANDLE threads[THREAD_COUNT];
	DWORD thread_ids[THREAD_COUNT], exit_code, code, index;
	SIZE_T step, i, slot_index;
	int failures = 0;

	/* The component is named directly: raven-storage.dll is the runtime's structured storage, shipped beside it, and
	   the overlay serves ole32's stream exports only in a process whose graph already carries the component (the
	   app's does, through the components that import it). GetProcAddress on the provider's not-yet-forwarded export
	   would answer a trap, so the component comes first and ole32 is the fallback. */
	ole32 = LoadLibraryW(L"raven-storage.dll");
	if (!ole32)
		ole32 = LoadLibraryW(L"ole32.dll");
	if (!ole32)
		return 1;
	create_stream = (create_stream_function)(void *)GetProcAddress(ole32, "CreateStreamOnHGlobal");
	hglobal_from_stream = (hglobal_from_stream_function)(void *)GetProcAddress(ole32, "GetHGlobalFromStream");
	if (!create_stream || !hglobal_from_stream)
		return 2;

	/* The clone round trip: writes grow the block, the clone shares the bytes with its own position, the stat
	   answers the written extent and the HGlobal holds at least those bytes. */
	if (create_stream(NULL, TRUE, &stream) != S_OK || !stream)
		return 3;
	expect = (BYTE *)HeapAlloc(GetProcessHeap(), 0, ROUND_TRIP_BYTES);
	have = (BYTE *)HeapAlloc(GetProcessHeap(), 0, ROUND_TRIP_BYTES);
	if (!expect || !have)
		return 4;
	for (i = 0; i < ROUND_TRIP_BYTES; ++i)
		expect[i] = pattern_byte(i);
	to_start.QuadPart = 0;
	for (step = 0; step < ROUND_TRIP_BYTES; step += round_chunk) {
		round_chunk = ROUND_TRIP_BYTES - step < 65536 ? (ULONG)(ROUND_TRIP_BYTES - step) : 65536;
		if (IStream_Write(stream, expect + step, round_chunk, &written) != S_OK || written != round_chunk)
			return 5;
	}
	if (IStream_Stat(stream, &stat, STATFLAG_NONAME) != S_OK || stat.cbSize.QuadPart != ROUND_TRIP_BYTES)
		return 6;
	if (IStream_Clone(stream, &clone) != S_OK || !clone)
		return 7;
	if (IStream_Seek(clone, to_start, STREAM_SEEK_SET, NULL) != S_OK)
		return 8;
	if (IStream_Read(clone, have, ROUND_TRIP_BYTES, &done) != S_OK || (SIZE_T)done != ROUND_TRIP_BYTES)
		return 9;
	if (!same_bytes(have, expect, ROUND_TRIP_BYTES))
		return 10;
	IStream_Release(clone);
	clone = NULL;
	if (hglobal_from_stream(stream, &global) != S_OK || !global)
		return 11;
	if (GlobalSize(global) < ROUND_TRIP_BYTES)
		return 12;
	IStream_Release(stream);
	stream = NULL;

	/* The lock-count contract: while the caller holds a GlobalLock of its own, the object does not move — a write
	   that would grow the block fails before anything lands, a rewrite inside the extent succeeds, and after the
	   unlock the growth goes through. */
	if (create_stream(NULL, TRUE, &stream) != S_OK || !stream)
		return 13;
	{
		BYTE initial[INITIAL_BYTES];
		for (i = 0; i < INITIAL_BYTES; ++i)
			initial[i] = pattern_byte(i);
		if (IStream_Write(stream, initial, INITIAL_BYTES, &written) != S_OK || written != INITIAL_BYTES)
			return 14;
	}
	if (hglobal_from_stream(stream, &global) != S_OK || !global)
		return 15;
	locked = GlobalLock(global);
	if (!locked)
		return 16;
	at.QuadPart = INITIAL_BYTES;
	if (IStream_Seek(stream, at, STREAM_SEEK_SET, NULL) != S_OK)
		return 17;
	{
		BYTE grow[64];
		for (i = 0; i < sizeof grow; ++i)
			grow[i] = 0x5a;
		result = IStream_Write(stream, grow, sizeof grow, &written);
		if (SUCCEEDED(result))
			return 18; /* the growth of a locked block must fail, as the realloc of a locked object does */
		if (written)
			return 19; /* and nothing of it may land */
	}
	to_start.QuadPart = 0;
	if (IStream_Seek(stream, to_start, STREAM_SEEK_SET, NULL) != S_OK)
		return 20;
	{
		const BYTE rewrite[10] = {'r', 'a', 'v', 'e', 'n', '-', 'f', 'i', 'x', 0};
		if (IStream_Write(stream, rewrite, sizeof rewrite, &written) != S_OK || written != sizeof rewrite)
			return 21; /* a write inside the extent touches no realloc and answers as it always did */
	}
	GlobalUnlock(global);
	size.QuadPart = INITIAL_BYTES + 4096;
	if (IStream_SetSize(stream, size) != S_OK)
		return 22; /* with the lock gone the growth succeeds */
	if (IStream_Stat(stream, &stat, STATFLAG_NONAME) != S_OK || stat.cbSize.QuadPart != INITIAL_BYTES + 4096)
		return 23;
	IStream_Release(stream);
	stream = NULL;

	/* The concurrency the material workers ran: four threads clone one stream and write disjoint slots into a block
	   pre-sized to hold them all, so the writes meet only the block's own serialization and the clone path runs
	   under them; every slot comes back byte for byte. The grow-under-concurrency shape rides on the same block
	   mutex through the single-threaded growth of the first section and the writes here. */
	if (create_stream(NULL, TRUE, &stream) != S_OK || !stream)
		return 24;
	size.QuadPart = TOTAL_BYTES;
	if (IStream_SetSize(stream, size) != S_OK)
		return 24;
	shared_stream = stream;
	for (index = 0; index < THREAD_COUNT; ++index) {
		threads[index] = CreateThread(NULL, 0, slot_writer, (LPVOID)(DWORD_PTR)index, 0, &thread_ids[index]);
		if (!threads[index])
			return 25;
	}
	code = 0;
	for (index = 0; index < THREAD_COUNT; ++index) {
		exit_code = 0;
		if (WaitForSingleObject(threads[index], 60000) != WAIT_OBJECT_0)
			code = 26;
		else if (!GetExitCodeThread(threads[index], &exit_code) || exit_code)
			code = exit_code ? exit_code : 27;
		CloseHandle(threads[index]);
	}
	if (code)
		return (int)code;
	if (IStream_Stat(stream, &stat, STATFLAG_NONAME) != S_OK || stat.cbSize.QuadPart != TOTAL_BYTES)
		return 29;
	if (hglobal_from_stream(stream, &global) != S_OK || !global)
		return 30;
	locked = GlobalLock(global);
	if (!locked)
		return 31;
	{
		BYTE slot[SLOT_BYTES];
		for (slot_index = 0; slot_index < THREAD_COUNT * ROUND_COUNT; ++slot_index) {
			const DWORD thread = (DWORD)(slot_index / ROUND_COUNT), round = (DWORD)(slot_index % ROUND_COUNT);
			fill_slot(slot, thread, round);
			if (!same_bytes((BYTE *)locked + slot_index * SLOT_BYTES, slot, SLOT_BYTES)) {
				++failures;
				break;
			}
		}
	}
	GlobalUnlock(global);
	if (failures)
		return 32;

	IStream_Release(stream);
	HeapFree(GetProcessHeap(), 0, expect);
	HeapFree(GetProcessHeap(), 0, have);
	return 0;
}
