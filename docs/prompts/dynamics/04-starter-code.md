Title: Starter Code — Bounded Queue & Signal Snippets (Implementation Prompt)

This file contains small, safe starter snippets to include in the first PRs. They favor simplicity and memory-safety (mutex + condition variable) for the initial implementation.

Bounded queue header (`src/common/queue.h`) — suggested API

```c
#ifndef COMMON_QUEUE_H
#define COMMON_QUEUE_H

#include <stddef.h>

typedef struct bqueue bqueue_t;

bqueue_t *bqueue_create(size_t capacity);
void bqueue_destroy(bqueue_t *q);
int bqueue_push(bqueue_t *q, void *item); /* returns 0 on success, -1 if closed */
void *bqueue_pop(bqueue_t *q); /* returns NULL if closed and empty */
void bqueue_close(bqueue_t *q);

#endif /* COMMON_QUEUE_H */
```

Simple implementation sketch (`src/common/queue.c`) — use pthreads

```c
#include "common/queue.h"
#include <stdlib.h>
#include <pthread.h>

struct bqueue {
    void **buf;
    size_t cap;
    size_t head, tail;
    int closed;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

bqueue_t *bqueue_create(size_t capacity) {
    bqueue_t *q = calloc(1, sizeof(*q));
    /* allocate capacity+1 internally so the API 'capacity' equals
       the effective number of storable items (avoids off-by-one semantics) */
    q->buf = calloc(capacity + 1, sizeof(void*));
    q->cap = capacity + 1;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

void bqueue_destroy(bqueue_t *q) {
    if (!q) return;
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    free(q->buf);
    free(q);
}

int bqueue_push(bqueue_t *q, void *item) {
    pthread_mutex_lock(&q->lock);
    while (!q->closed && ((q->tail + 1) % q->cap) == q->head) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    if (q->closed) {
        pthread_mutex_unlock(&q->lock);
        return -1;
    }
    q->buf[q->tail] = item;
    q->tail = (q->tail + 1) % q->cap;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

void *bqueue_pop(bqueue_t *q) {
    pthread_mutex_lock(&q->lock);
    while (!q->closed && q->head == q->tail) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    if (q->closed && q->head == q->tail) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    void *it = q->buf[q->head];
    q->head = (q->head + 1) % q->cap;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return it;
}

void bqueue_close(bqueue_t *q) {
    pthread_mutex_lock(&q->lock);
    q->closed = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}
```

Signal struct sketch (in `src/core/signal.h`)

```c
typedef struct {
    uint64_t id;
    uint32_t type;
    uint64_t timestamp_ns;
    uint8_t payload[]; /* flexible payload */
} signal_t;
```

Notes:
- These snippets intentionally favor clarity and safety. Replace or optimize (lock-free) only after profiling.
- Capacity semantics: the ring-buffer idiom requires an extra slot; the implementation above allocates `capacity + 1` so the external `capacity` parameter reflects the caller's requested effective capacity.
- Ownership semantics: define clear ownership rules. Recommended policy:
    - `bqueue_push()` transfers ownership of the `item` to the queue. After push returns successfully the caller MUST NOT free the item.
    - `bqueue_pop()` returns ownership to the consumer; the consumer is responsible for freeing the item when finished.
    - If producers prefer not to transfer ownership, provide a `bqueue_push_copy()` helper that copies items into queue-managed memory.
