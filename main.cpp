#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include "helper.h"
#include "unrar.h"
#include <QCommandLineParser>

void ExtractMovies(QFileInfo rarFile, QDir path, QStringList passwordList = QStringList());
Unrar unrar("unrar");
qint64 _minimumMovieSize = 1024 * 1024 * 1024; //1GiB;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("MKVExtract");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("inputDir", "where to look for rars");
    parser.addPositionalArgument("outputDir", "where to extract mkvs");
    parser.addOption(QCommandLineOption("m", "minimum movie size in MiB (default 1024)\nuseful to filter out samples", "minimumMovieSize"));
    parser.addOption(QCommandLineOption("p", "passwords file", "passwordFile"));
    parser.addOption(QCommandLineOption("u", "unrar binary (default is global path unrar)", "unrarBinary"));

    parser.process(a);
    if(parser.positionalArguments().length()!=2) {
        parser.showHelp();
        return EXIT_FAILURE;
    }

    if(parser.isSet("m")) {
        _minimumMovieSize = parser.value("m").toLongLong() * 1024 * 1024;

        if (_minimumMovieSize == 0)
        {
            qDebug() << "value " << parser.value("m") << "as minimum movie size not valid";
            return EXIT_FAILURE;
        }
    }

    if(parser.isSet("u")) {
        QString unrarBinary = parser.value("u");

        if (!QFileInfo(unrarBinary).exists()) {
            qDebug() << "unrar binary" << unrarBinary << "not found";
            return EXIT_FAILURE;
        }
        unrar.setBinary(unrarBinary);
    }

    QStringList passwordList;

    if(parser.isSet("p")) {
       QString pwFile = parser.value("p");
       QFile pf (pwFile);
       if(!pf.exists() || !pf.open(QFile::ReadOnly)) {
           qDebug() << "passwordfile" << pwFile << "does not exist or cannot be opened";
           return EXIT_FAILURE;
       }


       QString content = pf.readAll();
       QStringList lines = content.split(QRegularExpression("\\r?\\n"), QString::SkipEmptyParts);


       passwordList << lines;
    }

    QDir inputDir(parser.positionalArguments().at(0));
    QDir outputDir(parser.positionalArguments().at(1));

    if (!inputDir.exists()){
        qDebug() << "inputdir" << inputDir.absolutePath() << "does not exist";
        return EXIT_FAILURE;
    }


    QFileInfoList files = Helper::getFiles(inputDir, true, true);
    QFileInfoList baseRarFiles = Helper::getRarFiles(files, true);

    foreach (QFileInfo baseRarFile, baseRarFiles)
    {
        qDebug() << "Taking a look at" << baseRarFile.absoluteFilePath();
        ExtractMovies(baseRarFile, outputDir, passwordList);
    }

    qDebug() << "Done.";

    QCoreApplication::quit();
    return a.exec();
}

void ExtractMovies(QFileInfo rarFile, QDir path, QStringList passwordList)
{
    passwordList.removeDuplicates();

    bool hasPassword = unrar.hasPassword(rarFile);

    QString password = "";
    if (hasPassword)
    {
        password = unrar.crackPasswort(rarFile, passwordList);
        passwordList.insert(0, password);
        passwordList.removeDuplicates();

        if (password.isEmpty())
        {
            qDebug() << rarFile.absoluteFilePath() << "is password protected, but password not found";
            return;
        }
    }

    QList<Unrar::ContentInfo> contents = unrar.listContents(rarFile, password, false);

    QFileInfoList fileNames;
    QFileInfoList movieFiles;

    foreach (Unrar::ContentInfo content, contents)
    {
        if (content.Size > _minimumMovieSize && content.Name.endsWith("mkv"))
        {
            movieFiles << QFileInfo(content.Name);
        }
        fileNames.append(QFileInfo(content.Name));
    }

    QFileInfoList subRars = Helper::getRarFiles(fileNames, false);
    QDir extractDir = QDir(rarFile.absoluteDir().absolutePath() + "/extracted-" + rarFile.completeBaseName() + "/");

    if (!subRars.isEmpty())
        unrar.extract(rarFile, password, true, subRars, extractDir);

    QFileInfoList extractedSubRars = Helper::getRarFiles(Helper::getFiles(extractDir, false, true), true);
    foreach (QFileInfo extractedSubRar, extractedSubRars)
        ExtractMovies(extractedSubRar, path, passwordList);

    unrar.extract(rarFile, password, false, movieFiles, path);
}
