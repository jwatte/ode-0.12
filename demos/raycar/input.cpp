
#include "rc_input.h"
#include <string.h>

static bool inputs[ik_endOfInputs];
static bool hasAnalog;
static float gGas;
static float gBrake;
static float gSteer;

void clearInput()
{
    memset(inputs, 0, sizeof(inputs));
    hasAnalog = false;
}

void setInput(InputKind kind, bool on)
{
    inputs[kind] = on;
}

bool testInput(InputKind kind)
{
    return inputs[kind];
}

void setAnalogInput(bool available, float gas, float brake, float steer)
{
    gGas = gas;
    gBrake = brake;
    gSteer = steer;
    hasAnalog = available;
}

bool getAnalogInput(float &gas, float &brake, float &steer)
{
    gas = gGas;
    brake = gBrake;
    steer = gSteer;
    return hasAnalog;
}

