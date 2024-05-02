#include "../include/signaling.h"
#include <stdlib.h>
#include <threads.h>

struct Signal {
    mtx_t mutex;
    cnd_t cnd;
};

Signal *signal_initialize() {
    Signal *signal = malloc(sizeof(Signal));

    mtx_init(&signal->mutex, mtx_plain);
    cnd_init(&signal->cnd);

    return signal;
}

void signal_destroy(Signal *signal) {
    mtx_destroy(&signal->mutex);
    cnd_destroy(&signal->cnd);

    free(signal);
}

void signal(Signal *signal) {
    cnd_signal(&signal->cnd);
}

void signal_broadcast(Signal *signal) {
    cnd_broadcast(&signal->cnd);
}

void signal_wait(Signal *signal) {
    cnd_wait(&signal->cnd, &signal->mutex);
}

void signal_block(Signal *signal) {
    mtx_lock(&signal->mutex);
}

void signal_unblock(Signal *signal) {
    mtx_unlock(&signal->mutex);
}