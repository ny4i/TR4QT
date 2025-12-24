#ifndef FILEAPPENDER_H
#define FILEAPPENDER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QString>

namespace TR4QT {

/**
 * Thread-safe file appender with log rotation support
 *
 * Features:
 * - Automatic file rotation based on size
 * - Keeps N backup files (tr4qt.log.1, tr4qt.log.2, etc.)
 * - Atomic writes with mutex protection
 * - Automatic directory creation
 */
class FileAppender : public QObject {
    Q_OBJECT

public:
    explicit FileAppender(const QString& filePath, QObject* parent = nullptr);
    ~FileAppender();

    // Append message to log file
    void append(const QString& message);

    // Configuration
    void setMaxFileSize(qint64 bytes);  // Default: 10 MB
    qint64 maxFileSize() const { return m_maxFileSize; }

    void setMaxBackupFiles(int count);   // Default: 5
    int maxBackupFiles() const { return m_maxBackupFiles; }

    // File management
    bool open();
    void close();
    bool isOpen() const;

    QString filePath() const { return m_filePath; }

private:
    void rotateIfNeeded();
    void performRotation();

    QString m_filePath;
    QFile m_file;
    QTextStream m_stream;
    mutable QMutex m_mutex;
    qint64 m_maxFileSize;      // Maximum size before rotation
    int m_maxBackupFiles;       // Number of backup files to keep
    qint64 m_currentSize;       // Current file size
};

} // namespace TR4QT

#endif // FILEAPPENDER_H
