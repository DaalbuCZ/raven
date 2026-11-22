#include "guest_allocator.h"

namespace raven {

namespace {

constexpr uint64_t chunk_size = 0x10000, largest_shared = 4096;

// The size class of a small block: 0 holds 16 bytes, each next class twice as many.
size_t size_class(uint64_t size)
{
	size_t index = 0;
	for (uint64_t block = 16; block < size; block <<= 1)
		++index;
	return index;
}

}

// Everything below that touches the bookkeeping maps runs under the guard; the internal helpers assume it is held.
uint64_t GuestAllocator::allocate(uint64_t size, bool zero)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	return allocate_impl(size, zero);
}

uint64_t GuestAllocator::allocate_impl(uint64_t size, bool zero)
{
	if (heap.available()) {
		const auto address = heap.allocate(zero ? 8 : 0, std::max<uint64_t>(size, 1));
		// The program may have freed an earlier block at this address with HeapFree, which this allocator never sees: what it still
		// remembers of that block (an empty request, a movable handle) belongs to nothing now and must not apply to the new one.
		forget(address);
		return address;
	}
	if (size <= largest_shared) {
		const size_t index = size_class(std::max<uint64_t>(size, 1));
		const uint64_t block = uint64_t(16) << index;
		uint64_t address = 0;
		if (!free_blocks[index].empty()) {
			address = free_blocks[index].back();
			free_blocks[index].pop_back();
			if (zero)
				memory.clear(address, block);
		} else {
			auto &[cursor, end] = cutting[index];
			// A new chunk's pages read as zero.
			if (cursor + block > end) {
				const auto range = memory.allocate(0, chunk_size, 0x3000, 4);
				if (range.status)
					return 0;
				cursor = range.address;
				end = range.address + chunk_size;
			}
			address = cursor;
			cursor += block;
		}
		shared.emplace(address, std::max<uint64_t>(size, 1));
		return address;
	}
	const uint64_t rounded = (std::max<uint64_t>(size, 1) + 4095) & ~uint64_t(4095);
	const auto range = memory.allocate(0, rounded, 0x3000, 4);
	if (range.status)
		return 0;
	pages.emplace(range.address, std::max<uint64_t>(size, 1));
	return range.address;
}

void GuestAllocator::forget(uint64_t address)
{
		std::fprintf(stderr, "raven-alloc: forget branch2 handle %llx\n", (unsigned long long)address);
		std::fprintf(stderr, "raven-alloc: forget branch1 block %llx\n", (unsigned long long)address);
	empty.erase(address);
	if (const auto moved = moved_blocks.find(address); moved != moved_blocks.end()) {
		movable_blocks.erase(moved->second);
		moved_blocks.erase(moved);
	}
	if (const auto handle = movable_blocks.find(address); handle != movable_blocks.end()) {
		moved_blocks.erase(handle->second);
		movable_blocks.erase(handle);
	}
}

uint64_t GuestAllocator::allocate_movable(uint64_t size, bool zero)
{
	// The allocation goes through the guarded public path (its heap branch forgets the address's stale record), the
	// movable registration under the guard after it: a live block cannot be handed out again, so no other thread can
	// reach this address's bookkeeping in between.
	const auto address = allocate(size, zero);
	if (!address)
		return 0;
	std::lock_guard<std::recursive_mutex> lock(guard);
	empty.erase(address);
	moved_blocks.erase(address);
	movable_blocks[address] = address;
	if (!size)
		empty.insert(address);
	return address;
}

void GuestAllocator::make_movable(uint64_t address)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	movable_blocks.emplace(address, address);
}

uint64_t GuestAllocator::data(uint64_t handle) const
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	const auto found = movable_blocks.find(handle);
	return found == movable_blocks.end() ? handle : found->second;
}

uint64_t GuestAllocator::handle(uint64_t block) const
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	const auto found = moved_blocks.find(block);
	return found == moved_blocks.end() ? block : found->second;
}

void GuestAllocator::move(uint64_t handle, uint64_t block)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	auto &current = movable_blocks.at(handle);
	if (current != handle) {
		moved_blocks.erase(current);
		release_block(current);
	}
	current = block;
	moved_blocks[block] = handle;
}

