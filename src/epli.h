#pragma once

#include "epltree.h"
#include "swtable.h"

using namespace std;
using namespace epltree;

class EPLI
{
    const static int sample_m = 20; // TODO:

public:
    EPLI() {}
    ~EPLI() {}

    void Init()
    {
        tree = new EPLTree();
        table = new SWTable();
        table->Init();
        table_on = true; // TODO:
    }

    void BulkLoad(const kv_type bulk_kvs[], size_t num_keys)
    {
        tree->BulkLoad(bulk_kvs, num_keys);
    }

    void Put(key_type key, val_type val)
    {
        tree->Insert(key, val);
    }

    void Get(key_type key, val_type &val)
    {
        if (!table_on) // SWTable closed
        {
            tree->Get(key, val);
            return;
        }
        /* search in SW-Table */
        val = table->Get(key);
        if (!val)
        {
            tree->Get(key, val); // search in EPL-Tree
            if (sample_cur % sample_m == 0)
            {
                table->Put(key, val); // insert into hashtable for every sample_m get operation
            }
        }
        sample_cur = (sample_cur + 1) % sample_m;
    }

    void Info()
    {
        table->Print();
    }

private:
    EPLTree *tree;
    SWTable *table;
    bool table_on; // open SWTable for tilt workload
    int sample_cur;
};