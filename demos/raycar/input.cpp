
#include "rc_input.h"
#include <string.h>

bool inputs[ik_endOfInputs];

void clearInput()
{
    memset(inputs, 0, sizeof(inputs));
}

void setInput(InputKind kind, bool on)
{
    inputs[kind] = on;
}

bool testInput(InputKind kind)
{
    return inputs[kind];
}

