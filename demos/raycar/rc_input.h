
#if !defined(rc_input_h)
#define rc_input_h

enum InputKind
{
    ik_forward,
    ik_backward,
    ik_left,
    ik_right,
    ik_trigger,
    ik_start,
    ik_back,
    ik_endOfInputs
};

void clearInput();
void setInput(InputKind kind, bool on);
bool testInput(InputKind kind);
void setAnalogInput(bool available, float gas, float brake, float steer);
bool getAnalogInput(float &oGas, float &oBrake, float &oSteer);

#endif  //  rc_input_h
