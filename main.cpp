#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include "helper.h"
#include "unrar.h"
#include <QCommandLineParser>

void ExtractMovies(QFileInfo rarFile, QDir path, QStringList passwordList = QStringList());
Unrar unrar("unrar");

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("MKVExtract");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("inputDir", "where to look for rars");
    parser.addPositionalArgument("outputDir", "where to extract mkvs");
    parser.addOption(QCommandLineOption("p", "passwords file", "passwordFile"));
    parser.addOption(QCommandLineOption("u", "unrar binary (default is global path unrar)", "unrarBinary"));

    parser.process(a);
    if(parser.positionalArguments().length()!=2) {
        parser.showHelp();
        return EXIT_FAILURE;
    }

    if(parser.isSet("u")) {
        QString unrarBinary = parser.value("u");

        if (!QFileInfo(unrarBinary).exists()) {
            qDebug() << "unrar binary " << unrarBinary << " not found";
            return EXIT_FAILURE;
        }

        unrar.setBinary(unrarBinary);
       }

    QStringList passwordList;

    if(parser.isSet("p")) {
       QString pwFile = parser.value("p");
       QFile pf (pwFile);
       if(!pf.exists() || !pf.open(QFile::ReadOnly)) {
           qDebug() << "Pw file" << pwFile << "does not exist or cannot be opened";
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

    QFileInfoList files = inputDir.entryInfoList(QDir::Files);
    QFileInfoList baseRarFiles = Helper::getRarFiles(files, true);

    foreach (QFileInfo baseRarFile, baseRarFiles)
    {
        qDebug() << "Taking a look at " << baseRarFile.absoluteFilePath();
        ExtractMovies(baseRarFile, outputDir, passwordList);
    }

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
        if (content.Size > 1024 * 1024 * 1024 && content.Name.endsWith("mkv"))
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
