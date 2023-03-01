#pragma once
#include "util/nvm/nvm_alloc.h"
#include "pmem.h"

// #define USE_BITMAP // open to accelerate write

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

typedef uint64_t key_type;
typedef uint64_t val_type;
typedef std::pair<key_type, val_type> kv_type;

using namespace std;

namespace epltree
{
    // each Entry occupies 256B of NVM
    static const key_type INVALID_KEY = 0;
    static const int KVS_PER_ENTRY = 14;

    typedef int status;
    const static int STATUS_OK = 0;
    const static int STATUS_FAIL = -1;
    const static int STATUS_OVERFLOW = 1;

    kv_type tmp_kvs[KVS_PER_ENTRY];

#ifdef USE_BITMAP
    uint64_t base_addr;
#endif

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
            memset(this, 0, sizeof(Entry));
            min_key = init_min_key;
        }

        Entry(key_type init_min_key, const kv_type init_kvs[], size_t size)
        {
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

        void resetRightHalfKVs()
        {
            memset(kvs + KVS_PER_ENTRY / 2, 0, KVS_PER_ENTRY * sizeof(kv_type) / 2);
            // for (int i = KVS_PER_ENTRY / 2; i < KVS_PER_ENTRY; i++)
            // {
            //     kvs[i].first = INVALID_KEY;
            // }
        }

        void setPersistFlag(int v)
        {
            flag = v;
            clflush((char *)&flag);
            fence();
        }

        void setFlag(int v)
        {
            flag = v;
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

        // insert if there's empty place
        // please make sure there's no duplicate key
        status MaybePut(key_type key, val_type val)
        {
            // cout << "maybe put " << key << ", " << val << endl;
            int empty_pos = -1;
            for (int i = 0; i < KVS_PER_ENTRY; i++)
            {
                if (kvs[i].first == INVALID_KEY) // first empty postion
                {
                    empty_pos = i;
                    break;
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
            // NVM::Mem_persist(&kvs[empty_pos], sizeof(kv_type));
            clflush((char *)&kvs[empty_pos]);
            fence();
            return STATUS_OK;
        }

#ifdef USE_BITMAP
        // insert if there's empty place
        status PutAtPos(key_type key, val_type val, int pos)
        {
            // cout << "put " << key << " at pos " << pos << endl;
            // assert(pos >= 0 && pos < KVS_PER_ENTRY);
            // write value before key
            setVal(pos, val);
            setKey(pos, key);
            // NVM::Mem_persist(&kvs[empty_pos], sizeof(kv_type));
            clflush((char *)&kvs[pos]);
            fence();
            return STATUS_OK;
        }
#endif

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

#ifdef USE_BITMAP

    inline int ffs_short(uint16_t x) // x must be non-zero
    {
        // if (x == 0)
        // {
        //     return 0;
        // }
        int num = 1;
        if ((x & 0x00FF) == 0)
        {
            num += 8;
            x >>= 8;
        }
        if ((x & 0x000F) == 0)
        {
            num += 4;
            x >>= 4;
        }
        if ((x & 0x0003) == 0)
        {
            num += 2;
            x >>= 2;
        }
        if ((x & 0x0001) == 0)
        {
            num += 1;
        }
        return num;
    }

    class __attribute__((packed)) EntryPointer
    {

#define READ_SIX_BYTE(addr) ((*(uint64_t *)addr) & 0x0000FFFFFFFFFFFFUL)
        uint8_t pointer_[6]; // uint48_t, LSB == 1 means NULL
    public:
        EntryPointer() { pointer_[0] = 1; }

        ALWAYS_INLINE bool HasSetup() const { return !(pointer_[0] & 1); };

        void Setup(Entry *entry)
        {
            uint64_t pointer = (uint64_t)(entry)-base_addr;
            memcpy(pointer_, &pointer, sizeof(pointer_));
        }

        ALWAYS_INLINE Entry *pointer() const
        {
            return (Entry *)(READ_SIX_BYTE(pointer_) + base_addr);
        }
    };

    struct MetaData
    {
        uint16_t bitmap;           // 2B bitmap
        EntryPointer entryPointer; // 6B pointer
        // Entry *entry;
        /* constructor */
        MetaData()
        {
            // memset(this, 0, sizeof(MetaData));
            bitmap = 0x0000;
        }

        MetaData(Entry *entry)
        {
            bitmap = 0x0000;
            this->entryPointer.Setup(entry);
            // this->entry = entry;
        }

        inline void set_bitmap(int pos) // pos = 0..15
        {
            bitmap |= 1 << pos;
        }

        inline int find_first_zero() // -1 if not found
        {
            return ffs_short(~bitmap) - 1;
        }

        inline void reset_half_bitmap()
        {
            bitmap = 0x007F;
        }

        inline void reset_n_bitmap(size_t n)
        {
            if (likely(n == KVS_PER_ENTRY))
            {
                bitmap = 0x3FFF;
                return;
            }
            while (n--)
            {
                bitmap |= 1 << n;
            }
        }
    };
#endif
}