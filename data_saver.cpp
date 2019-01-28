#include "data_saver.h"
#include "finder.h"
#include "scanner.h"
#include <iostream>
#include <QString>
#include <QVector>
#include <QThread>

data_saver::data_saver()
{
    int k = QThread::idealThreadCount() - 1;
    if (k <= 0) k = 2;
    numberOfThreads = k;
}

void data_saver::set_flag() {
    aborted_flag = true;
    for (auto v : finders) {
        v->set_flag();
    }
    for (auto v : scan)
        v->set_flag();
}

void data_saver::add_path(const QString &path) {
    if (added.find(path) == added.end()) {
        added.insert(path);
        watcher->addPath(path);
    }
}
void data_saver::get_files(const QString &dir, QMap<QString, bool> &was, QSet<QString> &fileList, bool isSearch) {
    if (aborted_flag) return;
    QDir d(dir);
    if (was[d.canonicalPath()]) return;
    was[d.canonicalPath()] = true;
    //std::cout << d.canonicalPath().toStdString() << "\n";
    QFileInfoList list = d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    if (!isSearch)
        add_path(dir);
    for (const auto& file_info : list)
    {
        if (aborted_flag) return;
        if (file_info.isSymLink()) {
            //ignore
        } else if (file_info.isReadable()) {
            if (file_info.isDir()) {
                get_files(file_info.absoluteFilePath(), was, fileList, isSearch);
            } else {
                QString path = file_info.canonicalFilePath();
                fileList.insert(path);
                if (!isSearch) {
                    add_path(path);
                }
            }
        }
    }
}


void data_saver::scan_directory(QString dir) {
    finishedThreads = 0;
    QVector<QString> fileList;
    QMap<QString, bool> was;
    if (currentDir == dir) {
        for (auto v : fileList) {
            auto it = data.find(v);
            if (it != data.end())
                data.erase(it);
            //std::cout << "I got it! " << v.toStdString() << "\n";
        }

    } else {
        added.clear();
        currentDir = dir;
        watcher = new QFileSystemWatcher;
        get_files(dir, was, changed, false);
    }
    for (auto v : changed) {
        fileList.append(v);
    }
    if (aborted_flag) return;
    connect(watcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(directory_changed(const QString &)));
    connect(watcher, SIGNAL(fileChanged(const QString &)), this, SLOT(directory_changed(const QString &)));

    for (auto v : threadsForScanning)
        v->quit();
    threadsForScanning.resize(0);
    for (int i = 0; i < numberOfThreads; i++)
        threadsForScanning.push_back(new QThread);

    cnt = fileList.size();
    currentCnt = 0;
    scan.resize(0);
    QSet<QPair<long long, long long> > sizes;

    for (int i = 0; i < threadsForScanning.size(); i++) {
        scan.push_back(new scanner());
        sizes.insert({0, i});
    }

    for (auto path : fileList) {
        QPair<long long, long long> now = *sizes.begin();
        sizes.erase(sizes.begin());
        now.first += QFile(path).size();
        sizes.insert(now);
        scan[now.second]->add_file(path);
    }

    for (int i = 0; i < threadsForScanning.size(); i++) {
        scan[i]->moveToThread(threadsForScanning[i]);
        connect(scan[i], SIGNAL(percentage()), this, SLOT(show_percentage()));
        connect(threadsForScanning[i], SIGNAL(started()), scan[i], SLOT(run()));
        connect(scan[i], SIGNAL(finished()), threadsForScanning[i], SLOT(quit()));
        qRegisterMetaType<QSet<qint32> >("QSet<qint32>");
        connect(scan[i], SIGNAL(done(const QString&, const QSet<qint32> &)), this, SLOT(add_info(const QString&, const QSet<qint32> &)));
        connect(scan[i], SIGNAL(indexing_finished()), this, SLOT(indexing_finished()));

        threadsForScanning[i]->start();
    }
}

void data_saver::directory_changed(const QString &path) {
    //std::cout << path.toStdString() << "\n";
    if (QFileInfo(path).isDir()) {
            QMap<QString, bool> was;
            get_files(path, was, changed, false);
        }
    if (QFileInfo(path).isFile()) {
            changed.insert(path);
            add_path(path);
    }
}

void data_saver::indexing_finished() {
    finishedThreads++;
    if (aborted_flag) return;
    if (finishedThreads == numberOfThreads)
        emit s_indexing_finished();
}
void data_saver::reset_flag() {
    aborted_flag = false;
}

void data_saver::add_info(const QString& path, const QSet<qint32> &_data) {
    data[path] = _data;
    auto it = changed.find(path);
    if (it != changed.end())
        changed.erase(it);
    //std::cout << path.toStdString() << " update indexes " << _data.size() << "\n";
}

void data_saver::search(QString textToFind) {

    if (aborted_flag) return;
    QMap<QString, bool> was;
    QSet<QString> f;
    get_files(currentDir, was, f, true);
    QVector<QString> fileList;
    for (auto v : f) fileList.push_back(v);


    cnt = fileList.size();
    currentCnt = 0;

    finishedThreads = 0;
    for (auto v : threadsForSearch)
        v->quit();
    threadsForSearch.resize(0);
    for (int i = 0; i < numberOfThreads; i++)
        threadsForSearch.push_back(new QThread);
    finders.resize(0);
    fileNames.resize(0);
    for (int i = 0; i < threadsForSearch.size(); i++) {
        finders.push_back(new finder(textToFind));
        for (int j = i; j < fileList.size(); j += threadsForSearch.size()) {
            finders[i]->add_file(fileList[j], data[fileList[j]]);
        }
    }
    if (aborted_flag) return;
    //std::cout << threadsForSearch.size() << "\n";
    for (int i = 0; i < threadsForSearch.size(); i++) {
        finders[i]->moveToThread(threadsForSearch[i]);
        connect(finders[i], SIGNAL(percentage()), this, SLOT(show_percentage()));
        connect(threadsForSearch[i], SIGNAL(started()), finders[i], SLOT(run()));
        connect(finders[i], SIGNAL(finished()), threadsForSearch[i], SLOT(quit()));
        qRegisterMetaType<QVector<QString>>("QVector<QString>");
        connect(finders[i], SIGNAL(done(const QVector<QString> &)), this, SLOT(add_info(const QVector<QString> &)));
        threadsForSearch[i]->start();
    }
}

void data_saver::show_percentage() {
    currentCnt++;
    if (cnt == 0) {
        emit percentage(100);
    } else {
//        std::cout << currentCnt << " " << cnt;
//        if (currentCnt > cnt) std::cout << " ya debil!\n";
//        else
//            std::cout << "\n";
        emit percentage(currentCnt * 100 / cnt);
    }
}


void data_saver::add_info(const QVector<QString> &paths) {
    fileNames.append(paths);
    finishedThreads++;
    if (aborted_flag) return;
    //std::cout << "kukarek\n";
    if (finishedThreads == threadsForSearch.size()) {
        emit done(fileNames);
    }
}

