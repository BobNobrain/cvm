#include <stdio.h>
#include <string.h>

#define UTIL_ARENA_IMPL
#include "util_int.h"

void *malloc_or_die(size_t size) {
    void *ptr = malloc(size);
    if (ptr == 0) {
        fprintf(stderr, "Dead due to malloc failure\n");
        abort();
    }
    return ptr;
}

void *realloc_or_die(void *old, size_t size) {
    void *ptr = realloc(old, size);
    if (ptr == 0) {
        fprintf(stderr, "Dead due to realloc failure\n");
        abort();
    }
    return ptr;
}

typedef struct ArenaRegion {
    void *start;
    size_t pos;
    size_t size;
} ArenaRegion;

struct Arena {
    ArenaRegion *regions;
    size_t region_size;
    size_t n_regions;
    size_t capacity;
    size_t max_regions;
};

const size_t _util_arena_initial_regions_capacity = 32;

#define DEBUG_ARENA 0

#if DEBUG_ARENA
#define IFDEBUG(S) S
#else
#define IFDEBUG(S)
#endif

size_t _util_arena_add_padding(size_t bytes) {
    if (bytes % sizeof(max_align_t) == 0) { return bytes; }
    size_t padding = sizeof(max_align_t) - (bytes % sizeof(max_align_t));
    return bytes + padding;
}

Arena* arena_new(size_t region_size) {
    Arena *arena = malloc_or_die(sizeof(Arena));
    arena->region_size = _util_arena_add_padding(region_size);
    arena->n_regions = 0;

    arena->capacity = _util_arena_initial_regions_capacity;
    arena->regions = malloc_or_die(arena->capacity * sizeof(ArenaRegion));

    arena->max_regions = 0;

    IFDEBUG( printf("allocated new arena(%zu) <%p>\n", region_size, arena); )

    return arena;
}

void arena_destroy(Arena *arena) {
    IFDEBUG( printf("destroying arena <%p>\n", arena); )

    if (arena == 0) { return; }

    IFDEBUG( printf("  arena <%p>.n_regions = %zu\n", arena, arena->n_regions); )
    for (size_t i = 0; i < arena->n_regions; i++) {
        ArenaRegion *region = &arena->regions[i];
        IFDEBUG( printf("  arena <%p>.regions[%zu]: %zu / %zu allocated\n", arena, i, region->pos, region->size); )
        free(region->start);
    }

    free(arena->regions);
    free(arena);
}

ArenaRegion* _util_arena_new_region(Arena *arena) {
    if (arena->max_regions > 0 && arena->n_regions >= arena->max_regions) {
        die("arena exceeded max regions allowed");
    }

    if (arena->n_regions >= arena->capacity) {
        arena->capacity += _util_arena_initial_regions_capacity;
        arena->regions = realloc_or_die(arena->regions, arena->capacity * sizeof(ArenaRegion));
    }

    ArenaRegion new_region = { .pos = 0, .size = arena->region_size, .start = 0 };
    new_region.start = malloc_or_die(new_region.size);
    arena->regions[arena->n_regions] = new_region;

    IFDEBUG( printf("allocated new arena region #%zu <%p> for <%p>\n", arena->n_regions, new_region.start, arena); )

    arena->n_regions += 1;
    return &arena->regions[arena->n_regions - 1];
}

ArenaRegion* _util_arena_find_free_region(Arena *arena, size_t bytes) {
    ArenaRegion *target_region = 0;

    for (size_t i = 0; i < arena->n_regions; i++) {
        ArenaRegion *region = &arena->regions[i];
        if (region->size >= region->pos + bytes) {
            target_region = region;
            break;
        }
    }

    return target_region;
}

void* arena_alloc(Arena *arena, size_t bytes) {
    IFDEBUG( printf("trying to allocate %zu bytes in arena <%p>\n", bytes, arena); )

    if (arena == 0) { return malloc_or_die(bytes); }

    if (bytes > arena->region_size) {
        die("cannot allocate in arena: requested size is bigger than arena region size");
    }

    ArenaRegion *target_region = _util_arena_find_free_region(arena, bytes);
    if (target_region == 0) {
        target_region = _util_arena_new_region(arena);
    }

    size_t bytes_with_padding = _util_arena_add_padding(bytes);
    void *result = target_region->start + target_region->pos;
    target_region->pos += bytes_with_padding;

    IFDEBUG( printf("allocated <%p> in arena <%p>, padded to %zu\n", result, arena, bytes_with_padding); )

    return result;
}

void* arena_realloc(Arena *arena, void *ptr, size_t old_size, size_t new_size) {
    IFDEBUG( printf("trying to reallocate %zu->%zu bytes for <%p> in arena <%p>\n", old_size, new_size, ptr, arena); )

    if (arena == 0) { return realloc_or_die(ptr, new_size); }

    uintptr_t target = (uintptr_t) ptr;
    ArenaRegion *src_region = 0;

    for (size_t i = 0; i < arena->n_regions; i++) {
        ArenaRegion *region = &arena->regions[i];
        uintptr_t rstart = (uintptr_t) region->start;
        uintptr_t rend = (uintptr_t) region->start + region->size;

        if (target >= rstart && target < rend) {
            src_region = region;
            break;
        }
    }

    if (src_region == 0) {
        die("cannot reallocate in this arena: the pointer does not belong to any of its regions");
    }

    if (old_size == new_size) {
        return ptr;
    }

    uintptr_t rstart = (uintptr_t) src_region->start;
    bool is_last_in_region = (target - rstart) + _util_arena_add_padding(old_size) == src_region->pos;

    if (new_size < old_size) {
        // we just need to contract the memory allocated
        if (is_last_in_region) {
            src_region->pos = src_region->pos - old_size + new_size;
        }

        return ptr;
    }

    if (is_last_in_region && src_region->pos - old_size + new_size <= src_region->size) {
        // there's enough room for expansion
        src_region->pos = _util_arena_add_padding(src_region->pos - old_size + new_size);
        return ptr;
    }

    void *new_allocation = arena_alloc(arena, new_size);
    memcpy(new_allocation, ptr, old_size);
    IFDEBUG( printf("moved data <%p>-><%p> (%zu bytes)\n", ptr, new_allocation, old_size); )
    return new_allocation;
}

Arena* arena_global() {
    return 0;
}

Arena* arena_TODO() {
    return 0;
}

void arena_set_max_regions(Arena *arena, size_t max_regions) {
    arena->max_regions = max_regions;
}
