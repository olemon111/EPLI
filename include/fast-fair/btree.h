// /*
//    Copyright (c) 2018, UNIST. All rights reserved. The license is a free
//    non-exclusive, non-transferable license to reproduce, use, modify and display
//    the source code version of the Software, with or without modifications solely
//    for non-commercial research, educational or evaluation purposes. The license
//    does not entitle Licensee to technical support, telephone assistance,
//    enhancements or updates to the Software. All rights, title to and ownership
//    interest in the Software, including all intellectual property rights therein
//    shall remain in UNIST.
// */

// #include <unistd.h>
// #include <stdio.h>
// #include <stdint.h>
// #include <time.h>
// #include <stdlib.h>
// #include <math.h>
// #include <iostream>
// #include <fstream>
// #include <vector>
// #include <string.h>
// #include <cassert>
// #include <climits>
// #include <future>
// #include <mutex>

// #include "../util/nvm/nvm_alloc.h"

// #define PAGESIZE 256
// #define CACHE_LINE_SIZE 64

// #define IS_FORWARD(c) (c % 2 == 0)

// using entry_key_t = uint64_t;
// using namespace std;

// inline void clflush(char *data, int len)
// {
// #ifndef USE_MEM
//   NVM::Mem_persist(data, len);
// #endif
// }

// namespace FastFair
// {

//   // const size_t NVM_ValueSize = 256;
//   static void alloc_memalign(void **ret, size_t alignment, size_t size)
//   {
// #ifdef USE_MEM
//     posix_memalign(ret, alignment, size);
// #else
//     *ret = NVM::data_alloc->alloc(size);
// #endif
//   }

//   class page;

//   class btree
//   {
//   private:
//     int height;
//     char *root;

//   public:
//     btree();
//     btree(page *root);
//     void setNewRoot(char *);
//     void btree_insert(entry_key_t, char *);
//     void btree_insert_internal(char *, entry_key_t, char *, uint32_t);
//     void btree_delete(entry_key_t);
//     void btree_delete_internal(entry_key_t, char *, uint32_t, entry_key_t *, bool *, page **);
//     char *btree_search(entry_key_t);
//     page *btree_search_leaf(entry_key_t);
//     void btree_search_range(entry_key_t, entry_key_t, unsigned long *);
//     void btree_search_range(entry_key_t, entry_key_t, std::vector<pair<entry_key_t, uint64_t>> &result, int &size);
//     void btree_search_range(entry_key_t, entry_key_t, void **values, int &size);
//     int btree_search_range(entry_key_t, uint32_t, std::pair<entry_key_t, entry_key_t> *&);
//     void printAll();
//     void PrintInfo();
//     void CalculateSapce(uint64_t &space);

//     friend class page;
//   };

//   class header
//   {
//   private:
//     page *leftmost_ptr;     // 8 bytes
//     page *sibling_ptr;      // 8 bytes
//     uint32_t level;         // 4 bytes
//     uint8_t switch_counter; // 1 bytes
//     uint8_t is_deleted;     // 1 bytes
//     int16_t last_index;     // 2 bytes
//     char dummy[8];          // 8 bytes

//     friend class page;
//     friend class btree;

//   public:
//     header()
//     {
//       leftmost_ptr = NULL;
//       sibling_ptr = NULL;
//       switch_counter = 0;
//       last_index = -1;
//       is_deleted = false;
//     }

//     ~header()
//     {
//     }
//   };

//   class entry
//   {
//   private:
//     entry_key_t key; // 8 bytes
//     char *ptr;       // 8 bytes
//   public:
//     entry()
//     {
//       key = LONG_MAX;
//       ptr = NULL;
//     }

//     friend class page;
//     friend class btree;
//   };

//   const int cardinality = (PAGESIZE - sizeof(header)) / sizeof(entry);
//   const int count_in_line = CACHE_LINE_SIZE / sizeof(entry);

//   class page
//   {
//   private:
//     header hdr;                 // header in persistent memory, 16 bytes
//     entry records[cardinality]; // slots in persistent memory, 16 bytes * n

//   public:
//     friend class btree;

//     page(uint32_t level = 0)
//     {
//       hdr.level = level;
//       records[0].ptr = NULL;
//     }

//     // this is called when tree grows
//     page(page *left, entry_key_t key, page *right, uint32_t level = 0)
//     {
//       hdr.leftmost_ptr = left;
//       hdr.level = level;
//       records[0].key = key;
//       records[0].ptr = (char *)right;
//       records[1].ptr = NULL;

//       hdr.last_index = 0;

//       clflush((char *)this, sizeof(page));
//     }

//     void *operator new(size_t size)
//     {
//       void *ret;
//       //   posix_memalign(&ret,64,size);
//       alloc_memalign(&ret, 64, size);
//       return ret;
//     }

//     uint32_t GetLevel()
//     {
//       return hdr.level;
//     }

//     void linear_search_range(entry_key_t min, entry_key_t max,
//                              std::vector<pair<entry_key_t, uint64_t>> &result, int &size);
//     void linear_search_range(entry_key_t min, entry_key_t max, void **values, int &size);

//     inline int count()
//     {
//       uint8_t previous_switch_counter;
//       int count = 0;
//       do
//       {
//         previous_switch_counter = hdr.switch_counter;
//         count = hdr.last_index + 1;

//         while (count >= 0 && records[count].ptr != NULL)
//         {
//           if (IS_FORWARD(previous_switch_counter))
//             ++count;
//           else
//             --count;
//         }

//         if (count < 0)
//         {
//           count = 0;
//           while (records[count].ptr != NULL)
//           {
//             ++count;
//           }
//         }

//       } while (previous_switch_counter != hdr.switch_counter);

//       return count;
//     }

//     inline bool remove_key(entry_key_t key)
//     {
//       // Set the switch_counter
//       if (IS_FORWARD(hdr.switch_counter))
//         ++hdr.switch_counter;

