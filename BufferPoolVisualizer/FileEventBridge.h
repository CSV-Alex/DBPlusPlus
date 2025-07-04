#pragma once

#include <QObject>
#include <QFile>
#include <QProcess>
#include <QTimer>

class FileEventBridge : public QObject
{
    Q_OBJECT
public:
    explicit FileEventBridge(QObject* parent = nullptr);

    /** Arranca tu DBPlusPlus.exe en nueva consola y deja stdin/stdout heredados */
    Q_INVOKABLE void startConsole(const QString& exePath,
        const QString& workingDir = QString());

signals:
    /// Emite cada vez que hay un nuevo bloque "#STATUS" parseado
    void statusUpdated(const QStringList& rows);

private slots:
    void checkFile();

private:
    QProcess  m_proc;
    QFile     m_file;
    qint64    m_lastPos = 0;
    QTimer    m_timer;
};
