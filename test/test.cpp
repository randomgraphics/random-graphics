#include "rg/base.h"

void testLog() {
    RG_LOGI("printf style log %d", 1);
    RG_LOGI(s("c++ style log ") << 1 << 2);
    RG_LOG("main", I, "log with immediate controller name");
    auto ctrl = rg::log::Controller::getInstance("main");
    RG_LOG(ctrl, I, "log with external controller instance");
}

int main() {
    testLog();    
    return 0;
}