//       bool shift = false;
//       int i;
//       for (i = 0; records[i].ptr != NULL; ++i)
//       {
//         NVM::const_stat.AddCompare();
//         if (!shift && records[i].key == key)
//         {
//           records[i].ptr = (i == 0) ? (char *)hdr.leftmost_ptr : records[i - 1].ptr;
//           shift = true;
//         }

//         if (shift)
//         {
//           records[i].key = records[i + 1].key;
//           records[i].ptr = records[i + 1].ptr;

//           // sbh modify
//           // clflush((char *)(&records[i]), sizeof(entry));
//           // flush
//           uint64_t records_ptr = (uint64_t)(&records[i]);
//           int remainder = records_ptr % CACHE_LINE_SIZE;
//           bool do_flush = (remainder == 0) ||
//                           ((((int)(remainder + sizeof(entry)) / CACHE_LINE_SIZE) == 1) &&
//                            ((remainder + sizeof(entry)) % CACHE_LINE_SIZE) != 0);
//           if (do_flush)
//           {
//             clflush((char *)records_ptr, CACHE_LINE_SIZE);
//           }
//         }
//       }

//       if (shift)
//       {
//         --hdr.last_index;
//         clflush((char *)&(hdr.last_index), sizeof(int16_t));
//       }
//       return shift;
//     }

//     bool remove(btree *bt, entry_key_t key, bool only_rebalance = false, bool with_lock = true)
//     {
//       if (!only_rebalance)
//       {
//         register int num_entries_before = count();

//         // This node is root
//         if (this == (page *)bt->root)
//         {
//           if (hdr.level > 0)
//           {
//             if (num_entries_before == 1 && !hdr.sibling_ptr)
//             {
//               bt->root = (char *)hdr.leftmost_ptr;
//               clflush((char *)&(bt->root), sizeof(char *));

//               hdr.is_deleted = 1;
//               clflush((char *)&(hdr.is_deleted), sizeof(uint8_t));
//             }
//           }

//           // Remove the key from this node
//           bool ret = remove_key(key);
//           return true;
//         }

//         bool should_rebalance = true;
//         // check the node utilization
//         if (num_entries_before - 1 >= (int)((cardinality - 1) * 0.5))
//         {
//           should_rebalance = false;
//         }

//         // Remove the key from this node
//         bool ret = remove_key(key);

//         if (!should_rebalance)
//         {
//           return (hdr.leftmost_ptr == NULL) ? ret : true;
//         }
//       }

//       // Remove a key from the parent node
//       entry_key_t deleted_key_from_parent = 0;
//       bool is_leftmost_node = false;
//       page *left_sibling;
//       bt->btree_delete_internal(key, (char *)this, hdr.level + 1,
//                                 &deleted_key_from_parent, &is_leftmost_node, &left_sibling);

//       if (is_leftmost_node)
//       {
//         hdr.sibling_ptr->remove(bt, hdr.sibling_ptr->records[0].key, true,
//                                 with_lock);
//         return true;
//       }

//       register int num_entries = count();
//       register int left_num_entries = left_sibling->count();

//       // Merge or Redistribution
//       int total_num_entries = num_entries + left_num_entries;
//       if (hdr.leftmost_ptr)
//         ++total_num_entries;

//       entry_key_t parent_key;

//       if (total_num_entries > cardinality - 1)
//       { // Redistribution
//         register int m = (int)ceil(total_num_entries / 2);

//         if (num_entries < left_num_entries)
//         { // left -> right
//           if (hdr.leftmost_ptr == nullptr)
//           {
//             for (int i = left_num_entries - 1; i >= m; i--)
//             {
//               insert_key(left_sibling->records[i].key, left_sibling->records[i].ptr, &num_entries);
//             }

//             left_sibling->records[m].ptr = nullptr;
//             clflush((char *)&(left_sibling->records[m].ptr), sizeof(char *));

//             left_sibling->hdr.last_index = m - 1;
//             clflush((char *)&(left_sibling->hdr.last_index), sizeof(int16_t));

//             parent_key = records[0].key;
//           }
//           else
//           {
//             insert_key(deleted_key_from_parent, (char *)hdr.leftmost_ptr,
//                        &num_entries);

//             for (int i = left_num_entries - 1; i > m; i--)
//             {
//               insert_key(left_sibling->records[i].key, left_sibling->records[i].ptr, &num_entries);
//             }

//             parent_key = left_sibling->records[m].key;

//             hdr.leftmost_ptr = (page *)left_sibling->records[m].ptr;
//             clflush((char *)&(hdr.leftmost_ptr), sizeof(page *));

//             left_sibling->records[m].ptr = nullptr;
//             clflush((char *)&(left_sibling->records[m].ptr), sizeof(char *));

//             left_sibling->hdr.last_index = m - 1;
//             clflush((char *)&(left_sibling->hdr.last_index), sizeof(int16_t));
//           }

//           if (left_sibling == ((page *)bt->root))
//           {
//             page *new_root = new page(left_sibling, parent_key, this, hdr.level + 1);
//             bt->setNewRoot((char *)new_root);
//           }
//           else
//           {
//             bt->btree_insert_internal((char *)left_sibling, parent_key, (char *)this, hdr.level + 1);
//           }
//         }
//         else
//         { // from leftmost case
//           hdr.is_deleted = 1;
//           clflush((char *)&(hdr.is_deleted), sizeof(uint8_t));

//           page *new_sibling = new page(hdr.level);
//           new_sibling->hdr.sibling_ptr = hdr.sibling_ptr;

//           int num_dist_entries = num_entries - m;
//           int new_sibling_cnt = 0;

//           if (hdr.leftmost_ptr == nullptr)
//           {
//             for (int i = 0; i < num_dist_entries; i++)
//             {
//               left_sibling->insert_key(records[i].key, records[i].ptr,
//                                        &left_num_entries);
//             }

//             for (int i = num_dist_entries; records[i].ptr != NULL; i++)
//             {
//               new_sibling->insert_key(records[i].key, records[i].ptr,
//                                       &new_sibling_cnt, false);
//             }

