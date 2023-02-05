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

// #define USE_MEM

using namespace std;

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

} // namespace dbInter
