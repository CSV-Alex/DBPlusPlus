#pragma once
#include <QObject>
#include <QFile>
#include <QProcess>
#include <QTimer>

class FileEventBridge : public QObject {
    Q_OBJECT
public:
    explicit FileEventBridge(QObject* parent = nullptr);

    // estos 2 métodos pasan a ser invocables desde QML
    Q_INVOKABLE void startConsole(const QString& exePath,
        const QString& workingDir = QString());
    Q_INVOKABLE void readStatus();
    Q_INVOKABLE void sendInput(const QString& cmd);

    // signal para notificar a QML que llegó un nuevo bloque #STATUS
    Q_SIGNAL void statusUpdated(const QStringList& rows);

private Q_SLOTS:
    void checkFile();

private:
    QFile    m_file;
    QProcess m_proc;
    QTimer   m_timer;
    qint64   m_lastPos = 0;
};
