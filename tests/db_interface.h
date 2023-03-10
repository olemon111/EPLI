#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <future>
#include "ycsb/ycsb-c.h"
#include "random.h"
#include "alex/alex.h"
#include "../src/epli.h"
#include "apex/apex.h"
#include "lbtree/lbtree_wrapper.hpp"
#include "fast-fair/btree.h"

// #define USE_MEM

using namespace std;
using namespace epltree;
using FastFair::btree;

/*
 *file_exists -- checks if file exists
 */
static inline int file_exists(char const *file) { return access(file, F_OK); }

namespace KV
{
  class Key_t
  {
    typedef std::array<double, 1> model_key_t;

  public:
    static constexpr size_t model_key_size() { return 1; }
    static Key_t max()
    {
      static Key_t max_key(std::numeric_limits<uint64_t>::max());
      return max_key;
    }
    static Key_t min()
    {
      static Key_t min_key(std::numeric_limits<uint64_t>::min());
      return min_key;
    }

    Key_t() : key(0) {}
    Key_t(uint64_t key) : key(key) {}
    Key_t(const Key_t &other) { key = other.key; }
    Key_t &operator=(const Key_t &other)
    {
      key = other.key;
      return *this;
    }

    model_key_t to_model_key() const
    {
      model_key_t model_key;
      model_key[0] = key;
      return model_key;
    }

    friend bool operator<(const Key_t &l, const Key_t &r) { return l.key < r.key; }
    friend bool operator>(const Key_t &l, const Key_t &r) { return l.key > r.key; }
    friend bool operator>=(const Key_t &l, const Key_t &r) { return l.key >= r.key; }
    friend bool operator<=(const Key_t &l, const Key_t &r) { return l.key <= r.key; }
    friend bool operator==(const Key_t &l, const Key_t &r) { return l.key == r.key; }
    friend bool operator!=(const Key_t &l, const Key_t &r) { return l.key != r.key; }

    uint64_t key;
  };
}

namespace dbInter
{

  static inline std::string human_readable(double size)
  {
    static const std::string suffix[] = {
        "B",
        "KB",
        "MB",
        "GB"};
    const int arr_len = 4;

    std::ostringstream out;
    out.precision(2);
    for (int divs = 0; divs < arr_len; ++divs)
    {
      if (size >= 1024.0)
      {
        size /= 1024.0;
      }
      else
      {
        out << std::fixed << size;
        return out.str() + suffix[divs];
      }
    }
    out << std::fixed << size;
    return out.str() + suffix[arr_len - 1];
  }

  class AlexDB : public ycsbc::KvDB
  {
    typedef uint64_t KEY_TYPE;
    typedef uint64_t PAYLOAD_TYPE;
    using Alloc = std::allocator<std::pair<KEY_TYPE, PAYLOAD_TYPE>>;
    typedef alex::Alex<KEY_TYPE, PAYLOAD_TYPE, alex::AlexCompare, Alloc> alex_t;

  public:
    AlexDB() : alex_(nullptr) {}
    AlexDB(alex_t *alex) : alex_(alex) {}
    virtual ~AlexDB()
    {
      delete alex_;
    }

    void Init()
    {
      alex_ = new alex_t();
    }

    void Bulk_load(const std::pair<uint64_t, uint64_t> data[], int size)
    {
      alex_->bulk_load(data, size);
    }

    void Info()
    {
      cout << "Data size: " << alex_->data_size() / (1024 * 1024 * 1024.0) << " GB" << endl;
      cout << "Model size: " << alex_->model_size() / (1024 * 1024.0) << " MB" << endl;
      cout << "Total size: " << (alex_->model_size() + alex_->data_size()) / (1024 * 1024 * 1024.0) << " GB" << endl;
    }

    int Put(uint64_t key, uint64_t value)
    {
      alex_->insert(key, value);
      return 1;
    }

    int Get(uint64_t key, uint64_t &value)
    {
      value = *(alex_->get_payload(key));
      return 1;
    }
    int Update(uint64_t key, uint64_t value)
    {
      uint64_t *addrs = (alex_->get_payload(key));
      *addrs = value;
      return 1;
    }
    int Delete(uint64_t key)
    {
      alex_->erase(key);
      return 1;
    }
    int Scan(uint64_t start_key, int len, std::vector<std::pair<uint64_t, uint64_t>> &results)
    {
      auto it = alex_->lower_bound(start_key);
      int num_entries = 0;
      while (num_entries < len && it != alex_->end())
      {
        results.push_back({it.key(), it.payload()});
        num_entries++;
        it++;
      }
      return 1;
    }
    void PrintStatic()
    {
    }

  private:
    alex_t *alex_;
  };

  class EPLIDB : public ycsbc::KvDB
  {
  public:
    EPLIDB() : epli_(nullptr) {}
    virtual ~EPLIDB()
    {
      NVM::env_exit();
      delete epli_;
    }

