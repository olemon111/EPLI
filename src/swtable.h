#pragma once

#include "clht/clht_lb_res.h"
#include <unistd.h>
#include <sys/syscall.h>

class SWTable
{
    // const static size_t num_buckets = 256; // TODO:
    // const static size_t num_buckets = 1024;
    // const static size_t num_buckets = 4096;
    const static size_t num_buckets = 16384;
    // const static size_t num_buckets = 65536;
    // const static size_t num_buckets = 262144;
    // const static size_t num_buckets = 1048576;
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
        clht_put(hashtable, key, val, lt);
    }

    void Update(key_type key, val_type val)
    {
        clht_update(hashtable, key, val, lt);
    }

    val_type Get(key_type key)
    {
        lt++;
        return clht_get(hashtable->ht, key, lt);
    }

    val_type Remove(key_type key)
    {
        return clht_remove(hashtable, key);
    }

    void Print()
    {
        clht_print(hashtable->ht);
    }

    double get_size()
    {
        return (double)clht_size_mem(hashtable->ht);
    }

private:
    clht_t *hashtable;
    uint32_t lt; // logical time
};