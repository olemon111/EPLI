#pragma once

#include <unistd.h>
#include <atomic>
#include "epltree_config.h"
#include "util/nvm/common_time.h"
#include "util/nvm/statistic.h"
#include "util/nvm/nvm_alloc.h"
#include <pthread.h>

#include "datalist.h"
#include "alex/alex.h"

namespace epltree
{
    typedef alex::Alex<key_type, Entry *> index_type;

    class EPLTree
    {
    public:
        EPLTree()
        {
            // init NVM data
            datalist = new DataList();
            datalist->Init();
            // init index
            index = new index_type();
            index->insert(INVALID_KEY, datalist->Head());
        }

        ~EPLTree()
        {
            delete datalist;
            delete index;
        }

        void BulkLoad(const kv_type bulk_kvs[], size_t num_keys)
        {
            cout << "bulk load " << num_keys << " kvs" << endl;
            if (!num_keys)
            {
                return;
            }
            // load head entry
            datalist->Head()->setOrderedKVs(bulk_kvs, min(num_keys, size_t(KVS_PER_ENTRY)));
            NVM::Mem_persist(datalist->Head(), sizeof(Entry));
            index->erase(INVALID_KEY);
            index->insert(datalist->Head()->GetMinKey(), datalist->Head());
            if (num_keys <= KVS_PER_ENTRY)
            {
                return;
            }
            Entry *cur = datalist->Head();
            for (size_t i = 1; KVS_PER_ENTRY * i < num_keys; i++)
            {

                Entry *new_entry = (Entry *)NVM::data_alloc->alloc_aligned(sizeof(Entry));
                key_type new_min_key = bulk_kvs[i * KVS_PER_ENTRY].first;
                new (new_entry) Entry(new_min_key, bulk_kvs + i * KVS_PER_ENTRY, min(num_keys - i * KVS_PER_ENTRY, size_t(KVS_PER_ENTRY)));
                cur->next = new_entry;
                // NVM::Mem_persist(cur, sizeof(Entry));
                cur = cur->next;
                index->insert(new_min_key, new_entry);
            }
            // NVM::Mem_persist(cur, sizeof(Entry));
            datalist->Persist();
            // FIXME: how to persist
        }

        int Get(key_type key, val_type &val)
        {
            Entry *entry = *(index->get_payload_last_no_greater_than(key));
            assert(key >= entry->GetMinKey());
            // cout << "get key: " << key << ", min key: " << entry->GetMinKey() << endl;
            if (entry->Get(key, val) != STATUS_OK) // key not exist
            {
                cout << "key: " << key << " not exist, min key: " << entry->GetMinKey() << endl;
                val = 0;
                return STATUS_FAIL;
            }
            return STATUS_OK;
        }

        int Insert(key_type key, val_type val)
        {
            Entry *entry = *(index->get_payload_last_no_greater_than(key));
            assert(key >= entry->GetMinKey());
            // cout << "min key: " << entry->GetMinKey() << endl;
            int ret = entry->MaybePut(key, val);
            if (ret == STATUS_OVERFLOW) // need to split
            {
                splitEntry(entry);
                return Insert(key, val); // insert again
            }
            return ret; // OK/FAIL
        }

        int Update(key_type key, val_type val)
        {
        }

        int Delete(key_type key)
        {
        }

        int Scan(key_type start_key, int size, std::vector<kv_type> &results)
        {
        }

    private:
        void splitEntry(Entry *entry)
        {
            // cout << "split entry" << endl;
            // copy kvs and sort
            memcpy(tmp_kvs, entry->kvs, sizeof(entry->kvs));
            sort(tmp_kvs, tmp_kvs + KVS_PER_ENTRY, [](auto const &a, auto const &b)
                 { return a.first < b.first; });
            // assign new entry
            Entry *new_entry = (Entry *)NVM::data_alloc->alloc_aligned(sizeof(Entry));
            key_type new_min_key = tmp_kvs[KVS_PER_ENTRY / 2].first;
            new (new_entry) Entry(new_min_key, tmp_kvs + KVS_PER_ENTRY / 2, (KVS_PER_ENTRY + 1) / 2);
            // mark split
            entry->setPersistFlag(1);
            // add new entry to link list
            new_entry->next = entry->next;
            NVM::Mem_persist(new_entry, sizeof(Entry));
            //  update prev entry
            entry->next = new_entry;
            index->insert(new_min_key, new_entry);
            entry->setOrderedKVs(tmp_kvs, KVS_PER_ENTRY / 2);
            // end split
            entry->setPersistFlag(0);
            NVM::Mem_persist(entry, sizeof(Entry));
        }

    private:
        DataList *datalist;
        index_type *index;
    };
}