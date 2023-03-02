#pragma once

#include "util/nvm/nvm_alloc.h"
#include "entry.h"

using namespace std;

namespace epltree
{
    class DataList
    {
    public:
        DataList() {}
        ~DataList()
        {
            Entry *cur = entry_head;
            while (cur != nullptr)
            {
                Entry *prev = cur;
                cur = cur->next;
                NVM::data_alloc->Free(prev, sizeof(Entry));
            }
        }

        void Init(bool recover = false)
        {
            entry_head = (Entry *)NVM::data_alloc->alloc_aligned(sizeof(Entry));
            if (recover)
            {
                return;
            }
            new (entry_head) Entry(INVALID_KEY);
            NVM::Mem_persist(entry_head, sizeof(Entry));
        }

        Entry *Head()
        {
            return entry_head;
        }

        // void Persist()
        // {
        //     Entry *cur = entry_head;
        //     while (cur != nullptr)
        //     {
        //         NVM::Mem_persist(cur, sizeof(Entry));
        //         cur = cur->next;
        //     }
        // }

        void PrintKVsByMinKey(key_type min_key)
        {
            Entry *cur = entry_head;
            while (cur != nullptr)
            {
                if (cur->GetMinKey() == min_key)
                {
                    cur->PrintKVs();
                }
                cur = cur->next;
            }
        }

    private:
        Entry *entry_head; // head pointer of data list
    };

}