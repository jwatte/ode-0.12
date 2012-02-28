
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

#endif  //  rc_input_h
