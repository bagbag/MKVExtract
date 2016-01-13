#ifndef UNRAR_H
#define UNRAR_H

#include <QObject>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

class Unrar : public QObject
{
    Q_OBJECT

private:
    QString _binaryPath;

public:
    enum ContentType
    {
        File,
        Directory
    };

    enum ContentFlag
    {
        Encrypted
    };

    struct ContentInfo
    {
        QString Name;
        ContentType Type;
        qint64 Size;
        qint64 PackedSize;
        qreal Ratio;
        QString MTime;
        QString Attributes;
        QString CRC32;
        QString HostOS;
        QString Compression;
        QList<ContentFlag> Flags;
    };

    explicit Unrar(QString binaryPath);
    void setBinary(QString binaryPath);
    void extract(QFileInfo rarFile, QString password, bool fullPath, QDir extractPath = QDir("."));
    void extract(QFileInfo rarFile, QString password, bool fullPath, QFileInfoList files, QDir extractPath = QDir("."));
    QList<ContentInfo> listContents(QFileInfo rarFile, QString password, bool includeDirectories);
    bool hasPassword(QFileInfo rarFile);
    QString crackPasswort(QFileInfo rarFile, QStringList passwordList);
    QString runProcess(QStringList arguments, QDir workingDir, bool forwardChannels);

signals:

public slots:
private slots:
};

#endif // UNRAR_H
