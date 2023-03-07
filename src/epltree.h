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
#ifdef USE_BITMAP
    typedef alex::Alex<key_type, MetaData *> index_type;
#else
    typedef alex::Alex<key_type, Entry *> index_type;
#endif

    class EPLTree
    {
    public:
        EPLTree(bool recover = false)
        {
            // init NVM data
            datalist = new DataList();
            datalist->Init(recover);
            // init index
            index = new index_type();
#ifdef USE_BITMAP
            base_addr = uint64_t(NVM::data_alloc->pmem_addr_); // record base address
            index->insert(INVALID_KEY, new MetaData(datalist->Head()));
#else
            if (!recover)
            {
                index->insert(INVALID_KEY, datalist->Head());
            }
#endif
        }

        ~EPLTree()
        {
            delete datalist;
            delete index;
        }

        void Recover(size_t load_size)
        {
#ifdef USE_BITMAP
            auto index_kvs = new pair<key_type, MetaData *>[load_size / KVS_PER_ENTRY + 1];
            size_t index_kvs_size = 0;
#else
            auto index_kvs = new pair<key_type, Entry *>[load_size / KVS_PER_ENTRY + 1];
            size_t index_kvs_size = 0;
#endif
            // load head entry
            Entry *cur = datalist->Head();
            while (cur != NULL)
            {
                index_kvs[index_kvs_size].first = cur->GetMinKey();
#ifdef USE_BITMAP
                MetaData *data = new MetaData(cur);
                index_kvs[index_kvs_size++].second = data;
#else
                index_kvs[index_kvs_size++].second = cur;
#endif
                cur = cur->next;
            }
            index->bulk_load(index_kvs, index_kvs_size); // min-key already in order
            delete index_kvs;
        }

        // bulk_kvs already sorted
        void BulkLoad(const kv_type bulk_kvs[], size_t num_keys)
        {
            cout << "bulk load " << num_keys << " kvs" << endl;
            if (!num_keys)
            {
                return;
            }
#ifdef USE_BITMAP
            auto index_kvs = new pair<key_type, MetaData *>[num_keys / KVS_PER_ENTRY + 1];
            size_t index_kvs_size = 0;
#else
            auto index_kvs = new pair<key_type, Entry *>[num_keys / KVS_PER_ENTRY + 1];
            size_t index_kvs_size = 0;
#endif
            // load head entry
            datalist->Head()->setOrderedKVs(bulk_kvs, min(num_keys, size_t(KVS_PER_ENTRY)));
            NVM::Mem_persist(datalist->Head(), sizeof(Entry));
            index->erase(INVALID_KEY);
#ifdef USE_BITMAP
            MetaData *data = new MetaData(datalist->Head());
            data->reset_n_bitmap(min(num_keys, size_t(KVS_PER_ENTRY)));
            // index->insert(datalist->Head()->GetMinKey(), data);
            index_kvs[index_kvs_size].first = datalist->Head()->GetMinKey();
            index_kvs[index_kvs_size++].second = data;
#else
            // index->insert(datalist->Head()->GetMinKey(), datalist->Head());
            index_kvs[index_kvs_size].first = datalist->Head()->GetMinKey();
            index_kvs[index_kvs_size++].second = datalist->Head();
#endif
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
                NVM::Mem_persist(cur, sizeof(Entry));
                cur = cur->next;
#ifdef USE_BITMAP
                MetaData *data = new MetaData(new_entry);
                data->reset_n_bitmap(min(num_keys - i * KVS_PER_ENTRY, size_t(KVS_PER_ENTRY)));
                // index->insert(new_min_key, data);
                index_kvs[index_kvs_size].first = new_min_key;
                index_kvs[index_kvs_size++].second = data;
#else
                // index->insert(new_min_key, new_entry);
                index_kvs[index_kvs_size].first = new_min_key;
                index_kvs[index_kvs_size++].second = new_entry;
#endif
            }
            index->bulk_load(index_kvs, index_kvs_size);
            NVM::Mem_persist(cur, sizeof(Entry));
            // datalist->Persist();
        }

        //         // bulk_kvs already sorted
        //         void BulkLoad(const kv_type bulk_kvs[], size_t num_keys)
        //         {
        //             cout << "bulk load " << num_keys << " kvs" << endl;
        //             if (!num_keys)
        //             {
        //                 return;
        //             }
        //             // load head entry
        //             datalist->Head()->setOrderedKVs(bulk_kvs, min(num_keys, size_t(KVS_PER_ENTRY)));
        //             NVM::Mem_persist(datalist->Head(), sizeof(Entry));
        //             index->erase(INVALID_KEY);
        // #ifdef USE_BITMAP
        //             MetaData *data = new MetaData(datalist->Head());
        //             data->reset_n_bitmap(min(num_keys, size_t(KVS_PER_ENTRY)));
        //             index->insert(datalist->Head()->GetMinKey(), data);
        // #else
        //             index->insert(datalist->Head()->GetMinKey(), datalist->Head());
        // #endif
        //             if (num_keys <= KVS_PER_ENTRY)
        //             {
        //                 return;
        //             }
        //             Entry *cur = datalist->Head();
        //             for (size_t i = 1; KVS_PER_ENTRY * i < num_keys; i++)
        //             {

        //                 Entry *new_entry = (Entry *)NVM::data_alloc->alloc_aligned(sizeof(Entry));
        //                 key_type new_min_key = bulk_kvs[i * KVS_PER_ENTRY].first;
        //                 new (new_entry) Entry(new_min_key, bulk_kvs + i * KVS_PER_ENTRY, min(num_keys - i * KVS_PER_ENTRY, size_t(KVS_PER_ENTRY)));
        //                 cur->next = new_entry;
        //                 NVM::Mem_persist(cur, sizeof(Entry));
        //                 cur = cur->next;
        // #ifdef USE_BITMAP
        //                 MetaData *data = new MetaData(new_entry);
        //                 data->reset_n_bitmap(min(num_keys - i * KVS_PER_ENTRY, size_t(KVS_PER_ENTRY)));
        //                 index->insert(new_min_key, data);
        // #else
        //                 index->insert(new_min_key, new_entry);
        // #endif
        //             }
        //             NVM::Mem_persist(cur, sizeof(Entry));
        //             // datalist->Persist();
        //         }

        int Get(key_type key, val_type &val)
        {
#ifdef USE_BITMAP
            Entry *entry = (*(index->get_payload_last_no_greater_than(key)))->entryPointer.pointer();
#else
            Entry *entry = *(index->get_payload_last_no_greater_than(key));
#endif
            // assert(key >= entry->GetMinKey());
            // cout << "get key: " << key << ", min key: " << entry->GetMinKey() << endl;
            if (entry->Get(key, val) != STATUS_OK) // key not exist
            {
                // cout << "key: " << key << " not exist, min key: " << entry->GetMinKey() << endl;
                val = 0;
                return STATUS_FAIL;
            }
            return STATUS_OK;
        }

        int Insert(key_type key, val_type val)
        {
#ifdef USE_BITMAP
            MetaData *data = *(index->get_payload_last_no_greater_than(key));
            Entry *entry = data->entryPointer.pointer();
#else
            Entry *entry = *(index->get_payload_last_no_greater_than(key));
#endif
            if (entry == NULL)
            {
#ifdef USE_BITMAP
                data = *(index->get_payload_last_no_greater_than(key - 1));
                entry = data->entryPointer.pointer(); // reach max
#else
                entry = *(index->get_payload_last_no_greater_than(key - 1)); // reach max
#endif
            }
            // assert(key >= entry->GetMinKey());
            // cout << "min key: " << entry->GetMinKey() << endl;
            int ret = 0;
#ifdef USE_BITMAP
            int pos = data->find_first_zero();
            if (pos >= KVS_PER_ENTRY) // overflow
            {
                splitEntry(data);
                // return Insert(key, val);
                if (key >= entry->next->GetMinKey()) // FIXME: may cause trouble in lgn test
                {
                    return entry->next->MaybePut(key, val);
                }
                return entry->MaybePut(key, val);
            }
            else
            {
                entry->PutAtPos(key, val, pos);
                data->set_bitmap(pos);
                return STATUS_OK;
            }
#else
            ret = entry->MaybePut(key, val);
#endif
            if (ret == STATUS_OVERFLOW) // need to split
            {
#ifdef USE_BITMAP
                splitEntry(data);
#else
                splitEntry(entry);
#endif
                return Insert(key, val); // insert again
                // if (key >= entry->next->GetMinKey()) // FIXME: may cause trouble in lgn test
                // {
                //     return entry->next->MaybePut(key, val);
                // }
                // return entry->MaybePut(key, val);
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

        double get_size()
        {
            return index->data_size() + index->model_size();
        }

    private:
        // void splitEntry(Entry *entry)
        // {
        //     // copy kvs and sort
        //     sort(entry->kvs, entry->kvs + KVS_PER_ENTRY, [](auto const &a, auto const &b)
        //          { return a.first < b.first; });
        //     // assign new entry
        //     Entry *new_entry = (Entry *)NVM::data_alloc->alloc_aligned(sizeof(Entry));
        //     key_type new_min_key = entry->kvs[KVS_PER_ENTRY / 2].first;
        //     new (new_entry) Entry(new_min_key, entry->kvs + KVS_PER_ENTRY / 2, (KVS_PER_ENTRY + 1) / 2);
        //     // mark split
        //     entry->setPersistFlag(1);
        //     // entry->setFlag(0);
        //     // add new entry to link list
        //     new_entry->next = entry->next;
        //     NVM::Mem_persist(new_entry, sizeof(Entry));
        //     //  update prev entry
        //     entry->next = new_entry;
        //     index->insert(new_min_key, new_entry);
        //     entry->resetRightHalfKVs();
        //     // end split
        //     entry->setFlag(0);
        //     NVM::Mem_persist(entry, sizeof(Entry));
        // }
#ifdef USE_BITMAP
        void splitEntry(MetaData *data)
        {
            Entry *entry = data->entryPointer.pointer();
            // copy kvs and sort
            memcpy(tmp_kvs, entry->kvs, sizeof(entry->kvs));
            sort(tmp_kvs, tmp_kvs + KVS_PER_ENTRY, [](auto const &a, auto const &b)
                 { return a.first < b.first; });
            // assign new entry
            Entry *new_entry = (Entry *)NVM::data_alloc->alloc_aligned(sizeof(Entry));
            key_type new_min_key = tmp_kvs[KVS_PER_ENTRY / 2].first;
            new (new_entry) Entry(new_min_key, tmp_kvs + KVS_PER_ENTRY / 2, (KVS_PER_ENTRY + 1) / 2);
            // mark split
            entry->setFlag(1);
            // entry->setPersistFlag(1);
            // add new entry to link list
            new_entry->next = entry->next;
            NVM::Mem_persist(new_entry, sizeof(Entry));
            //  update prev entry
            entry->next = new_entry;
            MetaData *newData = new MetaData(new_entry);
            newData->reset_half_bitmap();
            index->insert(new_min_key, newData);
            entry->setOrderedKVs(tmp_kvs, KVS_PER_ENTRY / 2);
            data->reset_half_bitmap();
            // end split
            entry->setFlag(0);
            NVM::Mem_persist(entry, sizeof(Entry));
        }
#else
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
            entry->setFlag(1);
            entry->setPersistFlag(1);
            // add new entry to link list
            new_entry->next = entry->next;
            NVM::Mem_persist(new_entry, sizeof(Entry));
            //  update prev entry
            entry->next = new_entry;
            index->insert(new_min_key, new_entry);
            entry->setOrderedKVs(tmp_kvs, KVS_PER_ENTRY / 2);
            // end split
            entry->setFlag(0);
            NVM::Mem_persist(entry, sizeof(Entry));
        }
#endif

    private:
        DataList *datalist;
        index_type *index;
    };
}