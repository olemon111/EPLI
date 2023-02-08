/**
 *
 * @file tree.h
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 *
 * TBD
 *
 * @section DESCRIPTION
 *
 * The tree class defines the methods of trees.
 */

#ifndef _BTREE_TREE_H
#define _BTREE_TREE_H
/* ---------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <thread>
#include <atomic>
#include <vector>
#include <bits/hash_bytes.h>

#include <immintrin.h>
/* ---------------------------------------------------------------------- */
/*                            Default Parameters                          */
/* ---------------------------------------------------------------------- */

// the size of a tree node
#define NONLEAF_LINE_NUM 4 // 256B
#define LEAF_LINE_NUM 4    // 256B

// the number of leaf nodes to prefetch ahead in jump pointer array
// prefetching
#ifndef PREFETCH_NUM_AHEAD
#define PREFETCH_NUM_AHEAD 3
#endif

/* ---------------------------------------------------------------------- */
/*                 Node Size, Key Size, and Pointer Size                  */
/* ---------------------------------------------------------------------- */

// node size
#define NONLEAF_SIZE (CACHE_LINE_SIZE * NONLEAF_LINE_NUM)
#define LEAF_SIZE (CACHE_LINE_SIZE * LEAF_LINE_NUM)

// key size and pointer size: 8B
typedef long long KEY_TYPE;
#define KEY_SIZE 8     /* size of a key in tree node */
#define POINTER_SIZE 8 /* size of a pointer/value in node */
#define ITEM_SIZE 8    /* key size or pointer size */

#define MAX_KEY ((KEY_TYPE)(0x7fffffffffffffffULL))
#define MIN_KEY ((KEY_TYPE)(0x8000000000000000ULL))

#include "keyinput.h"
#include "nodepref.h"
#include "mempool.h"
#include "nvm-common.h"
#include "performance.h"

/* ---------------------------------------------------------------------- */
/*                            Useful funcions                             */
/* ---------------------------------------------------------------------- */
/* GCC builtin functions

int __builtin_ffs (unsigned int x)
Returns one plus the index of the least significant 1-bit of x, or if x is
zero, returns zero.

int __builtin_popcount (unsigned int x)
Returns the number of 1-bits in x.

*/

#define bitScan(x) __builtin_ffs(x)
#define countBit(x) __builtin_popcount(x)

extern size_t key_size_;

static inline unsigned char hashcode1B(KEY_TYPE x)
{
#ifdef VAR_KEY
   return std::_Hash_bytes((char *)x, key_size_, 1) & 0xff;
#else
   x ^= x >> 32;
   x ^= x >> 16;
   x ^= x >> 8;
#endif
   return (unsigned char)(x & 0x0ffULL);
}

static inline unsigned long long rdtsc(void)
{
   unsigned hi, lo;
   __asm__ __volatile__("rdtsc"
                        : "=a"(lo), "=d"(hi));
   return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#define lbt_min(x, y) ((x) <= (y) ? (x) : (y))
#define lbt_max(x, y) ((x) <= (y) ? (y) : (x))

// compute ceiling(x/y) and floor(x/y)
#define ceiling(x, y) (((x) + (y)-1) / (y))
#define floor(x, y) ((x) / (y))

#define swap(x, y)   \
   do                \
   {                 \
      auto _t = (x); \
      (x) = (y);     \
      (y) = _t;      \
   } while (0)

/* ---------------------------------------------------------------------- */
class tree
{
public:
   /**
    * bulkload a tree
    *
    * @param keynum   number of keys to bulkload
    * @param input    the keyInput structure that contains the keys
    * @param bfill    the fill factor, which is a float in (0,1]
    * @return         the number of tree levels
    */
   virtual int bulkload(int keynum, keyInput *input, float bfill)
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
      return 0;
   }

   /**
    * randomize the key orders in nodes (only for unsorted/bitmap trees)
    */
   virtual void randomize() {}

   /**
    * given a search key, perform the search operation
    *
    * @param key   the search key
    * @param pos   the position to return
    * @return the leaf node to return
    *
    * If a match to the given search key is found, then the leaf node and the
    * matching key position is returned.  If a match is not found, then the leaf
    * node and the position to a previous key is returned.
    */
   virtual void *lookup(KEY_TYPE key, int *pos)
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
      return NULL;
   }

   /**
    * obtain the record pointer
    *
    * @param p     leaf node pointer
    * @param pos   index entry position in the leaf node
    * @return      the associated record pointer
    */
   virtual void *get_recptr(void *p, int pos)
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
      return NULL;
   }

   /**
    * insert an index entry
    *
    * @param key   the index key
    * @param ptr   the record pointer
    */
   virtual void insert(KEY_TYPE key, void *ptr)
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
   }

   /**
    * delete an index entry
    *
    * @param key   the index key to delete
    */
   virtual void del(KEY_TYPE key)
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
   }

   /**
    * range scan (Baotong's version)
    *
    * @param key   the index key to begin scan
    * @param to_scan number of records to scan
    * @param result array to store scan results
    *
    */
   virtual int range_scan_by_size(const KEY_TYPE &key, uint32_t to_scan, char *result)
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
      return 0;
   }

   virtual int rangeScan(KEY_TYPE key, uint32_t scan_size, char *result)
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
      return 0;
   }

   /**
    * print the tree structure
    */
   virtual void print()
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
   }

   /**
    * check the correctness of the tree structure
    *
    * @param start   the start key of the tree
    * @param end     the end key of the tree
    */
   virtual void check(KEY_TYPE *start, KEY_TYPE *end)
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
   }

   /**
    * obtain the level parameter of the tree
    */
   virtual int level()
   {
      fprintf(stderr, "Not implemented!\n");
      exit(1);
      return 0;
   }

}; // tree

/* ---------------------------------------------------------------------- */
extern tree *the_treep;
extern int worker_thread_num;
extern const char *nvm_file_name;

extern int parse_command(int argc, char **argv);

#ifdef INSTRUMENT_INSERTION
extern int insert_total;         // insert_total=
extern int insert_no_split;      //              insert_no_split
extern int insert_leaf_split;    //             +insert_leaf_split
extern int insert_nonleaf_split; //            +insert_nonleaf_split
extern int total_node_splits;    // an insertion may cause multiple node splits
#endif                           // INSTRUMENT_INSERTION

// a specific tree should implement a child class of tree
// and the following function
extern tree *initTree(void *nvm_addr, bool recover);

/* ---------------------------------------------------------------------- */
#endif /* _BTREE_TREE_H */
