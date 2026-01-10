#include "FileAppender.h"
#include "../core/Constants.h"
#include <QDir>
#include <QMutexLocker>
#include <QDebug>

namespace TR4QT {

FileAppender::FileAppender(const QString& filePath, QObject* parent)
    : QObject(parent)
    , m_filePath(filePath)
    , m_maxFileSize(DEFAULT_MAX_LOG_FILE_SIZE)
    , m_maxBackupFiles(5)               // 5 backup files default
    , m_currentSize(0)
{
}

FileAppender::~FileAppender() {
    close();
}

bool FileAppender::open() {
    QMutexLocker locker(&m_mutex);

    if (m_file.isOpen()) {
        return true;  // Already open
    }

    // Ensure directory exists
    QFileInfo fileInfo(m_filePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            qWarning() << "Failed to create log directory:" << dir.absolutePath();
            return false;
        }
    }

    // Open file for appending
    m_file.setFileName(m_filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << m_filePath << m_file.errorString();
        return false;
    }

    // Set up text stream
    m_stream.setDevice(&m_file);
    m_currentSize = m_file.size();

    return true;
}

void FileAppender::close() {
    QMutexLocker locker(&m_mutex);

    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
    m_currentSize = 0;
}

void FileAppender::flush() {
    QMutexLocker locker(&m_mutex);

    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.flush();
    }
}

bool FileAppender::isOpen() const {
    QMutexLocker locker(&m_mutex);
    return m_file.isOpen();
}

void FileAppender::append(const QString& message) {
    QMutexLocker locker(&m_mutex);

    if (!m_file.isOpen()) {
        if (!const_cast<FileAppender*>(this)->open()) {
            return;  // Failed to open file
        }
    }

    // Check if rotation needed before writing
    rotateIfNeeded();

    // Write message
    m_stream << message;
    m_stream.flush();  // Ensure message is written

    // Update current size
    qint64 messageSize = message.toUtf8().size();
    m_currentSize += messageSize;
}

void FileAppender::setMaxFileSize(qint64 bytes) {
    QMutexLocker locker(&m_mutex);
    m_maxFileSize = bytes;
}

void FileAppender::setMaxBackupFiles(int count) {
    QMutexLocker locker(&m_mutex);
    m_maxBackupFiles = count;
}

void FileAppender::rotateIfNeeded() {
    // Called with mutex already locked
    if (m_currentSize < m_maxFileSize) {
        return;  // No rotation needed
    }

    performRotation();
}

void FileAppender::performRotation() {
    // Called with mutex already locked

    // Close current file
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }

    QString basePath = m_filePath;

    // Delete oldest backup if it exists
    QString oldestBackup = basePath + QString(".%1").arg(m_maxBackupFiles);
    if (QFile::exists(oldestBackup)) {
        QFile::remove(oldestBackup);
    }

    // Rotate existing backups: .1 → .2, .2 → .3, etc.
    for (int i = m_maxBackupFiles - 1; i >= 1; --i) {
        QString from = basePath + QString(".%1").arg(i);
        QString to = basePath + QString(".%1").arg(i + 1);
        if (QFile::exists(from)) {
            // Remove destination if it exists
            if (QFile::exists(to)) {
                QFile::remove(to);
            }
            QFile::rename(from, to);
        }
    }

    // Rename current log to .1
    if (QFile::exists(basePath)) {
        QString backup = basePath + ".1";
        if (QFile::exists(backup)) {
            QFile::remove(backup);
        }
        QFile::rename(basePath, backup);
    }

    // Open new log file
    m_file.setFileName(basePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open new log file after rotation:" << m_filePath;
        return;
    }

    m_stream.setDevice(&m_file);
    m_currentSize = 0;
}

} // namespace TR4QT
