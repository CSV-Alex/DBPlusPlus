#pragma once

#include <QObject>
#include <QProcess>

class EventBridge : public QObject {
    Q_OBJECT

public:
    explicit EventBridge(QObject* parent = nullptr);

    /** Lanza el .exe de consola y crea una nueva ventana de consola */
    Q_INVOKABLE void startConsole(const QString& exePath);

signals:
    void eventOccurred(const QString& evt, int pageId);

private slots:
    void handleStdout();
    void handleProcessStarted();
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus status);
    Q_INVOKABLE void sendInput(const QString& text);

private:
    QProcess proc;
};
