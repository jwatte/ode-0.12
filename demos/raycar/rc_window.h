
#if !defined(rc_window_h)
#define rc_window_h

#include <string>

void setupWindow(int width, int height, char const *title);
void renderWindow();
void pollInput();
void error(std::string const&arg);
void error(int error, std::string const &arg);
extern bool running;
extern bool active;

void resetTimer();
double timer();
void setTimer(double value);

#endif
