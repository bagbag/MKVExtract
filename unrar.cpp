#include "unrar.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <limits>
#include <QThread>

const static QString CONTENT_INFO_PATTERN(R"/(\s*(?<key>.*): (?<value>[^\r\n]*))/");

Unrar::Unrar(QString binaryPath ) : QObject()
{
    _binaryPath = binaryPath;
}

void Unrar::setBinary(QString binaryPath)
{
    _binaryPath = binaryPath;
}

void Unrar::extract(QFileInfo rarFile, QString password, bool fullPath, bool overwrite, QDir extractPath)
{
    QStringList arguments;
    arguments << (fullPath ? "x" : "e") << "-y" << "-o" + QString(overwrite ? "+" : "-") << "-p" + (password.isEmpty() ? "-" : password) << rarFile.absoluteFilePath() << extractPath.absolutePath() + "/";

    runProcess(arguments, rarFile.dir(), true);
}

void Unrar::extract(QFileInfo rarFile, QString password, bool fullPath, QFileInfoList files, bool overwrite, QDir extractPath)
{
    if (files.length() == 0)
        return;

    QStringList arguments;
    arguments << (fullPath ? "x" : "e") << "-y" << "-o" + QString(overwrite ? "+" : "-") << "-p" + (password.isEmpty() ? "-" : password) << rarFile.absoluteFilePath();
    if (files.length() > 0)
    {
        foreach (QFileInfo file, files)
        {
            arguments << file.filePath();
        }
    }

    arguments << extractPath.absolutePath() + "/";

    runProcess(arguments, rarFile.dir(), true);
}

QList<Unrar::ContentInfo> Unrar::listContents(QFileInfo rarFile, QString password, bool includeDirectories)
{
    if (!QFileInfo(rarFile).exists())
    {
        qDebug() << "(Unrar::listContents): File" << rarFile.absoluteFilePath() << "does not exist";
        return QList<Unrar::ContentInfo>();
    }

    QStringList arguments;
    arguments << "lt" << "-v" << "-p" + (password.isEmpty() ? "-" : password) << rarFile.absoluteFilePath();

    QString output = runProcess(arguments, rarFile.dir(), false);

    QStringList split = output.split(QRegularExpression("\r?\n\r?\n"), QString::SkipEmptyParts);

    if (split.length() <= 2)
        return QList<Unrar::ContentInfo>();

    split.removeFirst();
    split.removeFirst(); //remove Copyright etc.
    split.removeDuplicates();

    QHash<QString, Unrar::ContentInfo> contentInfos;

    QRegularExpression regex(CONTENT_INFO_PATTERN);
    foreach (QString entry, split)
    {
        QRegularExpressionMatchIterator matches = regex.globalMatch(entry);

        ContentInfo info;
        while (matches.hasNext())
        {
            QRegularExpressionMatch match = matches.next();

            QString key(match.captured("key"));
            QString value(match.captured("value"));

            if (key == "Name") {
                info.Name = value;
            }
            if (key == "Type")
            {
                if (value == "File")
                    info.Type = ContentType::File;
                else if (value == "Directory")
                    info.Type = ContentType::Directory;
            }
            if (key == "Size")
                info.Size = value.toLongLong();
            if (key == "Packed Size")
                info.PackedSize = value.toLongLong();
            if (key == "Ratio")
                info.Ratio = value.replace("%","").toFloat()/(qreal)100;
            if (key == "mtime")
                info.MTime = value;
            if (key == "Attributes")
                info.Attributes = value;
            if (key == "CRC32")
                info.CRC32 = value;
            if (key == "Host OS")
                info.HostOS = value;
            if (key == "Compression")
                info.Compression = value;
            if (key == "Flags")
            {
                if (value == "encrypted")
                    info.Flags.append(ContentFlag::Encrypted);
            }
        }

        if (contentInfos.contains(info.Name))
        {
            //contentInfos[info.Name].Size += info.Size;
            contentInfos[info.Name].PackedSize += info.PackedSize;
            contentInfos[info.Name].CRC32 = info.CRC32;
        }
        else if (!info.Name.isEmpty() && (info.Type == ContentType::File || includeDirectories))
            contentInfos[info.Name] = info;

    }

    return contentInfos.values();
}

bool Unrar::hasPassword(QFileInfo rarFile)
{
    QStringList arguments;
    arguments << "lt" << "-v" << "-p-" << rarFile.absoluteFilePath();

    QString output = runProcess(arguments, rarFile.dir(), false);

    return output.contains("Flags: encrypted") || output.contains("The specified password is incorrect.") || output.contains(QRegularExpression(R"/(Details: RAR \d, .*?encrypted headers)/"));
}

QString Unrar::crackPasswort(QFileInfo rarFile, QStringList passwordList)
{
    QStringList arguments;
    arguments << "lt" << "-v" << "-p-" << rarFile.absoluteFilePath();

    QString output = runProcess(arguments, rarFile.dir(), false);

    bool encryptedHeaders = output.contains(QRegularExpression(R"/(Details: RAR \d, .*?encrypted headers)/"));

    qint64 smallestFileSize = std::numeric_limits<qint64>::max();
    QString smallestFile;
    if (!encryptedHeaders)
    {
        QList<ContentInfo> contents = listContents(rarFile, "", false);

        foreach (ContentInfo content, contents)
        {
            if (content.Size < smallestFileSize)
            {
                smallestFileSize = content.Size;
                smallestFile = content.Name;
            }
        }
    }

    foreach (QString password, passwordList)
    {
        qDebug() << "testing password: " << password;

        QStringList arguments;
        arguments << "t" << "-p" + password << rarFile.absoluteFilePath();
        if (!encryptedHeaders)
            arguments << smallestFile;

        QString output = runProcess(arguments, rarFile.dir(), false);

        if (output.trimmed().endsWith("All OK"))
            return password;
    }

    return "";
}

QString Unrar::runProcess(QStringList arguments, QDir workingDir, bool forwardChannels)
{
    QProcess process;
    process.setProgram(_binaryPath);

    process.setArguments(arguments);
    process.setWorkingDirectory(workingDir.absolutePath());
    if (forwardChannels)
        process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start();

    process.waitForFinished(10000000);

    return QString(process.readAll());
}
