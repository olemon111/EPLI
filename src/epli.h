#pragma once

#include "epltree.h"
#include "swtable.h"

using namespace std;
using namespace epltree;

#define SAMPLE_M 20 // change in dynamic workload test
// #define MIN_HIT_RATE 0.001
#define MIN_HIT_RATE 0.002
// #define MIN_HIT_RATE 0.1
// #define USE_SWTABLE // comment this line to disable SWTable
#ifdef USE_SWTABLE
// #define SWTABLE_AUTO_CLOSE // comment this line to close SWTable when hit rate is too low
#endif

class EPLI
{
public:
    EPLI() {}
    ~EPLI() {}

    void Init(bool recover = false)
    {
        tree = new EPLTree(recover);
#ifdef USE_SWTABLE
        table = new SWTable();
        table->Init();
        table_on = true;
        hit_cnt = 0;
        tot_get = 0;
#endif
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

    void Update(key_type key, val_type val)
    {
#ifdef USE_SWTABLE
        table->Update(key, val);
#endif
        tree->Update(key, val);
    }

    void Get(key_type key, val_type &val)
    {
#ifdef USE_SWTABLE
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
#ifdef SWTABLE_AUTO_CLOSE
        /* close SWTable when hit rate is too low */
        if (hit_cnt > 0 && hit_cnt < MIN_HIT_RATE * tot_get)
        {
            table_on = false;
            cout << "Close SWTable with hit rate: " << hit_cnt / (double)tot_get << ", hit_cnt: " << hit_cnt << ", tot_get:" << tot_get << endl;
        }
#endif
#else
        tree->Get(key, val);
#endif
    }

    int RangeScan(uint64_t start_key, uint64_t end_key, std::vector<std::pair<uint64_t, uint64_t>> &results)
    {
        return tree->RangeScan(start_key, end_key, results);
    }

    void Remove(key_type key)
    {
#ifdef USE_SWTABLE
        table->Remove(key);
#endif
        tree->Remove(key);
    }

    void Reset() // open swtable
    {
#ifdef USE_SWTABLE
        cout << "reset swtable" << endl;
        table_on = true;
        tot_get = 0;
        hit_cnt = 0;
#endif
    }

    void Info()
    {
        double index_sz = tree->get_size();
#ifdef USE_SWTABLE
        cout << "hit cnt: " << hit_cnt << ", total get cnt: " << tot_get << endl;
        double table_sz = table->get_size();
        // table->Print();
        cout << "EPLI total dram size: " << (index_sz + table_sz) / 1024.0 / 1024.0 << " MB, alex size: " << index_sz / 1024.0 / 1024.0 << ", table size: " << table_sz / 1024.0 / 1024.0 << endl;
#else
        cout << "EPLI total dram size: " << index_sz / 1024.0 / 1024.0 << " MB, alex size: " << index_sz / 1024.0 / 1024.0 << endl;
#endif
    }

private:
    EPLTree *tree;
#ifdef USE_SWTABLE
    SWTable *table;
    bool table_on; // open SWTable for tilt workload
    double hit_cnt;
    size_t tot_get;
#endif
};