//             clflush((char *)(new_sibling), sizeof(page));

//             left_sibling->hdr.sibling_ptr = new_sibling;
//             clflush((char *)&(left_sibling->hdr.sibling_ptr), sizeof(page *));

//             parent_key = new_sibling->records[0].key;
//           }
//           else
//           {
//             left_sibling->insert_key(deleted_key_from_parent,
//                                      (char *)hdr.leftmost_ptr, &left_num_entries);

//             for (int i = 0; i < num_dist_entries - 1; i++)
//             {
//               left_sibling->insert_key(records[i].key, records[i].ptr,
//                                        &left_num_entries);
//             }

//             parent_key = records[num_dist_entries - 1].key;

//             new_sibling->hdr.leftmost_ptr = (page *)records[num_dist_entries - 1].ptr;
//             for (int i = num_dist_entries; records[i].ptr != NULL; i++)
//             {
//               new_sibling->insert_key(records[i].key, records[i].ptr,
//                                       &new_sibling_cnt, false);
//             }
//             clflush((char *)(new_sibling), sizeof(page));

//             left_sibling->hdr.sibling_ptr = new_sibling;
//             clflush((char *)&(left_sibling->hdr.sibling_ptr), sizeof(page *));
//           }

//           if (left_sibling == ((page *)bt->root))
//           {
//             page *new_root = new page(left_sibling, parent_key, new_sibling, hdr.level + 1);
//             bt->setNewRoot((char *)new_root);
//           }
//           else
//           {
//             bt->btree_insert_internal((char *)left_sibling, parent_key, (char *)new_sibling, hdr.level + 1);
//           }
//         }
//       }
//       else
//       {
//         hdr.is_deleted = 1;
//         clflush((char *)&(hdr.is_deleted), sizeof(uint8_t));
//         if (hdr.leftmost_ptr)
//           left_sibling->insert_key(deleted_key_from_parent,
//                                    (char *)hdr.leftmost_ptr, &left_num_entries);

//         for (int i = 0; records[i].ptr != NULL; ++i)
//         {
//           left_sibling->insert_key(records[i].key, records[i].ptr, &left_num_entries);
//         }

//         left_sibling->hdr.sibling_ptr = hdr.sibling_ptr;
//         clflush((char *)&(left_sibling->hdr.sibling_ptr), sizeof(page *));
//       }

//       return true;
//     }

//     inline void
//     insert_key(entry_key_t key, char *ptr, int *num_entries, bool flush = true,
//                bool update_last_index = true)
//     {
//       // update switch_counter
//       if (!IS_FORWARD(hdr.switch_counter))
//         ++hdr.switch_counter;

//       // FAST
//       if (*num_entries == 0)
//       { // this page is empty
//         entry *new_entry = (entry *)&records[0];
//         entry *array_end = (entry *)&records[1];
//         new_entry->key = (entry_key_t)key;
//         new_entry->ptr = (char *)ptr;

//         array_end->ptr = (char *)NULL;

//         if (flush)
//         {
//           clflush((char *)this, CACHE_LINE_SIZE);
//         }
//       }
//       else
//       {
//         int i = *num_entries - 1, inserted = 0, to_flush_cnt = 0;
//         records[*num_entries + 1].ptr = records[*num_entries].ptr;
//         // clflush((char*)&(records[*num_entries+1].ptr), sizeof(char*));
//         if (flush)
//         {
//           if ((uint64_t) & (records[*num_entries + 1].ptr) % CACHE_LINE_SIZE == 0)
//             clflush((char *)&(records[*num_entries + 1].ptr), sizeof(char *));
//         }

//         // FAST
//         for (i = *num_entries - 1; i >= 0; i--)
//         {
//           NVM::const_stat.AddCompare();
//           if (key < records[i].key)
//           {
//             records[i + 1].ptr = records[i].ptr;
//             records[i + 1].key = records[i].key;

//             // clflush((char *)(&records[i+1]), sizeof(entry));
//             if (flush)
//             {
//               uint64_t records_ptr = (uint64_t)(&records[i + 1]);

//               int remainder = records_ptr % CACHE_LINE_SIZE;
//               bool do_flush = (remainder == 0) ||
//                               ((((int)(remainder + sizeof(entry)) / CACHE_LINE_SIZE) == 1) && ((remainder + sizeof(entry)) % CACHE_LINE_SIZE) != 0);
//               if (do_flush)
//               {
//                 clflush((char *)records_ptr, CACHE_LINE_SIZE);
//                 to_flush_cnt = 0;
//               }
//               else
//                 ++to_flush_cnt;
//             }
//           }
//           else
//           {
//             records[i + 1].ptr = records[i].ptr;
//             records[i + 1].key = key;
//             records[i + 1].ptr = ptr;
//             // clflush((char *)(&records[i+1]), sizeof(entry));
//             if (flush)
//               clflush((char *)&records[i + 1], sizeof(entry));
//             inserted = 1;
//             break;
//           }
//         }
//         if (inserted == 0)
//         {
//           records[0].ptr = (char *)hdr.leftmost_ptr;
//           records[0].key = key;
//           records[0].ptr = ptr;
//           if (flush)
//             clflush((char *)&records[0], sizeof(entry));
//         }
//       }

//       if (update_last_index)
//       {
//         hdr.last_index = *num_entries;
//         clflush((char *)&(hdr.last_index), sizeof(int16_t));
//       }
//       ++(*num_entries);
//     }

//     // Insert a new key - FAST and FAIR
//     page *store(btree *bt, char *left, entry_key_t key, char *right,
//                 bool flush, page *invalid_sibling = NULL)
//     {
//       // If this node has a sibling node,
//       if (hdr.sibling_ptr && (hdr.sibling_ptr != invalid_sibling))
//       {
//         // Compare this key with the first key of the sibling
//         NVM::const_stat.AddCompare();
//         if (key > hdr.sibling_ptr->records[0].key)
//         {
//           return hdr.sibling_ptr->store(bt, NULL, key, right,
//                                         true, invalid_sibling);
//         }
//       }

