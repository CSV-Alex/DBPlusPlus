#include "FileEventBridge.h"
#include <QTextStream>
#include <QDebug>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

FileEventBridge::FileEventBridge(QObject* parent)
    : QObject(parent)
    , m_file("D:/DBPlusPlus/DBPlusPlus/events.log")
{
    qDebug() << "[FileEventBridge] Constructor: preparando watcher en events.log";

    // 1) Si no existe, créalo vacío
    if (!m_file.exists()) {
        qDebug() << "[FileEventBridge] El archivo no existía, creando.";
        m_file.open(QIODevice::WriteOnly);
        m_file.close();
    }

    // 2) Ábrelo en sólo lectura, ponte al final
    if (m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastPos = m_file.size();
        qDebug() << "[FileEventBridge] Abierto en lectura. Tamaño inicial =" << m_lastPos;
    }
    else {
        qWarning() << "[FileEventBridge] ¡No pude abrir events.log para lectura!";
    }

    // 3) Timer cada 200ms para hacer polling
    connect(&m_timer, &QTimer::timeout, this, &FileEventBridge::checkFile);
    m_timer.start(200);
    qDebug() << "[FileEventBridge] Timer iniciado (200ms).";
}

void FileEventBridge::startConsole(const QString& exePath, const QString& workingDir)
{
    qDebug() << "[FileEventBridge] startConsole() →" << exePath << "en dir" << workingDir;

    // 1) Forzar nueva consola
    m_proc.setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments* args) {
#ifdef Q_OS_WIN
            args->flags |= CREATE_NEW_CONSOLE;
            args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
#endif
        }
    );

    // 2) No necesitamos capturar stdout aquí, sólo lanzar
    m_proc.setProcessChannelMode(QProcess::MergedChannels);

    // 3) Working directory
    if (!workingDir.isEmpty()) {
        m_proc.setWorkingDirectory(workingDir);
    }

    // 4) Arrancar
    m_proc.start(exePath);
    if (!m_proc.waitForStarted(3000)) {
        qWarning() << "[FileEventBridge] ¡No arrancó la consola!";
    }
    else {
        qDebug() << "[FileEventBridge] Consola arrancada con PID =" << m_proc.processId();
    }
}

void FileEventBridge::readStatus()
{
    // simplemente dispara la comprobación de fichero
    checkFile();
}

void FileEventBridge::sendInput(const QString& cmd)
{
    if (m_proc.state() == QProcess::Running) {
        QByteArray ba = cmd.toLocal8Bit();
        m_proc.write(ba);
        qDebug() << "[FileEventBridge] Enviado a consola:" << cmd.trimmed();
    }
    else {
        qWarning() << "[FileEventBridge] No puedo escribir, proceso no está corriendo";
    }
}

void FileEventBridge::checkFile()
{
    if (!m_file.isOpen()) return;

    qint64 sz = m_file.size();
    if (sz < m_lastPos) {
        // el log se truncó: volvemos al principio
        m_lastPos = 0;
    }
    if (sz == m_lastPos) return;

    m_file.seek(m_lastPos);
    QTextStream in(&m_file);

    // buscamos el bloque que empieza con "#STATUS"
    bool inStatus = false;
    QStringList lines;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line == "#STATUS") {
            inStatus = true;
            lines.clear();
            continue;
        }
        if (!inStatus) continue;
        if (line.startsWith('#')) {
            // fin de bloque
            break;
        }
        lines << line;
    }
    m_lastPos = m_file.pos();

    if (inStatus && !lines.isEmpty()) {
        qDebug() << "[FileEventBridge] statusUpdated con" << lines;
        emit statusUpdated(lines);
    }
}
