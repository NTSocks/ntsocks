/*
 * <p>Title: hash_map.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date 12/23/19 
 * @version 1.0
 */

#ifndef SHM_RING_BUFFER_HASH_MAP_H
#define SHM_RING_BUFFER_HASH_MAP_H

#include "data_struct.h"
#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct entry {
    let key;        // key
    let value;      // value
    struct entry * next;    // conflicted linked table
} *Entry;

#define newEntry() NEW(struct entry)
#define newEntryList(length) (Entry)malloc(length * sizeof(struct entry))

// define hashmap structure
typedef struct hashMap *HashMap;

#define newHashMap() NEW(struct hashMap)

// hash function prototype
typedef int(*HashCode)(HashMap, let key);

// judge equal function prototype
typedef bool(*Equal)(let key1, let key2);

// add element
typedef void(*Put)(HashMap hashMap, let key, let value);

// get element
typedef let(*Get)(HashMap hashMap, let key);

// remove element
typedef let(*Remove)(HashMap hashMap, let key);

// clear hashmap
typedef void(*Clear)(HashMap hashMap);

// judge if the specified key exists
typedef bool(*Exists)(HashMap hashMap, let key);

typedef struct hashMap {
    int size;
    int listSize;
    HashCode hashCode;
    Equal equal;
    Entry list; // storage location
    Put put;
    Get get;
    Remove remove;
    Clear clear;
    Exists exists;
    bool autoAssign;    // set if dynamic adjust the memory size
                        // according to current data size, default true
} *HashMap;

// Iterator Structure
typedef struct hashMapIterator {
    Entry entry;            // the location pointed by iterator
    int count;              // iterate times
    int hashCode;           // hash value of key-value pair
    HashMap hashMap;
}*HashMapIterator;

#define newHashMapIterator() NEW(struct hashMapIterator)

// 默认哈希函数
static int defaultHashCode(HashMap hashMap, let key);

// 默认判断键值是否相等
static bool defaultEqual(let key1, let key2);

// 默认添加键值对
static void defaultPut(HashMap hashMap, let key, let value);

// 默认获取键对应值
static let defaultGet(HashMap hashMap, let key);

// 默认删除键
static let defaultRemove(HashMap hashMap, let key);

// 默认判断键是否存在
static bool defaultExists(HashMap hashMap, let key);

// 默认清空Map
static void defaultClear(HashMap hashMap);

// 重新构建
static void resetHashMap(HashMap hashMap, int listSize);

// 创建一个哈希结构
HashMap createHashMap(HashCode hashCode, Equal equal);

// 创建哈希结构迭代器
HashMapIterator createHashMapIterator(HashMap hashMap);

// 迭代器是否有下一个
bool hasNextHashMapIterator(HashMapIterator iterator);

// 迭代到下一次
HashMapIterator nextHashMapIterator(HashMapIterator iterator);

// 释放迭代器内存
void freeHashMapIterator(HashMapIterator * iterator);





#ifdef __cplusplus
};
#endif


#endif //SHM_RING_BUFFER_HASH_MAP_H
