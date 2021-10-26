#ifndef SHM_RING_BUFFER_ARRAY_H
#define SHM_RING_BUFFER_ARRAY_H

#include "data_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list {
    let item;
    struct list * next;
    struct list * last;
}*ListNode, *List;

#define newList() NEW(struct list)
#define newListNode() NEW(struct list)

typedef struct array *Array;

typedef Array(*Concat)(Array a, Array b);

typedef let (*Pop)(Array array);

typedef int(*Push)(Array array, let item);

typedef void(*Reverse)(Array array);

typedef let (*Shift)(Array array);

typedef int (*Unshift)(Array array, let item);

typedef Array(*Slice)(Array array, int start, int end);

typedef int(*SortBy)(let item1, let item2);

typedef void(*Sort)(Array array, SortBy sort);

typedef bool(*FilterBy)(let item);

typedef Array(*Filter)(Array array, FilterBy filter);

typedef let (*IndexOf)(Array array, int index);

typedef struct array {
    Pop pop;
    List head;
    List tail;
    Sort sort;
    Push push;
    int length;
    Shift shift;
    Slice slice;
    Filter filter;
    Concat concat;
    Reverse reverse;
    Unshift unshift;
    IndexOf indexOf;
}*Array;

#define newArray() NEW(struct array)

typedef struct arrayIterator {
    ListNode node;
    int count;
    Array array;
}*ArrayIterator;

#define newArrayIterator() NEW(struct arrayIterator)

Array createArray(int size);

void freeArray(Array * array);

ArrayIterator createArrayIterator(Array array);

bool hasNextArrayIterator(ArrayIterator iterator);

ArrayIterator nextArrayIterator(ArrayIterator iterator);

void freeArrayIterator(ArrayIterator * iterator);

#ifdef __cplusplus
};
#endif

#endif //SHM_RING_BUFFER_ARRAY_H
