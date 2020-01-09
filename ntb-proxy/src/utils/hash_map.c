/*
 * <p>Title: hash_map.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date 12/23/19 
 * @version 1.0
 */

#include "hash_map.h"

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





int defaultHashCode(HashMap hashMap, let key) {
    char * k = (char *)key;
    unsigned long h = 0;
    while (*k) {
        h = (h << 4) + *k++;
        unsigned long g = h & 0xF0000000L;
        if (g) {
            h ^= g >> 24;
        }
        h &= ~g;
    }
    return h % hashMap->listSize;
}

bool defaultEqual(let key1, let key2) {
    return strcmp((string)key1, (string)key2) ? false : true;
}

void resetHashMap(HashMap hashMap, int listSize) {

    if (listSize < 8) return;

    // 键值对临时存储空间
    Entry tempList = newEntryList(hashMap->size);

    HashMapIterator iterator = createHashMapIterator(hashMap);
    int length = hashMap->size;
    for (int index = 0; hasNextHashMapIterator(iterator); index++) {
        // 迭代取出所有键值对
        iterator = nextHashMapIterator(iterator);
        tempList[index].key = iterator->entry->key;
        tempList[index].value = iterator->entry->value;
        tempList[index].next = NULL;
    }
    freeHashMapIterator(&iterator);

    // 清除原有键值对数据
    hashMap->size = 0;
    for (int i = 0; i < hashMap->listSize; i++) {
        Entry current = &hashMap->list[i];
        current->key = NULL;
        current->value = NULL;
        if (current->next != NULL) {
            while (current->next != NULL) {
                Entry temp = current->next->next;
                free(current->next);
                current->next = temp;
            }
        }
    }

    // 更改内存大小
    hashMap->listSize = listSize;
    Entry relist = (Entry)realloc(hashMap->list, hashMap->listSize * sizeof(struct entry));
    if (relist != NULL) {
        hashMap->list = relist;
        relist = NULL;
    }

    // 初始化数据
    for (int i = 0; i < hashMap->listSize; i++) {
        hashMap->list[i].key = NULL;
        hashMap->list[i].value = NULL;
        hashMap->list[i].next = NULL;
    }

    // 将所有键值对重新写入内存
    for (int i = 0; i < length; i++) {
//        Array x = tempList[i].value;
        hashMap->put(hashMap, tempList[i].key, tempList[i].value);
    }
    free(tempList);
}

void defaultPut(HashMap hashMap, let key, let value) {
    int index = hashMap->hashCode(hashMap, key);
    if (hashMap->list[index].key == NULL) {
        hashMap->size++;
        // 该地址为空时直接存储
        hashMap->list[index].key = key;
        hashMap->list[index].value = value;
    }
    else {

        Entry current = &hashMap->list[index];
        while (current != NULL) {
            if (hashMap->equal(key, current->key)) {
                // 对于键值已经存在的直接覆盖
                hashMap->list[index].value = value;
                return;
            }
            current = current->next;
        };

        // 发生冲突则创建节点挂到相应位置的next上
        Entry entry = newEntry();
        entry->key = key;
        entry->value = value;
        entry->next = hashMap->list[index].next;
        hashMap->list[index].next = entry;
        hashMap->size++;
    }

    if (hashMap->autoAssign && hashMap->size >= hashMap->listSize) {
        // 内存扩充至原来的两倍
        // *注: 扩充时考虑的是当前存储元素数量与存储空间的大小关系，而不是存储空间是否已经存满，
        // 例如: 存储空间为10，存入了10个键值对，但是全部冲突了，所以存储空间空着9个，其余的全部挂在一个上面，
        // 这样检索的时候和遍历查询没有什么区别了，可以简单这样理解，当我存入第11个键值对的时候一定会发生冲突，
        // 这是由哈希函数本身的特性(取模)决定的，冲突就会导致检索变慢，所以这时候扩充存储空间，对原有键值对进行
        // 再次散列，会把冲突的数据再次分散开，加快索引定位速度。
        resetHashMap(hashMap, hashMap->listSize * 2);
    }
}

let defaultGet(HashMap hashMap, let key) {
    if (hashMap->exists(hashMap, key)) {
        int index = hashMap->hashCode(hashMap, key);
        Entry entry = &hashMap->list[index];
        while (entry != NULL) {
            if (hashMap->equal(entry->key, key)) {
                return entry->value;
            }
            entry = entry->next;
        }
    }
    return NULL;
}

