#include <rg/opengl.h>

#ifdef _MSC_VER
#pragma warning(disable: 4127) // QT headers didn't use "if contexpr" when it should.
#endif
#include <QtWidgets/QtWidgets>
#include <QtWidgets/QApplication>
#include <QTimer>
#include <QOpenGLContext>

using namespace rg;
using namespace rg::opengl;

void render() {
    clearScreen(.0f, .5f, .5f);
}

int main(int argc, char ** argv) {
    // initialize Qt window
    QGuiApplication app(argc, argv);
    QWindow win;
    win.setSurfaceType(QSurface::OpenGLSurface);
    win.create();
    win.resize(1280, 720);

    // create QT opengl context
    QOpenGLContext rc;
    if (!rc.create()) {
        RG_LOGE("Failed to create GL context.");
        return -1;
    }
    if (!rc.makeCurrent(&win)) {
        RG_LOGE("Failed to make GL context current.");
        return -1;
    }

    if (!initExtensions()) return -1;

    // setup idle timer
    QTimer idleTimer;
    QObject::connect(&idleTimer, &QTimer::timeout, [&]{
        rc.makeCurrent(&win);
        render();
        rc.swapBuffers(&win);
    });
    idleTimer.start(0);

    // start the main event loop
    win.show();
    return app.exec();
}