//       register int num_entries = count();

//       // FAST
//       if (num_entries < cardinality - 1)
//       {
//         insert_key(key, right, &num_entries, flush);
//         return this;
//       }
//       else
//       { // FAIR
//         // overflow
//         // create a new node
//         page *sibling = new page(hdr.level);
//         register int m = (int)ceil(num_entries / 2);
//         entry_key_t split_key = records[m].key;

//         // migrate half of keys into the sibling
//         int sibling_cnt = 0;
//         if (hdr.leftmost_ptr == NULL)
//         { // leaf node
//           for (int i = m; i < num_entries; ++i)
//           {
//             sibling->insert_key(records[i].key, records[i].ptr, &sibling_cnt, false);
//           }
//         }
//         else
//         { // internal node
//           for (int i = m + 1; i < num_entries; ++i)
//           {
//             sibling->insert_key(records[i].key, records[i].ptr, &sibling_cnt, false);
//           }
//           sibling->hdr.leftmost_ptr = (page *)records[m].ptr;
//         }

//         sibling->hdr.sibling_ptr = hdr.sibling_ptr;
//         clflush((char *)sibling, sizeof(page));

//         hdr.sibling_ptr = sibling;
//         clflush((char *)&hdr, sizeof(hdr));

//         // set to NULL
//         if (IS_FORWARD(hdr.switch_counter))
//           hdr.switch_counter += 2;
//         else
//           ++hdr.switch_counter;
//         records[m].ptr = NULL;
//         clflush((char *)&records[m], sizeof(entry));

//         hdr.last_index = m - 1;
//         clflush((char *)&(hdr.last_index), sizeof(int16_t));

//         num_entries = hdr.last_index + 1;

//         page *ret;

//         // insert the key
//         NVM::const_stat.AddCompare();
//         if (key < split_key)
//         {
//           insert_key(key, right, &num_entries);
//           ret = this;
//         }
//         else
//         {
//           sibling->insert_key(key, right, &sibling_cnt);
//           ret = sibling;
//         }

//         // Set a new root or insert the split key to the parent
//         if (bt->root == (char *)this)
//         { // only one node can update the root ptr
//           page *new_root = new page((page *)this, split_key, sibling,
//                                     hdr.level + 1);
//           bt->setNewRoot((char *)new_root);
//         }
//         else
//         {
//           bt->btree_insert_internal(NULL, split_key, (char *)sibling,
//                                     hdr.level + 1);
//         }

//         return ret;
//       }
//     }

//     // Search keys with linear search
//     void linear_search_range(entry_key_t min, entry_key_t max, unsigned long *buf)
//     {
//       int i, off = 0;
//       uint8_t previous_switch_counter;
//       page *current = this;

//       while (current)
//       {
//         int old_off = off;
//         do
//         {
//           previous_switch_counter = current->hdr.switch_counter;
//           off = old_off;

//           entry_key_t tmp_key;
//           char *tmp_ptr;

//           if (IS_FORWARD(previous_switch_counter))
//           {
//             if ((tmp_key = current->records[0].key) > min)
//             {
//               NVM::const_stat.AddCompare();
//               if (tmp_key < max)
//               {
//                 if ((tmp_ptr = current->records[0].ptr) != NULL)
//                 {
//                   if (tmp_key == current->records[0].key)
//                   {
//                     if (tmp_ptr)
//                     {
//                       buf[off++] = (unsigned long)tmp_ptr;
//                     }
//                   }
//                 }
//               }
//               else
//                 return;
//             }

//             for (i = 1; current->records[i].ptr != NULL; ++i)
//             {
//               if ((tmp_key = current->records[i].key) > min)
//               {
//                 NVM::const_stat.AddCompare();
//                 if (tmp_key < max)
//                 {
//                   if ((tmp_ptr = current->records[i].ptr) != current->records[i - 1].ptr)
//                   {
//                     if (tmp_key == current->records[i].key)
//                     {
//                       if (tmp_ptr)
//                         buf[off++] = (unsigned long)tmp_ptr;
//                     }
//                   }
//                 }
//                 else
//                   return;
//               }
//             }
//           }
//           else
//           {
//             for (i = count() - 1; i > 0; --i)
//             {
//               if ((tmp_key = current->records[i].key) > min)
//               {
//                 NVM::const_stat.AddCompare();
//                 if (tmp_key < max)
//                 {
//                   if ((tmp_ptr = current->records[i].ptr) != current->records[i - 1].ptr)
//                   {
//                     if (tmp_key == current->records[i].key)
//                     {
//                       if (tmp_ptr)
//                         buf[off++] = (unsigned long)tmp_ptr;
//                     }
//                   }
//                 }
//                 else
//                   return;
//               }
//             }

//             if ((tmp_key = current->records[0].key) > min)
//             {
//               NVM::const_stat.AddCompare();
//               if (tmp_key < max)
//               {
//                 if ((tmp_ptr = current->records[0].ptr) != NULL)
//                 {
//                   if (tmp_key == current->records[0].key)
//                   {
//                     if (tmp_ptr)
//                     {
//                       buf[off++] = (unsigned long)tmp_ptr;
//                     }
//                   }
//                 }
//               }
//               else
//                 return;
//             }
//           }
//         } while (previous_switch_counter != current->hdr.switch_counter);

//         current = current->hdr.sibling_ptr;
//       }
//     }

//     // Search keys with linear search
//     int linear_search_range(entry_key_t min, entry_key_t max,
//                             uint32_t to_scan, std::pair<entry_key_t, entry_key_t> *&res)
//     {
//       int i, off = 0;
//       uint8_t previous_switch_counter;
//       page *current = this;

//       while (current)
//       {
//         int old_off = off;
//         do
//         {
//           previous_switch_counter = current->hdr.switch_counter;
//           off = old_off;

//           entry_key_t tmp_key;
//           char *tmp_ptr;

