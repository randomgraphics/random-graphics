#include "rg/base.h"

void testLog() {
    RG_LOG(I, "printf style log %d", 1);
    RG_LOG(E, ss("c++ style log ") << 1 << 2);
    RG_LOG(I, c("main"), "log with immediate controller name");
    auto ctrl = rg::log::Controller::getInstance("main");
    RG_LOG(I, ctrl, "log with external controller instance");
}

int main() {
    testLog();    
    return 0;
}