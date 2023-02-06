#pragma once

#include "clht/clht_lb_res.h"
#include <unistd.h>
#include <sys/syscall.h>

class SWTable
{
    size_t num_buckets = 512; // TODO:
    size_t num_threads = 1;

public:
    SWTable() {}

    ~SWTable()
    {
        clht_gc_destroy(hashtable);
    }

    void Init()
    {
        hashtable = clht_create(num_buckets);
        assert(hashtable != NULL);
        uint32_t ID = syscall(SYS_getgid);
        clht_gc_thread_init(hashtable, ID);
        ssmem_allocator_t *alloc = (ssmem_allocator_t *)malloc(sizeof(ssmem_allocator_t));
        assert(alloc != NULL);
        ssmem_alloc_init_fs_size(alloc, SSMEM_DEFAULT_MEM_SIZE, SSMEM_GC_FREE_SET_SIZE, ID);
    }

    void Put(key_type key, val_type val)
    {
        clht_put(hashtable, key, val);
    }

    void Update(key_type key, val_type val) // FIXME:
    {
        clht_remove(hashtable, key);
        clht_put(hashtable, key, val);
    }

    val_type Get(key_type key)
    {
        lt++;
        return clht_get(hashtable->ht, key);
    }

    val_type Remove(key_type key)
    {
        return clht_remove(hashtable, key);
    }

    void Print()
    {
        clht_print(hashtable->ht);
    }

private:
    clht_t *hashtable;
    size_t lt;
};