//           if (IS_FORWARD(previous_switch_counter))
//           {
//             if ((tmp_key = current->records[0].key) > min)
//             {
//               if (tmp_key < max)
//               {
//                 if ((tmp_ptr = current->records[0].ptr) != NULL)
//                 {
//                   if (tmp_key == current->records[0].key)
//                   {
//                     if (tmp_ptr)
//                     {
//                       if (off < to_scan)
//                       {
//                         res[off].first = tmp_key;
//                         res[off++].second = (unsigned long)tmp_ptr;
//                       }
//                       else
//                         return off;
//                     }
//                   }
//                 }
//               }
//               else
//                 return off;
//             }

//             for (i = 1; current->records[i].ptr != NULL; ++i)
//             {
//               if ((tmp_key = current->records[i].key) > min)
//               {
//                 if (tmp_key < max)
//                 {
//                   if ((tmp_ptr = current->records[i].ptr) !=
//                       current->records[i - 1].ptr)
//                   {
//                     if (tmp_key == current->records[i].key)
//                     {
//                       if (tmp_ptr)
//                       {
//                         if (off < to_scan)
//                         {
//                           res[off].first = tmp_key;
//                           res[off++].second = (unsigned long)tmp_ptr;
//                         }
//                         else
//                           return off;
//                       }
//                     }
//                   }
//                 }
//                 else
//                   return off;
//               }
//             }
//           }
//           else
//           {
//             for (i = count() - 1; i > 0; --i)
//             {
//               if ((tmp_key = current->records[i].key) > min)
//               {
//                 if (tmp_key < max)
//                 {
//                   if ((tmp_ptr = current->records[i].ptr) !=
//                       current->records[i - 1].ptr)
//                   {
//                     if (tmp_key == current->records[i].key)
//                     {
//                       if (tmp_ptr)
//                       {
//                         if (off < to_scan)
//                         {
//                           res[off].first = tmp_key;
//                           res[off++].second = (unsigned long)tmp_ptr;
//                         }
//                         else
//                           return off;
//                       }
//                     }
//                   }
//                 }
//                 else
//                   return off;
//               }
//             }

//             if ((tmp_key = current->records[0].key) > min)
//             {
//               if (tmp_key < max)
//               {
//                 if ((tmp_ptr = current->records[0].ptr) != NULL)
//                 {
//                   if (tmp_key == current->records[0].key)
//                   {
//                     if (tmp_ptr)
//                     {
//                       if (off < to_scan)
//                       {
//                         res[off].first = tmp_key;
//                         res[off++].second = (unsigned long)tmp_ptr;
//                       }
//                       else
//                         return off;
//                     }
//                   }
//                 }
//               }
//               else
//                 return off;
//             }
//           }
//         } while (previous_switch_counter != current->hdr.switch_counter);

//         current = current->hdr.sibling_ptr;
//       }
//       return off;
//     }

//     char *linear_search(entry_key_t key)
//     {
//       int i = 1;
//       uint8_t previous_switch_counter;
//       char *ret = NULL;
//       char *t;
//       entry_key_t k;

//       if (hdr.leftmost_ptr == NULL)
//       { // Search a leaf node
//         do
//         {
//           previous_switch_counter = hdr.switch_counter;
//           ret = NULL;

//           // search from left ro right
//           if (IS_FORWARD(previous_switch_counter))
//           {
//             NVM::const_stat.AddCompare();
//             if ((k = records[0].key) == key)
//             {
//               if ((t = records[0].ptr) != NULL)
//               {
//                 if (k == records[0].key)
//                 {
//                   ret = t;
//                   continue;
//                 }
//               }
//             }

//             for (i = 1; records[i].ptr != NULL; ++i)
//             {
//               NVM::const_stat.AddCompare();
//               if ((k = records[i].key) == key)
//               {
//                 if (records[i - 1].ptr != (t = records[i].ptr))
//                 {
//                   if (k == records[i].key)
//                   {
//                     ret = t;
//                     break;
//                   }
//                 }
//               }
//             }
//           }
//           else
//           { // search from right to left
//             for (i = count() - 1; i > 0; --i)
//             {
//               NVM::const_stat.AddCompare();
//               if ((k = records[i].key) == key)
//               {
//                 if (records[i - 1].ptr != (t = records[i].ptr) && t)
//                 {
//                   if (k == records[i].key)
//                   {
//                     ret = t;
//                     break;
//                   }
//                 }
//               }
//             }

//             if (!ret)
//             {
//               NVM::const_stat.AddCompare();
//               if ((k = records[0].key) == key)
//               {
//                 if (NULL != (t = records[0].ptr) && t)
//                 {
//                   if (k == records[0].key)
//                   {
//                     ret = t;
//                     continue;
//                   }
//                 }
//               }
//             }
//           }
//         } while (hdr.switch_counter != previous_switch_counter);

//         if (ret)
//         {
//           return ret;
//         }
//         NVM::const_stat.AddCompare();
//         if ((t = (char *)hdr.sibling_ptr) && key >= ((page *)t)->records[0].key)
//           return t;

//         return NULL;
//       }
//       else
//       { // internal node
//         do
//         {
//           previous_switch_counter = hdr.switch_counter;
//           ret = NULL;

//           if (IS_FORWARD(previous_switch_counter))
//           {
//             NVM::const_stat.AddCompare();
//             if (key < (k = records[0].key))
//             {
//               if ((t = (char *)hdr.leftmost_ptr) != records[0].ptr)
//               {
//                 ret = t;
//                 continue;
//               }
//             }

//             for (i = 1; records[i].ptr != NULL; ++i)
//             {
//               NVM::const_stat.AddCompare();
//               if (key < (k = records[i].key))
//               {
//                 if ((t = records[i - 1].ptr) != records[i].ptr)
//                 {
//                   ret = t;
//                   break;
//                 }
//               }
//             }

