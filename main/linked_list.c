#include "linked_list.h"
#include "esp_heap_caps.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "linked_list";

bool linked_list_create(linked_list_t *list, size_t max_size)
{
    if (list == NULL || max_size == 0)
    {
        return false;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->max_size = max_size;

    list->mutex = xSemaphoreCreateMutex();
    if (list->mutex == NULL)
    {
        return false;
    }

    return true;
}

bool linked_list_enqueue(linked_list_t *list, const void *data, size_t size)
{
    if (list == NULL || data == NULL || size == 0)
    {
        return false;
    }

    if (xSemaphoreTake(list->mutex, portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    if (list->count >= list->max_size)
    {
        ESP_LOGW(TAG, "Linked list is full, cannot enqueue (count=%u, max_size=%u)", 
                 (unsigned int)list->count, (unsigned int)list->max_size);
        xSemaphoreGive(list->mutex);
        return false;
    }

    linked_list_node_t *node = heap_caps_malloc(sizeof(linked_list_node_t), MALLOC_CAP_SPIRAM);
    if (node == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate node");
        xSemaphoreGive(list->mutex);
        return false;
    }

    node->data = (void *)data;
    node->size = size;
    node->next = NULL;

    if (list->tail == NULL)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
    }

    list->count++;

    xSemaphoreGive(list->mutex);
    return true;
}

void *linked_list_peek_head(linked_list_t *list, size_t *size)
{
    if (list == NULL || size == NULL)
    {
        return NULL;
    }

    if (xSemaphoreTake(list->mutex, portMAX_DELAY) != pdTRUE)
    {
        return NULL;
    }

    if (list->count == 0)
    {
        xSemaphoreGive(list->mutex);
        return NULL;
    }

    *size = list->head->size;
    void *data = list->head->data;

    xSemaphoreGive(list->mutex);
    return data;
}

void *linked_list_peek_tail(linked_list_t *list, size_t *size)
{
    if (list == NULL || size == NULL)
    {
        return NULL;
    }

    if (xSemaphoreTake(list->mutex, portMAX_DELAY) != pdTRUE)
    {
        return NULL;
    }

    if (list->count == 0)
    {
        xSemaphoreGive(list->mutex);
        return NULL;
    }

    *size = list->tail->size;
    void *data = list->tail->data;

    xSemaphoreGive(list->mutex);
    return data;
}

void *linked_list_pop_head(linked_list_t *list, size_t *size)
{
    if (list == NULL || size == NULL)
    {
        return NULL;
    }

    if (xSemaphoreTake(list->mutex, portMAX_DELAY) != pdTRUE)
    {
        return NULL;
    }

    if (list->count == 0)
    {
        xSemaphoreGive(list->mutex);
        return NULL;
    }

    linked_list_node_t *node = list->head;
    void *data = node->data;
    *size = node->size;

    list->head = node->next;
    if (list->head == NULL)
    {
        list->tail = NULL;
    }

    list->count--;

    heap_caps_free(node);

    xSemaphoreGive(list->mutex);
    return data;
}

void *linked_list_pop_tail(linked_list_t *list, size_t *size)
{
    if (list == NULL || size == NULL)
    {
        return NULL;
    }

    if (xSemaphoreTake(list->mutex, portMAX_DELAY) != pdTRUE)
    {
        return NULL;
    }

    if (list->count == 0)
    {
        xSemaphoreGive(list->mutex);
        return NULL;
    }

    linked_list_node_t *node = list->tail;
    void *data = node->data;
    *size = node->size;

    if (list->head == list->tail)
    {
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        linked_list_node_t *current = list->head;
        while (current->next != node)
        {
            current = current->next;
        }
        current->next = NULL;
        list->tail = current;
    }

    list->count--;

    heap_caps_free(node);

    xSemaphoreGive(list->mutex);
    return data;
}

void linked_list_destroy(linked_list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    while (list->count > 0)
    {
        size_t size;
        linked_list_pop_head(list, &size);
    }

    if (list->mutex != NULL)
    {
        vSemaphoreDelete(list->mutex);
        list->mutex = NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->max_size = 0;
}

size_t linked_list_count(linked_list_t *list)
{
    if (list == NULL)
    {
        return 0;
    }

    size_t count = 0;

    if (xSemaphoreTake(list->mutex, portMAX_DELAY) == pdTRUE)
    {
        count = list->count;
        xSemaphoreGive(list->mutex);
    }

    return count;
}
