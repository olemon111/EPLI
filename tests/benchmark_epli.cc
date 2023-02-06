#include "../src/epli.h"
#include "util/nvm/nvm_alloc.h"

// #define REST true
#define REST false

using namespace epltree;
using namespace std;

// size_t LOAD_SIZE = 400000000;
size_t LOAD_SIZE = 200000000;
// size_t LOAD_SIZE = 2000000;

int main(int argc, char *argv[])
{
    /* init nvm */
    NVM::env_init();
    NVM::data_init();
    /* init index tree */
    EPLI *epli = new EPLI();
    epli->Init();
    /* generate workload */
    vector<key_type> keys;
    keys.resize(LOAD_SIZE);
    iota(keys.begin(), keys.end(), 1);
    // reverse(keys.begin(), keys.end());
    kv_type *bulk_kvs = new kv_type[LOAD_SIZE];
    for (size_t i = 1; i <= LOAD_SIZE; i++)
    {
        bulk_kvs[i - 1].first = i;
        bulk_kvs[i - 1].second = i + 1;
    }
    // sort(bulk_kvs, bulk_kvs + LOAD_SIZE, [](auto const &a, auto const &b)
    //      { return a.first < b.first; });

    /* Test BulkLoad */
    epli->BulkLoad(bulk_kvs, LOAD_SIZE);

    // /* Test Put */
    // for (auto key : keys)
    // {
    //     // cout << "EPL-Tree PUT " << key << "-------------------" << endl;
    //     tree->Insert(key, key + 1);
    // }
    cout << "EPLI PUT end" << endl;

    /* Test Get */
    size_t wrong_get = 0;
    for (auto key : keys)
    {
        // cout << "EPL-Tree GET " << key << "-------------------" << endl;
        val_type value;
        epli->Get(key, value);
        // cout << "key: " << key << ", val: " << value << endl;
        assert(value == key + 1);
        if (value != key + 1)
        {
            cout << "wrong kv " << key << ", " << value << endl;
            // break;
            wrong_get++;
        }
    }
    // Test Delete
    cout << "wrong get:" << wrong_get << endl;

    return 0;
}