//             if (!ret)
//             {
//               ret = records[i - 1].ptr;
//               continue;
//             }
//           }
//           else
//           { // search from right to left
//             for (i = count() - 1; i >= 0; --i)
//             {
//               NVM::const_stat.AddCompare();
//               if (key >= (k = records[i].key))
//               {
//                 if (i == 0)
//                 {
//                   if ((char *)hdr.leftmost_ptr != (t = records[i].ptr))
//                   {
//                     ret = t;
//                     break;
//                   }
//                 }
//                 else
//                 {
//                   if (records[i - 1].ptr != (t = records[i].ptr))
//                   {
//                     ret = t;
//                     break;
//                   }
//                 }
//               }
//             }
//           }
//         } while (hdr.switch_counter != previous_switch_counter);

//         if ((t = (char *)hdr.sibling_ptr) != NULL)
//         {
//           NVM::const_stat.AddCompare();
//           if (key >= ((page *)t)->records[0].key)
//             return t;
//         }

//         if (ret)
//         {
//           return ret;
//         }
//         else
//           return (char *)hdr.leftmost_ptr;
//       }

//       return NULL;
//     }

//     // print a node
//     void print()
//     {
//       if (hdr.leftmost_ptr == NULL)
//         printf("[%d] leaf %p \n", this->hdr.level, this);
//       else
//         printf("[%d] internal %p \n", this->hdr.level, this);
//       printf("last_index: %d\n", hdr.last_index);
//       printf("switch_counter: %d\n", hdr.switch_counter);
//       printf("search direction: ");
//       if (IS_FORWARD(hdr.switch_counter))
//         printf("->\n");
//       else
//         printf("<-\n");

//       if (hdr.leftmost_ptr != NULL)
//         printf("%p ", hdr.leftmost_ptr);

//       for (int i = 0; records[i].ptr != NULL; ++i)
//         printf("%ld,%p ", records[i].key, records[i].ptr);

//       printf("%p ", hdr.sibling_ptr);

//       printf("\n");
//     }

//     void printAll()
//     {
//       if (hdr.leftmost_ptr == NULL)
//       {
//         printf("printing leaf node: ");
//         print();
//       }
//       else
//       {
//         printf("printing internal node: ");
//         print();
//         ((page *)hdr.leftmost_ptr)->printAll();
//         for (int i = 0; records[i].ptr != NULL; ++i)
//         {
//           ((page *)records[i].ptr)->printAll();
//         }
//       }
//     }

//     void CalculateSapce(uint64_t &space)
//     {
//       if (hdr.leftmost_ptr == NULL)
//       {
//         space += PAGESIZE;
//       }
//       else
//       {
//         space += PAGESIZE;
//         ((page *)hdr.leftmost_ptr)->CalculateSapce(space);
//         for (int i = 0; records[i].ptr != NULL; ++i)
//         {
//           ((page *)records[i].ptr)->CalculateSapce(space);
//         }
//       }
//     }
//   };

//   static inline page *NewBpNode()
//   {
//     return new page();
//   }

//   void page::linear_search_range(entry_key_t min, entry_key_t max,
//                                  std::vector<pair<uint64_t, uint64_t>> &result, int &size)
//   {
//     int i, off = 0;
//     uint8_t previous_switch_counter;
//     page *current = this;

//     while (current)
//     {
//       int old_off = off;
//       do
//       {
//         previous_switch_counter = current->hdr.switch_counter;
//         off = old_off;

//         entry_key_t tmp_key;
//         char *tmp_ptr;

//         if (IS_FORWARD(previous_switch_counter))
//         {
//           if ((tmp_key = current->records[0].key) > min)
//           {
//             NVM::const_stat.AddCompare();
//             if (tmp_key < max)
//             {
//               if ((tmp_ptr = current->records[0].ptr) != NULL)
//               {
//                 if (tmp_key == current->records[0].key)
//                 {
//                   if (tmp_ptr)
//                   {
//                     // buf[off++] = (unsigned long)tmp_ptr;
//                     result.push_back({tmp_key, (uint64_t)tmp_ptr});
//                     off++;
//                     if (off >= size)
//                     {
//                       return;
//                     }
//                   }
//                 }
//               }
//             }
//             else
//             {
//               size = off;
//               return;
//             }
//           }

//           for (i = 1; current->records[i].ptr != NULL; ++i)
//           {
//             if ((tmp_key = current->records[i].key) > min)
//             {
//               NVM::const_stat.AddCompare();
//               if (tmp_key < max)
//               {
//                 if ((tmp_ptr = current->records[i].ptr) != current->records[i - 1].ptr)
//                 {
//                   if (tmp_key == current->records[i].key)
//                   {
//                     if (tmp_ptr)
//                     {
//                       // buf[off++] = (unsigned long)tmp_ptr;
//                       result.push_back({tmp_key, (uint64_t)tmp_ptr});
//                       off++;
//                       if (off >= size)
//                       {
//                         return;
//                       }
//                     }
//                   }
//                 }
//               }
//               else
//               {
//                 size = off;
//                 return;
//               }
//             }
//           }
//         }
//         else
//         {
//           for (i = count() - 1; i > 0; --i)
//           {
//             if ((tmp_key = current->records[i].key) > min)
//             {
//               NVM::const_stat.AddCompare();
//               if (tmp_key < max)
//               {
//                 if ((tmp_ptr = current->records[i].ptr) != current->records[i - 1].ptr)
//                 {
//                   if (tmp_key == current->records[i].key)
//                   {
//                     if (tmp_ptr)
//                     {
//                       // buf[off++] = (unsigned long)tmp_ptr;
//                       result.push_back({tmp_key, (uint64_t)tmp_ptr});
//                       off++;
//                       if (off >= size)
//                       {
//                         return;
//                       }
//                     }
//                   }
//                 }
//               }
//               else
//               {
//                 size = off;
//                 return;
//               }
//             }
//           }

