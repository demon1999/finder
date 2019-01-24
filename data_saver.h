#ifndef DATA_SAVER_H
#define DATA_SAVER_H

#include <iostream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QCryptographicHash>
#include <QSet>
#include <QPair>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <atomic>
#include "finder.h"
#include "scanner.h"

class data_saver : public QObject
{
        Q_OBJECT

private:
    QVector<scanner*> scan;
    QVector<QThread*> threadsForScanning;
    QSet<QString> added;
    QSet<QString> changed;
    QFileSystemWatcher *watcher;

    int cnt, currentCnt, numberOfThreads, finishedThreads;
    QVector<QString> fileNames;
    QVector<finder*> finders;
    QVector<QThread*> threadsForSearch;
    std::atomic<bool> aborted_flag;
    QMap<QString, QSet<qint32> > data;
    QString currentDir;
    void indexing();
    void add_path(const QString &);
    void get_files(const QString &dir, QMap<QString, bool> &was, QSet<QString> &fileList, bool isSearch);
public:
    data_saver();
    void set_flag();
    void reset_flag();
public slots:
    void add_info(const QString &, const QSet<qint32> &);
    void add_info(const QVector<QString> &);
    void search(QString textToFind);
    void scan_directory(QString dir);
    void show_percentage();
    void directory_changed(const QString &);
    void indexing_finished();
signals:
    void s_indexing_finished();
    void percentage(int k);
    void done(const QVector<QString> &);
};

#endif // DATA_SAVER_H
