#ifndef RL_QUICK_CHATTER_CHATTER_H
#define RL_QUICK_CHATTER_CHATTER_H

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>

extern int xi_opcode;

void init(int id, const char* mappings);

#endif //RL_QUICK_CHATTER_CHATTER_H