//           if ((tmp_key = current->records[0].key) > min)
//           {
//             NVM::const_stat.AddCompare();
//             if (tmp_key < max)
//             {
//               if ((tmp_ptr = current->records[0].ptr) != NULL)
//               {
//                 if (tmp_key == current->records[0].key)
//                 {
//                   if (tmp_ptr)
//                   {
//                     // buf[off++] = (unsigned long)tmp_ptr;
//                     result.push_back({tmp_key, (uint64_t)tmp_ptr});
//                     off++;
//                     if (off >= size)
//                     {
//                       return;
//                     }
//                   }
//                 }
//               }
//             }
//             else
//             {
//               size = off;
//               return;
//             }
//           }
//         }
//       } while (previous_switch_counter != current->hdr.switch_counter);

//       current = current->hdr.sibling_ptr;
//     }
//     size = off;
//   }

//   void page::linear_search_range(entry_key_t min, entry_key_t max, void **values, int &size)
//   {
//     int i, off = 0;
//     uint8_t previous_switch_counter;
//     page *current = this;

//     while (current)
//     {
//       int old_off = off;
//       do
//       {
//         previous_switch_counter = current->hdr.switch_counter;
//         off = old_off;

//         entry_key_t tmp_key;
//         char *tmp_ptr;

//         if (IS_FORWARD(previous_switch_counter))
//         {
//           if ((tmp_key = current->records[0].key) > min)
//           {
//             NVM::const_stat.AddCompare();
//             if (tmp_key < max)
//             {
//               if ((tmp_ptr = current->records[0].ptr) != NULL)
//               {
//                 if (tmp_key == current->records[0].key)
//                 {
//                   if (tmp_ptr)
//                   {
//                     // buf[off++] = (unsigned long)tmp_ptr;
//                     values[off] = tmp_ptr;
//                     off++;
//                     if (off >= size)
//                     {
//                       return;
//                     }
//                   }
//                 }
//               }
//             }
//             else
//             {
//               size = off;
//               return;
//             }
//           }

//           for (i = 1; current->records[i].ptr != NULL; ++i)
//           {
//             if ((tmp_key = current->records[i].key) > min)
//             {
//               NVM::const_stat.AddCompare();
//               if (tmp_key < max)
//               {
//                 if ((tmp_ptr = current->records[i].ptr) != current->records[i - 1].ptr)
//                 {
//                   if (tmp_key == current->records[i].key)
//                   {
//                     if (tmp_ptr)
//                     {
//                       // buf[off++] = (unsigned long)tmp_ptr;
//                       values[off] = tmp_ptr;
//                       off++;
//                       if (off >= size)
//                       {
//                         return;
//                       }
//                     }
//                   }
//                 }
//               }
//               else
//               {
//                 size = off;
//                 return;
//               }
//             }
//           }
//         }
//         else
//         {
//           for (i = count() - 1; i > 0; --i)
//           {
//             if ((tmp_key = current->records[i].key) > min)
//             {
//               NVM::const_stat.AddCompare();
//               if (tmp_key < max)
//               {
//                 if ((tmp_ptr = current->records[i].ptr) != current->records[i - 1].ptr)
//                 {
//                   if (tmp_key == current->records[i].key)
//                   {
//                     if (tmp_ptr)
//                     {
//                       // buf[off++] = (unsigned long)tmp_ptr;
//                       values[off] = tmp_ptr;
//                       off++;
//                       if (off >= size)
//                       {
//                         return;
//                       }
//                     }
//                   }
//                 }
//               }
//               else
//               {
//                 size = off;
//                 return;
//               }
//             }
//           }

//           if ((tmp_key = current->records[0].key) > min)
//           {
//             NVM::const_stat.AddCompare();
//             if (tmp_key < max)
//             {
//               if ((tmp_ptr = current->records[0].ptr) != NULL)
//               {
//                 if (tmp_key == current->records[0].key)
//                 {
//                   if (tmp_ptr)
//                   {
//                     // buf[off++] = (unsigned long)tmp_ptr;
//                     values[off] = tmp_ptr;
//                     off++;
//                     if (off >= size)
//                     {
//                       return;
//                     }
//                   }
//                 }
//               }
//             }
//             else
//             {
//               size = off;
//               return;
//             }
//           }
//         }
//       } while (previous_switch_counter != current->hdr.switch_counter);

//       current = current->hdr.sibling_ptr;
//     }
//     size = off;
//   }

//   btree::btree()
//   {
//     root = (char *)new page();
//     printf("[Fast-Fair]: root is %p, btree is %p.\n", root, this);
//     height = 1;
//   }

//   btree::btree(page *root_)
//   {
//     if (root_ == nullptr)
//     {
//       root = (char *)new page();
//       height = 1;
//     }
//     else
//     {
//       root = (char *)root_;
//       height = root_->GetLevel() + 1;
//     }
//     printf("[Fast-Fair]: root is %p, btree is %p, height is %d.\n", root, this, height);
//   }

//   void btree::setNewRoot(char *new_root)
//   {
//     this->root = (char *)new_root;
//     clflush((char *)&(this->root), sizeof(char *));
//     ++height;
//   }

//   char *btree::btree_search(entry_key_t key)
//   {
//     page *p = (page *)root;

//     while (p->hdr.leftmost_ptr != NULL)
//     {
//       p = (page *)p->linear_search(key);
//     }

//     page *t;
//     while ((t = (page *)p->linear_search(key)) == p->hdr.sibling_ptr)
//     {
//       p = t;
//       if (!p)
//       {
//         break;
//       }
//     }

//     if (!t)
//     {
//       // printf("NOT FOUND %llx, t = %x\n", key, t);
//       return NULL;
//     }

//     return (char *)t;
//   }

//   page *btree::btree_search_leaf(entry_key_t key)
//   {
//     page *p = (page *)root;

//     while (p->hdr.leftmost_ptr != NULL)
//     {
//       p = (page *)p->linear_search(key);
//     }
//     return p;
//   }

//   // insert the key in the leaf node
//   void btree::btree_insert(entry_key_t key, char *right)
//   { // need to be string
//     page *p = (page *)root;

//     while (p->hdr.leftmost_ptr != NULL)
//     {
//       p = (page *)p->linear_search(key);
//     }

