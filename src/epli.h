#pragma once

#include "epltree.h"
#include "swtable.h"

using namespace std;
using namespace epltree;

#define SAMPLE_M 20 // TODO:
#define MIN_HIT_RATE 0.001

class EPLI
{
public:
    EPLI() {}
    ~EPLI() {}

    void Init()
    {
        tree = new EPLTree();
        table = new SWTable();
        table->Init();
        table_on = true;
        hit_cnt = 0;
        tot_get = 0;
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
        /* close SWTable when hit rate is too low */
        if (hit_cnt > 0 && hit_cnt < MIN_HIT_RATE * tot_get)
        {
            table_on = false;
            cout << "Close SWTable with hit rate: " << hit_cnt / (double)tot_get << ", hit_cnt: " << hit_cnt << ", tot_get:" << tot_get << endl;
        }
    }

    void Info()
    {
        cout << "hit cnt: " << hit_cnt << endl;
        // table->Print();
    }

private:
    EPLTree *tree;
    SWTable *table;
    bool table_on; // open SWTable for tilt workload
    double hit_cnt;
    size_t tot_get;
};