    void Recover(size_t load_size)
    {
      epli_->Recover(load_size);
    }

    void Init()
    {
      NVM::env_init();
      NVM::data_init();
      epli_ = new EPLI();
      epli_->Init();
    }

    void Init(bool recover)
    {
      NVM::env_init();
      NVM::data_init();
      epli_ = new EPLI();
      epli_->Init(recover);
    }

    void Bulk_load(const std::pair<uint64_t, uint64_t> data[], int size)
    {
      epli_->BulkLoad(data, size);
    }

    void Info()
    {
      epli_->Info();
      NVM::show_stat();
    }

    void Reset()
    {
      epli_->Reset();
    }

    int Put(uint64_t key, uint64_t value)
    {
      epli_->Put(key, value);
      return 1;
    }

    int Get(uint64_t key, uint64_t &value)
    {
      epli_->Get(key, value);
      return 1;
    }

    int Update(uint64_t key, uint64_t value) // TODO:
    {
      return 1;
    }

    int Delete(uint64_t key)
    {
      return 1;
    }

    int Scan(uint64_t start_key, int len, std::vector<std::pair<uint64_t, uint64_t>> &results)
    {
    }

  private:
    EPLI *epli_;
  };

  class ApexDB : public ycsbc::KvDB
  {
    typedef uint64_t KEY_TYPE;
    typedef uint64_t PAYLOAD_TYPE;
    using Alloc = my_alloc::allocator<std::pair<KEY_TYPE, PAYLOAD_TYPE>>;
    typedef apex::Apex<KEY_TYPE, PAYLOAD_TYPE, apex::AlexCompare, Alloc> apex_t;

  public:
    ApexDB() : apex_(nullptr) {}
    ApexDB(apex_t *apex) : apex_(apex) {}
    virtual ~ApexDB()
    {
      my_alloc::BasePMPool::ClosePool();
      delete apex_;
      apex_ = NULL;
    }

    void Recover(size_t load_size)
    {
      Init();
    }

    void Init()
    {
      Tree<uint64_t, uint64_t> *index = nullptr;

      bool recover = my_alloc::BasePMPool::Initialize(pool_name, pool_size);
      auto index_ptr = reinterpret_cast<Tree<uint64_t, uint64_t> **>(my_alloc::BasePMPool::GetRoot(sizeof(Tree<uint64_t, uint64_t> *)));
      if (recover)
      {
        cout << "recover\n";
        index = reinterpret_cast<Tree<uint64_t, uint64_t> *>(reinterpret_cast<char *>(*index_ptr) + 48);
        new (index) apex::Apex<uint64_t, uint64_t>(recover);
      }
      else
      {
        my_alloc::BasePMPool::ZAllocate(reinterpret_cast<void **>(index_ptr), sizeof(apex::Apex<uint64_t, uint64_t>) + 64);
        index = reinterpret_cast<Tree<uint64_t, uint64_t> *>(reinterpret_cast<char *>(*index_ptr) + 48);
        new (index) apex::Apex<uint64_t, uint64_t>();
      }
      apex_ = index;
    }

    void Bulk_load(const std::pair<uint64_t, uint64_t> data[], int size)
    {
      apex_->bulk_load(data, size);
    }

    void Info()
    {
      cout << "apex DRAM size: " << apex_->get_DRAM_size() / (1024 * 1024.0) << " MB." << endl;
      cout << "apex PM size: " << apex_->get_PM_size() / (1024 * 1024 * 1024.0) << " GB." << endl;
      // apex_->PrintInfo();
    }

    int Put(uint64_t key, uint64_t value)
    {
      apex_->insert(key, value);
      return 1;
    }
    int Get(uint64_t key, uint64_t &value)
    {
      apex_->search(key, &value);
      // assert(value == key);
      return 1;
    }
    int Update(uint64_t key, uint64_t value)
    {
      apex_->update(key, value);
      return 1;
    }
    int Delete(uint64_t key)
    {
      apex_->erase(key);
      return 1;
    }
    int Scan(uint64_t start_key, int len, std::vector<std::pair<uint64_t, uint64_t>> &results)
    {
      return 1;
    }
    void PrintStatic()
    {
    }

  private:
    Tree<uint64_t, uint64_t> *apex_;
  };

  class LBTreeDB : public ycsbc::KvDB
  {
  public:
    LBTreeDB() : tree_(nullptr) {}
    LBTreeDB(lbtree_wrapper *tree) : tree_(tree) {}
    virtual ~LBTreeDB() {}

