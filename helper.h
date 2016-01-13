#ifndef HELPER_H
#define HELPER_H

#include <QObject>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>

class Helper : public QObject
{
    Q_OBJECT
public:
    explicit Helper(QObject *parent = 0);
    static QFileInfoList getRarFiles(QFileInfoList files, bool baseRarOnly = false);
    static QFileInfoList getRelatedFiles(QFileInfoList files, QFileInfo baseRar);
    static QList<QDir> getDirs(QDir baseDir, bool includeBaseDir = false, bool recursive = false, bool followSymlinks = true);
    static QFileInfoList getFiles(QDir baseDir, bool recursive, bool followSymlinks);
    static QStringList getAbsolutFilePaths(QFileInfoList fileInfoList);

signals:

public slots:
};

#endif // HELPER_H
