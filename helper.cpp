#include "helper.h"

QFileInfoList Helper::getRarFiles(QFileInfoList files, bool baseRarOnly)
{
    QString allPatern = R"/((rar|r\d+)$)/";
    QString basePattern = R"/(^(?!.*\.part(\d*[^1]|[^0]*1)\.rar$).*\.(part0*1\.)?rar$)/";

    QFileInfoList rarFiles;

    foreach (QFileInfo file, files)
    {
        QRegularExpression regex(baseRarOnly ? basePattern : allPatern);
        QRegularExpressionMatch match = regex.match(file.absoluteFilePath());

        if (match.hasMatch())
            rarFiles.append(file);
    }

    return rarFiles;
}


QFileInfoList Helper::getRelatedFiles(QFileInfoList files, QFileInfo baseRar)
{
    QRegularExpression suffixRegex(R"/(rar|part\d+|r\d+)/");
    QString baseName = baseRar.absoluteFilePath();

    QFileInfo fileInfo;
    while (suffixRegex.match((fileInfo = QFileInfo(baseName)).suffix()).hasMatch())
    {
        baseName = fileInfo.completeBaseName();
    }

    QFileInfoList relatedFiles;

    QRegularExpression baseNameFileIsRarRegex(R"/(\.(part\d+\.rar|rar|r\d+)$)/");
    foreach (QFileInfo file, files)
    {
        if (baseNameFileIsRarRegex.match(file.absoluteFilePath()).hasMatch() && file.absoluteFilePath().startsWith(baseName))
            relatedFiles.append(file);
    }

    return relatedFiles;
}

QList<QDir> Helper::getDirs(QDir baseDir, bool includeBaseDir, bool recursive, bool followSymlinks)
{
    QList<QDir> dirs;

    if (includeBaseDir)
        dirs.append(baseDir);

    QDirIterator dirIterator(baseDir.absolutePath(), QDir::Dirs | QDir::NoDotAndDotDot, (recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags) |
                             (followSymlinks ? QDirIterator::FollowSymlinks : QDirIterator::NoIteratorFlags));

    while(dirIterator.hasNext())
    {
        dirs.append(QDir(dirIterator.next()));
    }

    return dirs;
}

QFileInfoList Helper::getFiles(QDir baseDir, bool recursive, bool followSymlinks)
{
    QFileInfoList fileInfos;

    QList<QDir> dirs;

    if (recursive) {
        dirs << getDirs(baseDir, true, true, followSymlinks);
    }
    else {
        dirs << baseDir;
    }

    foreach (QDir dir, dirs)
        fileInfos << dir.entryInfoList(QDir::Files);

    return fileInfos;
}

QStringList Helper::getAbsolutFilePaths(QFileInfoList fileInfos)
{
    QStringList absoluteFilePaths;

    foreach (QFileInfo fileInfo, fileInfos)
    {
        absoluteFilePaths.append(fileInfo.absoluteFilePath());
    }

    return absoluteFilePaths;
}
