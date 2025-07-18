#include "EventBridge.h"
#include <QDebug>
#include <windows.h>  // para CREATE_NEW_CONSOLE

EventBridge::EventBridge(QObject* parent)
    : QObject(parent)
{
    // cuando haya datos en stdout:
    connect(&proc, &QProcess::readyReadStandardOutput,
        this, &EventBridge::handleStdout);

    // debug: proceso arrancó
    connect(&proc, &QProcess::started,
        this, &EventBridge::handleProcessStarted);

    // debug: error en el proceso
    connect(&proc, &QProcess::errorOccurred,
        this, &EventBridge::handleProcessError);

    // debug: proceso terminó
    connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, &EventBridge::handleProcessFinished);
}

void EventBridge::startConsole(const QString& exePath)
{
    qDebug() << "[EventBridge] Starting console with new window:" << exePath;

    // Pedir una nueva ventana de consola
    proc.setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments* args) {
#ifdef Q_OS_WIN
            args->flags |= CREATE_NEW_CONSOLE;
            // dejamos stdin/stdout heredados para que funcione cin/cout
            args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
#endif
        }
    );

    // Que proc capture sólo stdout (para tus [EVENT]…)
    proc.setProcessChannelMode(QProcess::MergedChannels);

    // Opcional: si tu exe necesita un working dir concreto:
    proc.setWorkingDirectory("D:\\DBPlusPlus\\DBPlusPlus");

    // Finalmente arranca
    proc.start(exePath);

    if (!proc.waitForStarted(3000)) {
        qWarning() << "[EventBridge] ¡No arrancó la consola!";
    }
}

void EventBridge::handleStdout()
{
    while (proc.canReadLine()) {
        auto line = proc.readLine().trimmed();
        qDebug() << "[EventBridge] STDOUT:" << line;
        if (!line.startsWith("[EVENT]")) continue;
        auto parts = QString::fromUtf8(line).split(' ');
        if (parts.size() == 3) {
            emit eventOccurred(parts[1], parts[2].toInt());
        }
    }
}

void EventBridge::handleProcessStarted()
{
    qDebug() << "[EventBridge] Process started (PID =" << proc.processId() << ")";
}

void EventBridge::handleProcessError(QProcess::ProcessError error)
{
    qWarning() << "[EventBridge] Process error:" << error
        << "-" << proc.errorString();
}

void EventBridge::handleProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    qDebug() << "[EventBridge] Process finished with code"
        << exitCode << ", status =" << status;
}

void EventBridge::sendInput(const QString& text)
{
    if (proc.state() == QProcess::Running) {
        qDebug() << "[EventBridge] Writing to stdin:" << text.trimmed();
        proc.write(text.toUtf8());
    }
}