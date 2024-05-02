#ifndef SIGNALING_H
#define SIGNALING_H

typedef struct Signal Signal;

Signal *signal_initialize();
void signal_destroy(Signal *signal);
void signal(Signal *signal);
void signal_broadcast(Signal *signal);
void signal_wait(Signal *signal);
void signal_block(Signal *signal);
void signal_unblock(Signal *signal);

#endif