bool GuestAllocator::release(uint64_t address)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	const auto found = moved_blocks.find(address);
	const auto block = found == moved_blocks.end() ? address : found->second;
	if (const auto handle = movable_blocks.find(block); handle != movable_blocks.end()) {
		lock_counts.erase(block);
		if (handle->second != block) {
			moved_blocks.erase(handle->second);
			release_block(handle->second);
		}
		movable_blocks.erase(handle);
	}
	return release_block(block);
}

bool GuestAllocator::release_block(uint64_t address)
{
	empty.erase(address);
	if (heap.available())
		return heap.release(address);
	if (const auto block = shared.find(address); block != shared.end()) {
		free_blocks[size_class(block->second)].push_back(address);
		shared.erase(block);
		return true;
	}
	const auto found = pages.find(address);
	if (found == pages.end())
		return false;
	pages.erase(found);
	return memory.free(address, 0, 0x8000).status == 0;
}

uint64_t GuestAllocator::size_of(uint64_t address)
{
	if (empty.contains(address))
		return 0;
	if (heap.available())
		return heap.block_size(address);
	if (const auto block = shared.find(address); block != shared.end())
		return block->second;
	const auto found = pages.find(address);
	return found == pages.end() ? UINT64_MAX : found->second;
}

uint64_t GuestAllocator::size(uint64_t address)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	return size_of(address);
}

bool GuestAllocator::owns(uint64_t address)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	return heap.available() ? heap.block_size(address) != UINT64_MAX : pages.contains(address) || shared.contains(address);
}

bool GuestAllocator::movable(uint64_t handle) const
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	return movable_blocks.contains(handle);
}

void GuestAllocator::mark_empty(uint64_t block)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	empty.insert(block);
}

uint64_t GuestAllocator::acquire(uint64_t handle, bool *discarded)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	*discarded = false;
	if (!movable_blocks.contains(handle))
		// A fixed block: the address is the handle, no count and nothing to wait for — but it must be a block here.
		return size_of(handle) == UINT64_MAX ? UINT64_MAX : handle;
	const auto block = movable_blocks.at(handle);
	const auto size = size_of(block);
	if (size == UINT64_MAX)
		return UINT64_MAX;
	if (!size) {
		// A movable block without bytes is what Windows calls discarded: it has a handle, and nothing to lock.
		*discarded = true;
		return 0;
	}
	++lock_counts[handle];
	return block;
}

bool GuestAllocator::unlock_movable(uint64_t handle, bool *was_locked, bool *still)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	*was_locked = true;
	*still = true;
	if (!movable_blocks.contains(handle))
		return true; // a fixed block: nothing was counted, GlobalUnlock answers nonzero
	const auto count = lock_counts.find(handle);
	if (count == lock_counts.end() || !count->second) {
		*was_locked = false;
		return false;
	}
	if (--count->second)
		return true;
	lock_counts.erase(count);
	*still = false;
	return false;
}

uint64_t GuestAllocator::reallocate_movable(uint64_t handle, uint64_t size, bool zero, bool *locked,
	const std::function<void (uint64_t, uint64_t, uint64_t)> &copy)
{
	std::lock_guard<std::recursive_mutex> lock(guard);
	if (const auto count = lock_counts.find(handle); count != lock_counts.end() && count->second) {
		// A locked object does not move: the resize fails, as Windows reallocates nothing of a locked block.
		*locked = true;
		return 0;
	}
	// The whole move runs under the guard the handle's other calls wait on: the new block allocated, the old bytes
	// carried over, the handle re-pointed. A lock that lands during the move resolves the new pointer whole; the
	// handle's first block stays behind as the anchor, the way move() keeps it — freeing it would let a later
	// allocation land on the handle's address and forget the live registration.
	const uint64_t block = movable_blocks.at(handle);
	const uint64_t old = size_of(block);
	const uint64_t address = allocate_impl(size, zero);
	if (!address)
		return 0;
	if (const uint64_t kept = std::min<uint64_t>(old, size))
		copy(block, address, kept);
	if (!size)
		empty.insert(address);
	if (block != handle) {
		moved_blocks.erase(block);
		release_block(block);
	}
	movable_blocks[handle] = address;
	moved_blocks[address] = handle;
	*locked = false;
	return handle;
}

}
