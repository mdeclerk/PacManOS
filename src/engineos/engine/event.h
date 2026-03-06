#pragma once

#include "engineos/include/event.h"

bool event_enqueue(const struct event *e);
bool event_dequeue(struct event *e);

void event_wait(void);