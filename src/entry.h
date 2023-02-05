#pragma once
#include "util/nvm/nvm_alloc.h"

typedef uint64_t key_type;
typedef uint64_t val_type;
typedef std::pair<key_type, val_type> kv_type;

using namespace std;

namespace epltree
{
    // each Entry occupies less than 256B of NVM
    static const key_type INVALID_KEY = 0;
    static const int KVS_PER_ENTRY = 14;

    typedef int status;
    const static int STATUS_OK = 0;
    const static int STATUS_FAIL = -1;
    const static int STATUS_OVERFLOW = 1;

    kv_type tmp_kvs[KVS_PER_ENTRY]; // 16B (cache line size)

    struct Entry
    {
        /* total 248B <= 256B */
        Entry *next;                // 8B
        key_type min_key;           // 8B
        uint8_t flag;               // 1B
        kv_type kvs[KVS_PER_ENTRY]; // 16B * KVS_PER_ENTRY(14)
        char reserve[8];

        /* constructor */
        Entry(key_type init_min_key)
        {
            // cout << "init entry" << endl;
            memset(this, 0, sizeof(Entry));
            min_key = init_min_key;
        }

        Entry(key_type init_min_key, const kv_type init_kvs[], size_t size)
        {
            // cout << "init entry , min_key: " << init_min_key << ", size: " << size << endl;
            memset(this, 0, sizeof(Entry));
            min_key = init_min_key;
            for (size_t i = 0; i < size; i++)
            {
                setVal(i, init_kvs[i].second);
                setKey(i, init_kvs[i].first);
            }
        }

        /* inner setter and getter */
        void setKey(int i, key_type key)
        {
            kvs[i].first = key;
        }

        void setVal(int i, val_type val)
        {
            kvs[i].second = val;
        }

        void setOrderedKVs(const kv_type new_kvs[], int size)
        {
            memset(kvs, 0, sizeof(kvs));
            for (int i = 0; i < size; i++)
            {
                setVal(i, new_kvs[i].second);
                setKey(i, new_kvs[i].first);
            }
            min_key = new_kvs[0].first;
        }

        void setPersistFlag(int v)
        {
            flag = v;
            NVM::Mem_persist(&flag, sizeof(flag));
        }

        const key_type GetMinKey()
        {
            return min_key;
        }

        /* public methods */
        status Get(key_type key, val_type &val)
        {
            for (int i = 0; i < KVS_PER_ENTRY; i++)
            {
                if (kvs[i].first == key)
                {
                    val = kvs[i].second;
                    return STATUS_OK;
                }
            }
            return STATUS_FAIL;
        }

        // insert or update if there's empty place
        status MaybePut(key_type key, val_type val)
        {
            // cout << "maybe put " << key << ", " << val << endl;
            int empty_pos = -1;
            for (int i = 0; i < KVS_PER_ENTRY; i++)
            {
                if (key == kvs[i].first) // update value
                {
                    setVal(i, val);
                    NVM::Mem_persist(&kvs[i], sizeof(kvs[i]));
                    return STATUS_OK;
                }
                if (kvs[i].first == INVALID_KEY && empty_pos == -1) // first empty postion
                {
                    empty_pos = i;
                }
            }
            // no place for a new key
            if (empty_pos == -1)
            {
                return STATUS_OVERFLOW;
            }
            // write value before key
            setVal(empty_pos, val);
            setKey(empty_pos, key);
            NVM::Mem_persist(&kvs[empty_pos], sizeof(kv_type));
            return STATUS_OK;
        }

        void PrintKVs()
        {
            cout << "------------------------" << endl;
            cout << "print kvs, min key: " << min_key << endl;
            for (int i = 0; i < KVS_PER_ENTRY; i++)
            {
                cout << kvs[i].first << ", " << kvs[i].second << endl;
            }
            cout << "------------------------" << endl;
        }
    };
}