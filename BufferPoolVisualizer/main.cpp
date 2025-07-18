#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "FileEventBridge.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    // 1) Creamos el bridge que va a “tail-ear” events.log
    FileEventBridge bridge;
    QQmlApplicationEngine engine;

    // 2) Exponemos el bridge a QML
    engine.rootContext()->setContextProperty("bridge", &bridge);

    // 3) Arrancamos tu DBPlusPlus.exe en su propia consola
    //    Rutas directas: cámbialas a lo que corresponda en tu disco.
    QString exePath = "D:\\DBPlusPlus\\x64\\Debug\\DBPlusPlus.exe";
    QString workingDir = "D:\\DBPlusPlus\\DBPlusPlus";  // carpeta donde está events.log
    bridge.startConsole(exePath, workingDir);

    // 4) Cargamos la interfaz QML
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