    void Recover(size_t load_size)
    {
      constexpr const auto MEMPOOL_ALIGNMENT = 4096LL;
      size_t key_size_ = 0;
      size_t pool_size_ = ((size_t)(40UL * 1024 * 1024 * 1024));
      const char *pool_path_ = "/mnt/pmem1/lbl/lbtree-pool.obj";
      kp = new char[8];
      vp = new char[8];

      initUseful();
      worker_id = 0;
      worker_thread_num = 1;
      the_thread_mempools.init(worker_thread_num, 16 * 1024 * 1024 * 1024, MEMPOOL_ALIGNMENT);
      // the_thread_mempools.init(1, 4096, MEMPOOL_ALIGNMENT);
      the_thread_nvmpools.init(worker_thread_num, pool_path_, pool_size_);
      cout << "recover nvmpool" << endl;
      char *nvm_addr = (char *)nvmpool_alloc(256);
      cout << "nvm addr : " << (int *)nvm_addr << endl;
      nvmLogInit(worker_thread_num);
      tree_ = new lbtree_wrapper(nvm_addr, true);
      cout << "recover lbtree wrapper" << endl;
    }

    void Init()
    {
      constexpr const auto MEMPOOL_ALIGNMENT = 4096LL;
      size_t key_size_ = 0;
      size_t pool_size_ = ((size_t)(40UL * 1024 * 1024 * 1024));
      const char *pool_path_ = "/mnt/pmem1/lbl/lbtree-pool.obj";
      kp = new char[8];
      vp = new char[8];

      initUseful();
      worker_id = 0;
      worker_thread_num = 1;
      the_thread_mempools.init(worker_thread_num, 16 * 1024 * 1024 * 1024, MEMPOOL_ALIGNMENT);
      // the_thread_mempools.init(1, 4096, MEMPOOL_ALIGNMENT);
      the_thread_nvmpools.init(worker_thread_num, pool_path_, pool_size_);
      char *nvm_addr = (char *)nvmpool_alloc(256);
      // cout << "nvm addr : " << (int *)nvm_addr << endl;
      nvmLogInit(worker_thread_num);
      tree_ = new lbtree_wrapper(nvm_addr, false);
      // cout << "init lbtree wrapper" << endl;
    }

    void Info()
    {
      cout << "lbtree DRAM size: " << the_thread_mempools.tm_pools[worker_id].total_size << " MB" << endl;
      cout << "lbtree PM size: " << the_thread_nvmpools.tm_pools[worker_id].total_size / 1024.0 << " GB" << endl;
    }

    void Close()
    {
    }

    void Bulk_load(const std::pair<uint64_t, uint64_t> data[], int size)
    {
      // tree_->bulkload(size, );
    }
    int Put(uint64_t key, uint64_t value)
    {
      memcpy(kp, &key, sizeof(key));
      memcpy(vp, &value, sizeof(value));
      tree_->insert(kp, sizeof(key), vp, sizeof(value));
      return 1;
    }
    int Get(uint64_t key, uint64_t &value)
    {
      memcpy(kp, &key, sizeof(key));
      tree_->find(kp, sizeof(key), vp);
      uint64_t res;
      memcpy(&res, vp, sizeof(value));
      value = res;
      return 1;
    }
    int Update(uint64_t key, uint64_t value)
    {
      memcpy(kp, &key, sizeof(key));
      memcpy(vp, &value, sizeof(value));
      tree_->update(kp, sizeof(key), vp, sizeof(value));
      return 1;
    }
    int Delete(uint64_t key)
    {
      // tree_->del(key);
      return 1;
    }

    int Scan(uint64_t start_key, int len, std::vector<std::pair<uint64_t, uint64_t>> &results)
    {
      return 1;
    }
    void PrintStatic()
    {
      // tree_->PrintInfo();
    }

  private:
    lbtree_wrapper *tree_;
    char *kp, *vp;
  };

  class FastFairDb : public ycsbc::KvDB
  {
  public:
    FastFairDb() : tree_(nullptr) {}
    FastFairDb(btree *tree) : tree_(tree) {}
    virtual ~FastFairDb()
    {
      NVM::env_exit();
      delete tree_;
    }
    void Init()
    {
      NVM::data_init();
      tree_ = new btree();
      NVM::pmem_size = 0;
    }

    void Info()
    {
      // std::cout << "NVM WRITE : " << NVM::pmem_size << std::endl;
      tree_->PrintInfo();
      NVM::show_stat();
    }

    void Close()
    {
    }
    int Put(uint64_t key, uint64_t value)
    {
      tree_->btree_insert(key, (char *)value);
      return 1;
    }
    int Get(uint64_t key, uint64_t &value)
    {
      value = (uint64_t)tree_->btree_search(key);
      return 1;
    }
    int Update(uint64_t key, uint64_t value)
    {
      tree_->btree_delete(key);
      tree_->btree_insert(key, (char *)value);
      return 1;
    }

    int Delete(uint64_t key)
    {
      tree_->btree_delete(key);
      return 1;
    }

    int Scan(uint64_t start_key, int len, std::vector<std::pair<uint64_t, uint64_t>> &results)
    {
      tree_->btree_search_range(start_key, UINT64_MAX, results, len);
      return 1;
    }
    void PrintStatic()
    {
      NVM::show_stat();
      tree_->PrintInfo();
    }

  private:
    btree *tree_;
  };

} // namespace dbInter
