/*
 * <p>Title: array.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date 12/23/19 
 * @version 1.0
 */

#include "array.h"

static Array concat(Array a, Array b);

static let pop(Array a);

static int push(Array array, let item);

static void reverse(Array array);

static let shift(Array array);

static int unshift(Array array, let item);

static Array slice(Array array, int start, int end);

static void sort(Array array, SortBy sort);

static Array filter(Array array, FilterBy filter);

static let indexOf(Array array, int index);


Array concat(Array a, Array b) {
    Array array = createArray(0);
    ListNode node = a->head;
    a->tail->next = b->head;
    while (node != NULL) {
        array->push(array, node->item);
        node = node->next;
    }
    a->tail->next = NULL;
    return array;
}

let pop(Array array) {
    let item = NULL;
    if (array->length > 0) {
        array->length--;
        item = array->tail->item;
        ListNode node = array->tail;
        if (array->length == 0) {
            array->head = array->tail = NULL;
        }
        else {
            array->tail = array->tail->last;
            array->tail->next = NULL;
        }
        free(node);
    }
    return item;
}

int push(Array array, let item) {
    array->length++;
    ListNode node = newListNode();
    node->item = item;
    node->next = NULL;
    if (array->tail == NULL) {
        node->last = NULL;
        array->tail = node;
        array->head = node;
    }
    else {
        array->tail->next = node;
        node->last = array->tail;
        array->tail = node;
    }
    return array->length;
}

void reverse(Array array) {
    ListNode node = array->head;
    array->head = array->tail;
    array->tail = node;
    while (node != NULL) {
        ListNode temp = node->next;
        node->next = node->last;
        node->last = temp;
        node = node->last;
    }
}

let shift(Array array) {
    let item = NULL;
    if (array->length > 0) {
        ListNode node = array->head;
        array->length--;
        if (array->head->next != NULL) {
            array->head = array->head->next;
            array->head->last = NULL;
        }
        else {
            array->head = array->tail = NULL;
        }
        node->last = node->next = NULL;
        item = node->item;
        free(node);
    }
    return item;
}

int unshift(Array array, let item) {
    array->length++;
    ListNode node = newListNode();
    node->item = item;
    node->next = array->head;
    node->last = NULL;
    array->head->last = node;
    array->head = node;
    return array->length;
}

// 区别js的slice，js的选取区间为[start, end)，此处为[start, end]
Array slice(Array array, int start, int end) {
    Array sliceArray = createArray(0);

    // 验证区间合法性
    if ((start > array->length && end < -array->length)
            || ((start^end) > 0 && start >= end)
            || (start >= 0 && end < 0 && start >= end + array->length)
            || (start < 0 && end >= 0 && start + array->length >= end))
    {
        return sliceArray;
    }

    ListNode startNode, endNode;
    start = start >= 0 ? start : start + array->length;
    start = RANGE(0, array->length - 1, start);
    end = end >= 0 ? end : end + array->length;
    end = RANGE(0, array->length - 1, end);

    int half = array->length >> 1;
    if (start < half) {
        startNode = array->head;
        while (start-- != 0) {
            startNode = startNode->next;
        }
    }
    else {
        startNode = array->tail;
        while (start-- != 0) {
            startNode = startNode->last;
        }
    }

    if (end < half) {
        endNode = array->head;
        while (end-- != 0) {
            endNode = endNode->next;
        }
    }
    else {
        endNode = array->tail;
        while (end-- != 0) {
            endNode = endNode->last;
        }
    }

    while (startNode != endNode) {
        sliceArray->push(sliceArray, startNode->item);
        startNode = startNode->next;
    }
    sliceArray->push(sliceArray, endNode->item);
    return sliceArray;
}

void sort(Array array, SortBy sort) {
    if (array->length < 2) return;

    typedef struct stackItem {
        ListNode i;
        ListNode j;
        ListNode k;
        void * value;
    }*StackItem;

    Array stack = createArray(0);

    StackItem first = NEW(struct stackItem);
    first->i = array->head;
    first->j = array->tail;
    first->k = array->head;
    first->value = array->head->item;

    stack->push(stack, first);

    while (stack->length != 0) {
        StackItem stackItem = (StackItem)stack->pop(stack);
        ListNode i = stackItem->i;
        ListNode j = stackItem->j;
        void * k = stackItem->value;

        while (i != j) {
            while (sort(j->item, k) > 0 && i != j) {
                if (j->last == NULL) break;
                j = j->last;
            }
            i->item = j->item;

            while (sort(i->item, k) <= 0 && i != j) {
                if (i->next == NULL) break;
                i = i->next;
            }
            j->item = i->item;
        }
        i->item = k;

        if (i != stackItem->i && i->last != stackItem->i) {
            StackItem left = NEW(struct stackItem);
            left->i = stackItem->i;
            left->j = i->last;
            left->k = stackItem->i;
            left->value = left->k->item;

            stack->push(stack, left);
        }

        if (i != stackItem->j && i->next != stackItem->j) {
            StackItem right = NEW(struct stackItem);
            right->i = i->next;
            right->j = stackItem->j;
            right->k = i->next;
            right->value = right->k->item;

            stack->push(stack, right);
        }

        free(stackItem);
    }
}

Array filter(Array array, FilterBy filter) {
    Array result = createArray(0);
    ListNode node = array->head;
    while (node != NULL) {
        if (filter(node->item)) {
            result->push(result, node->item);
        }
        node = node->next;
    }
    return result;
}

let indexOf(Array array, int index) {
    // 索引位置与实际位置差1
    index++;
    if (index > 0 && index <= array->length) {
        ListNode node;
        if (index <= array->length / 2) {
            node = array->head;
            while (index-- > 1) {
                node = node->next;
            }
        }
        else {
            node = array->tail;
            index = array->length - index;
            while (index-- > 0) {
                node = node->last;
            }
        }
        return node->item;
    }
    return NULL;
}

Array createArray(int size) {
    Array array = newArray();
    array->pop = pop;
    array->sort = sort;
    array->push = push;
    array->shift = shift;
    array->slice = slice;
    array->concat = concat;
    array->filter = filter;
    array->reverse = reverse;
    array->unshift = unshift;
    array->indexOf = indexOf;
    array->head = array->tail = NULL;
    if (size > 0) {
        while (size-- > 0) {
            array->push(array, NULL);
        }
    }
    else {
        array->length = 0;
    }
    return array;
}

void freeArray(Array * array) {
    Array temp = *array;
    while (temp->length != 0) {
        temp->pop(temp);
    }
    free(*array);
    *array = NULL;
}

ArrayIterator createArrayIterator(Array array) {
    ArrayIterator iterator = newArrayIterator();
    iterator->count = 0;
    iterator->node = NULL;
    iterator->array = array;
    return iterator;
}

bool hasNextArrayIterator(ArrayIterator iterator) {
    return iterator->count < iterator->array->length ? true : false;
}

ArrayIterator nextArrayIterator(ArrayIterator iterator) {
    iterator->node = iterator->node == NULL ? 
            iterator->array->head : iterator->node->next;
    return iterator;
}

void freeArrayIterator(ArrayIterator * iterator) {
    free(*iterator);
    *iterator = NULL;
}
