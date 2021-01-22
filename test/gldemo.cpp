#ifdef _MSC_VER
#pragma warning(disable: 4127) // QT headers didn't use "if contexpr" when it should.
#endif
#include <QtCore/QtCore>
#include <rg/opengl.h>
#include <QtWidgets/QtWidgets>
#include <QtWidgets/QApplication>
#include <QTimer>

using namespace rg;
using namespace rg::gl;

void render(RenderContext & rc) {
    clearScreen();
    rc.swapBuffers();
}

int main(int argc, char ** argv) {
    // initialize Qt window
    QGuiApplication app(argc, argv);
    QWindow win;
    win.resize(1280, 720);

    // create context
    RenderContext::CreationParameters cp;
    cp.window = (RenderContext::WindowHandle)win.winId();
    RenderContext rc(cp);
    if (!rc.good()) return -1;
    rc.makeCurrent();

    // setup idle timer
    QTimer idleTimer;
    QObject::connect(&idleTimer, &QTimer::timeout, [&]{ render(rc); });
    idleTimer.start(0);

    // start the main event loop
    win.show();
    return app.exec();
}