let defaultRemove(HashMap hashMap, let key) {
    int index = hashMap->hashCode(hashMap, key);
    Entry entry = &hashMap->list[index];
    if (entry->key == NULL) {
        return NULL;
    }
    let entryKey = entry->key;
    bool result = false;
    if (hashMap->equal(entry->key, key)) {
        hashMap->size--;
        if (entry->next != NULL) {
            Entry temp = entry->next;
            entry->key = temp->key;
            entry->value = temp->value;
            entry->next = temp->next;
            free(temp);
        }
        else {
            entry->key = NULL;
            entry->value = NULL;
        }
        result = true;
    }
    else {
        Entry p = entry;
        entry = entry->next;
        while (entry != NULL) {
            if (hashMap->equal(entry->key, key)) {
                hashMap->size--;
                p->next = entry->next;
                free(entry);
                result = true;
                break;
            }
            p = entry;
            entry = entry->next;
        };
    }

    // 如果空间占用不足一半，则释放多余内存
    if (result && hashMap->autoAssign &&  hashMap->size < hashMap->listSize / 2) {
        resetHashMap(hashMap, hashMap->listSize / 2);
    }
    return entryKey;
}

bool defaultExists(HashMap hashMap, let key) {
    int index = hashMap->hashCode(hashMap, key);
    Entry entry = &hashMap->list[index];
    if (entry->key == NULL) {
        return false;
    }
    else {
        while (entry != NULL) {
            if (hashMap->equal(entry->key, key)) {
                return true;
            }
            entry = entry->next;
        }
        return false;
    }
}

void defaultClear(HashMap hashMap) {
    for (int i = 0; i < hashMap->listSize; i++) {
        // 释放冲突值内存
        Entry entry = hashMap->list[i].next;
        while (entry != NULL) {
            Entry next = entry->next;
            free(entry);
            entry = next;
        }
        hashMap->list[i].next = NULL;
    }
    // 释放存储空间
    free(hashMap->list);
    hashMap->list = NULL;
    hashMap->size = -1;
    hashMap->listSize = 0;
}

HashMap createHashMap(HashCode hashCode, Equal equal) {
    HashMap hashMap = newHashMap();
    hashMap->size = 0;
    hashMap->listSize = 8;
    hashMap->hashCode = hashCode == NULL ? defaultHashCode : hashCode;
    hashMap->equal = equal == NULL ? defaultEqual : equal;
    hashMap->exists = defaultExists;
    hashMap->get = defaultGet;
    hashMap->put = defaultPut;
    hashMap->remove = defaultRemove;
    hashMap->clear = defaultClear;
    hashMap->autoAssign = true;

    // 起始分配8个内存空间，溢出时会自动扩充
    hashMap->list = newEntryList(hashMap->listSize);
    Entry p = hashMap->list;
    for (int i = 0; i < hashMap->listSize; i++) {
        p[i].key = p[i].value = p[i].next = NULL;
    }
    return hashMap;
}

HashMapIterator createHashMapIterator(HashMap hashMap) {
    HashMapIterator iterator = newHashMapIterator();
    iterator->hashMap = hashMap;
    iterator->count = 0;
    iterator->hashCode = -1;
    iterator->entry = NULL;
    return iterator;
}

bool hasNextHashMapIterator(HashMapIterator iterator) {
    return iterator->count < iterator->hashMap->size ? true : false;
}

HashMapIterator nextHashMapIterator(HashMapIterator iterator) {
    if (hasNextHashMapIterator(iterator)) {
        if (iterator->entry != NULL && iterator->entry->next != NULL) {
            iterator->count++;
            iterator->entry = iterator->entry->next;
            return iterator;
        }
        while (++iterator->hashCode < iterator->hashMap->listSize) {
            Entry entry = &iterator->hashMap->list[iterator->hashCode];
            if (entry->key != NULL) {
                iterator->count++;
                iterator->entry = entry;
                break;
            }
        }
    }
    return iterator;
}

void freeHashMapIterator(HashMapIterator * iterator) {
    free(*iterator);
    *iterator = NULL;
}
 