//     if (!p->store(this, NULL, key, right, true))
//     { // store
//       btree_insert(key, right);
//     }
//   }

//   // store the key into the node at the given level
//   void btree::btree_insert_internal(char *left, entry_key_t key, char *right, uint32_t level)
//   {
//     if (level > ((page *)root)->hdr.level)
//       return;

//     page *p = (page *)this->root;

//     while (p->hdr.level > level)
//       p = (page *)p->linear_search(key);

//     if (!p->store(this, NULL, key, right, true))
//     {
//       btree_insert_internal(left, key, right, level);
//     }
//   }

//   void btree::btree_delete(entry_key_t key)
//   {
//     page *p = (page *)root;

//     while (p->hdr.leftmost_ptr != NULL)
//     {
//       p = (page *)p->linear_search(key);
//     }

//     page *t;
//     while ((t = (page *)p->linear_search(key)) == p->hdr.sibling_ptr)
//     {
//       p = t;
//       if (!p)
//         break;
//     }

//     if (p && t)
//     {
//       if (!p->remove(this, key))
//       {
//         btree_delete(key);
//       }
//     }
//     else
//     {
//       ;
//       // printf("not found the key to delete %llx\n", key);
//     }
//   }

//   void btree::btree_delete_internal(entry_key_t key, char *ptr, uint32_t level, entry_key_t *deleted_key,
//                                     bool *is_leftmost_node, page **left_sibling)
//   {
//     if (level > ((page *)this->root)->hdr.level)
//       return;

//     page *p = (page *)this->root;

//     while (p->hdr.level > level)
//     {
//       p = (page *)p->linear_search(key);
//     }

//     if ((char *)p->hdr.leftmost_ptr == ptr)
//     {
//       *is_leftmost_node = true;
//       return;
//     }

//     *is_leftmost_node = false;

//     for (int i = 0; p->records[i].ptr != NULL; ++i)
//     {
//       if (p->records[i].ptr == ptr)
//       {
//         if (i == 0)
//         {
//           if ((char *)p->hdr.leftmost_ptr != p->records[i].ptr)
//           {
//             *deleted_key = p->records[i].key;
//             *left_sibling = p->hdr.leftmost_ptr;
//             p->remove(this, *deleted_key, false, false);
//             break;
//           }
//         }
//         else
//         {
//           if (p->records[i - 1].ptr != p->records[i].ptr)
//           {
//             *deleted_key = p->records[i].key;
//             *left_sibling = (page *)p->records[i - 1].ptr;
//             p->remove(this, *deleted_key, false, false);
//             break;
//           }
//         }
//       }
//     }
//   }

//   // Function to search keys from "min" to "max"
//   void btree::btree_search_range(entry_key_t min, entry_key_t max, unsigned long *buf)
//   {
//     page *p = (page *)root;

//     while (p)
//     {
//       if (p->hdr.leftmost_ptr != NULL)
//       {
//         // The current page is internal
//         p = (page *)p->linear_search(min);
//       }
//       else
//       {
//         // Found a leaf
//         p->linear_search_range(min, max, buf);

//         break;
//       }
//     }
//   }

//   void btree::btree_search_range(entry_key_t min, entry_key_t max,
//                                  std::vector<pair<entry_key_t, uint64_t>> &result, int &size)
//   {
//     page *p = (page *)root;

//     while (p)
//     {
//       if (p->hdr.leftmost_ptr != NULL)
//       {
//         // The current page is internal
//         p = (page *)p->linear_search(min);
//       }
//       else
//       {
//         // Found a leaf
//         p->linear_search_range(min, max, result, size);

//         break;
//       }
//     }
//   }

//   void btree::btree_search_range(entry_key_t min, entry_key_t max, void **values, int &size)
//   {
//     page *p = (page *)root;

//     while (p)
//     {
//       if (p->hdr.leftmost_ptr != NULL)
//       {
//         // The current page is internal
//         p = (page *)p->linear_search(min);
//       }
//       else
//       {
//         // Found a leaf
//         p->linear_search_range(min, max, values, size);

//         break;
//       }
//     }
//   }

//   int btree::btree_search_range(entry_key_t start_key, uint32_t to_scan, std::pair<entry_key_t, entry_key_t> *&res)
//   {
//     int elems_cnt = 0;
//     page *p = (page *)root;
//     while (p)
//     {
//       if (p->hdr.leftmost_ptr != NULL)
//       {
//         // The current page is internal
//         p = (page *)p->linear_search(start_key);
//       }
//       else
//       {
//         // Found a leaf
//         elems_cnt = p->linear_search_range(start_key, UINT64_MAX, to_scan, res);
//         break;
//       }
//     }
//     return elems_cnt;
//   }

//   void btree::printAll()
//   {
//     int total_keys = 0;
//     page *leftmost = (page *)root;
//     printf("root: %p\n", root);
//     if (root)
//     {
//       do
//       {
//         page *sibling = leftmost;
//         while (sibling)
//         {
//           if (sibling->hdr.level == 0)
//           {
//             total_keys += sibling->hdr.last_index + 1;
//           }
//           sibling->print();
//           sibling = sibling->hdr.sibling_ptr;
//         }
//         printf("-----------------------------------------\n");
//         leftmost = leftmost->hdr.leftmost_ptr;
//       } while (leftmost);
//     }

//     printf("total number of keys: %d\n", total_keys);
//   }

//   void btree::CalculateSapce(uint64_t &space)
//   {
//     if (root != nullptr)
//     {
//       ((page *)root)->CalculateSapce(space);
//     }
//   }

//   void btree::PrintInfo()
//   {
//     // printf("This is a b+ tree.\n");
//     // printf("Node size is %lu, M path is %d.\n", sizeof(page), cardinality);
//     // printf("Tree height is %d.\n", height);
//     size_t space = 0;
//     CalculateSapce(space);
//     printf("fast&fair tree size is %f GB.\n", (double)space / 1024.0 / 1024.0 / 1024.0);
//   }
// } // namespace FastFair