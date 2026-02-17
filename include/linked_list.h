#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LINKED_LIST_CHUNK_SIZE 100

typedef struct linked_list_node {
    void *data;
    size_t size;
    struct linked_list_node *next;
} linked_list_node_t;

typedef struct {
    linked_list_node_t *head;
    linked_list_node_t *tail;
    size_t count;
    size_t max_size;
    SemaphoreHandle_t mutex;
} linked_list_t;

bool linked_list_create(linked_list_t *list, size_t max_size);
bool linked_list_enqueue(linked_list_t *list, const void *data, size_t size);
void *linked_list_peek_head(linked_list_t *list, size_t *size);
void *linked_list_peek_tail(linked_list_t *list, size_t *size);
void *linked_list_pop_head(linked_list_t *list, size_t *size);
void *linked_list_pop_tail(linked_list_t *list, size_t *size);
void linked_list_destroy(linked_list_t *list);
size_t linked_list_count(linked_list_t *list);
