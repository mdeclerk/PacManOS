#include "event.h"

#define EVENT_QUEUE_SIZE 64
#define EVENT_QUEUE_MASK (EVENT_QUEUE_SIZE - 1)

static struct {
    struct event events[EVENT_QUEUE_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} queue;

#define barrier() asm volatile("" ::: "memory")

bool event_enqueue(const struct event *e)
{
    uint32_t head = queue.head;

    if ((head - queue.tail) >= EVENT_QUEUE_SIZE)
        return false;

    queue.events[head & EVENT_QUEUE_MASK] = *e;
    barrier();
    queue.head = head + 1;

    return true;
}

bool event_dequeue(struct event *e)
{
    uint32_t tail = queue.tail;

    if (queue.head == tail)
        return false;

    *e = queue.events[tail & EVENT_QUEUE_MASK];
    barrier();
    queue.tail = tail + 1;

    return true;
}

void event_wait(void) 
{
    asm volatile("hlt");
}
