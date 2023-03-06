#pragma once

#include "epltree.h"
#include "swtable.h"

using namespace std;
using namespace epltree;

#define SAMPLE_M 20 // TODO:
#define MIN_HIT_RATE 0.01
// #define MIN_HIT_RATE 0.001
// #define SWTABLE_DEFAULT_OPEN

class EPLI
{
public:
    EPLI() {}
    ~EPLI() {}

    void Init(bool recover = false)
    {
        tree = new EPLTree(recover);
        table = new SWTable();
        table->Init();
#ifdef SWTABLE_DEFAULT_OPEN
        table_on = true;
#endif
        hit_cnt = 0;
        tot_get = 0;
    }

    void Recover(size_t load_size)
    {
        tree->Recover(load_size);
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
#ifdef SWTABLE_DEFAULT_OPEN
        if (!table_on) // SWTable closed
        {
            tree->Get(key, val);
            return;
        }
#endif
        /* search in SW-Table */
        val = table->Get(key);
        if (val)
        {
            hit_cnt++;
        }
        else
        {
            tree->Get(key, val); // search in EPL-Tree
            if (tot_get % SAMPLE_M == 0)
            {
                table->Put(key, val); // insert into hashtable for every SAMPLE_M get operation
            }
        }
        tot_get++;
#ifdef SWTABLE_DEFAULT_OPEN
        /* close SWTable when hit rate is too low */
        if (hit_cnt > 0 && hit_cnt < MIN_HIT_RATE * tot_get)
        {
            table_on = false;
            cout << "Close SWTable with hit rate: " << hit_cnt / (double)tot_get << ", hit_cnt: " << hit_cnt << ", tot_get:" << tot_get << endl;
        }
#endif
    }

    void Reset() // open swtable
    {
        table_on = true;
        tot_get = 0;
        hit_cnt = 0;
    }

    void Info()
    {
        cout << "hit cnt: " << hit_cnt << ", total get cnt: " << tot_get << endl;
        // table->Print();
        double index_sz = tree->get_size();
        double table_sz = table->get_size();
        cout << "EPLI total dram size: " << (index_sz + table_sz) / 1024.0 / 1024.0 << " MB, alex size: " << index_sz / 1024.0 / 1024.0 << ", table size: " << table_sz / 1024.0 / 1024.0 << endl;
    }

private:
    EPLTree *tree;
    SWTable *table;
    bool table_on; // open SWTable for tilt workload
    double hit_cnt;
    size_t tot_get;
};