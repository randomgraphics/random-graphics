#include "rg/opengl.h"

using namespace rg;
using namespace rg::gl;

#ifdef _MSC_VER
#pragma warning(disable: 4127) // QT headers didn't use "if contexpr" when it should.
#endif
#include <QtWidgets/QtWidgets>
#include <QtWidgets/QApplication>

void render(RenderContext & rc) {
    clearScreen();
    rc.swapBuffers();
}

int main(int argc, char ** argv) {
    // initialize Qt window
    QGuiApplication app(argc, argv);
    QWindow win;
    win.resize(1280, 720);

    // Initialize OpenGL
    if (gladLoadGL()) return -1;
#if RG_MSWIN
    if (!gladLoadWGL(::GetDC((HWND)win.winId()))) return -1;
#endif

    RenderContext::CreationParameters cp;
    cp.window = (RenderContext::WindowHandle)win.winId();
    RenderContext rc(cp);

    bool running = true;
    win.show();
    while(running) {
        app.processEvents();
        render(rc);
    }
}
