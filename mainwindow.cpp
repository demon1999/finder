#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scanner.h"

#include <iostream>
#include <QMap>
#include <QVector>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QCryptographicHash>
#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>
#include <QWidgetAction>
#include <QProgressBar>
#include <QSizePolicy>

main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{    
    ui->setupUi(this);
    progressBar = new QProgressBar(ui->StatusBar);
    ui->StatusBar->addPermanentWidget(progressBar);
    progressBar->hide();
    progressBar->setAlignment(Qt::AlignRight);
    progressBar->setMinimumSize(210, 30);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    int k = QThread::idealThreadCount() - 2;
    if (k == 1) k = 2;
    for (int i = 0; i < k; i++)
        threads.append(new QThread);
    QCommonStyle style;
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    //connect(ui->actionStop, &QAction::triggered, this, &main_window::stop_scanning);
    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);

}

main_window::~main_window()
{}

void main_window::select_directory()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    scan_directory(dir);
}

//void main_window::stop_scanning() {

//    if (scan == nullptr)
//        return;
//    scan->set_flag();
//    progressBar->hide();
//}

void main_window::get_files(const QString &dir, QMap<QString, bool> &was, QVector<QString> &fileList, bool isSearch) {
    QDir d(dir);
    if (was[d.canonicalPath()]) return;
    was[d.canonicalPath()] = true;

    QFileInfoList list = d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (QFileInfo file_info : list)
    {
        if (file_info.isSymLink()) {
            get_files(file_info.absoluteFilePath(), was, fileList, isSearch);
        } else
        if (file_info.isDir()) {
            if (QDir(file_info.absoluteFilePath()).isReadable())
                get_files(file_info.absoluteFilePath(), was, fileList, isSearch);
        } else {
            QString path = file_info.canonicalFilePath();
            if (((data.find(path) == data.end() || data[path].first != file_info.lastModified()) && !isSearch) ||
                (data.find(path) != data.end() && !data[path].second.empty() && isSearch))
                fileList.append(path);
        }
    }
}

bool main_window::is_running() {
    for (auto thread : threads) {
        if (thread->isRunning()) {
            QMessageBox::information(nullptr, "info", "You can't start new search, before previous one has finished.");
            return true;
        }
    }
    return false;
}
void main_window::scan_directory(QString const& dir)
{
    if (is_running()) {
        return;
    }

    progressBar->show();
    progressBar->setValue(0);
    progressBar->setFormat("update progress: " + QString::number(0) + "%");

    finishedThreads = 0;
    QVector<QString> fileList;
    QMap<QString, bool> was;
    get_files(dir, was, fileList, false);

    currentDir = dir;
    cnt = fileList.size();
    currentCnt = 0;
    scan.resize(0);
    QSet<QPair<long long, long long> > sizes;

    for (int i = 0; i < threads.size(); i++) {
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

    for (int i = 0; i < threads.size(); i++) {
        scan[i]->moveToThread(threads[i]);
        connect(scan[i], SIGNAL(percentage()), this, SLOT(show_percentage()));
        connect(threads[i], SIGNAL(started()), scan[i], SLOT(run()));
        connect(scan[i], SIGNAL(finished()), threads[i], SLOT(quit()));
        qRegisterMetaType<QMap<QString, QPair<QDateTime, QSet<qint32> > > >("QMap<QString, QPair<QDateTime, QSet<qint32> > >");
        connect(scan[i], SIGNAL(done(const QMap<QString, QPair<QDateTime, QSet<qint32> > > &)), this, SLOT(add_info(const QMap<QString, QPair<QDateTime, QSet<qint32> > > &)));
        threads[i]->start();
    }
}

void main_window::show_percentage() {
    currentCnt++;

    QString text = progressBar->text();
    QString p = "";
    for (int i = 0; i < text.size(); i++) {
        if (text[i] >= '0' && text[i] <= '9') break;
        p += text[i];
    }
    if (cnt == 0) {
        progressBar->setValue(100);;
        progressBar->setFormat(p + QString::number(100) + "%");
    } else {
        progressBar->setValue(currentCnt * 100 / cnt);
        progressBar->setFormat(p + QString::number(progressBar->value()) + "%");
    }
    progressBar->show();
}

void main_window::add_info(const QMap<QString, QPair<QDateTime, QSet<qint32> > > & _data) {
    finishedThreads++;
    for (auto v = _data.begin(); v != _data.end(); v++)
        data[v.key()] = v.value();
    if (finishedThreads == threads.size()) {
        for (auto &thread : threads) {
            if (thread->isRunning()) {
                thread->quit();
            }
        }
        find_word();
        return;
    }
//    progressBar->hide();
}

void main_window::find_word() {
    if (is_running()) {
        return;
    }

    progressBar->setValue(0);
    progressBar->setFormat("search progress: " + QString::number(0) + "%");

    finishedThreads = 0;
    QVector<QString> fileList;
    QMap<QString, bool> was;
    get_files(dir, was, fileList, true);
    cnt = fileList.size();
    currentCnt = 0;
    search.resize(0);
    fileNames.resize(0);

    for (int i = 0; i < threads.size(); i++) {
        search.push_back(new finder("kukarek"));
        for (int j = i; j < fileList.size(); j += threads.size()) {
            search[i]->addFile(fileList[j], data[fileList[j]]);
        }
    }

    for (int i = 0; i < threads.size(); i++) {
        search[i]->moveToThread(threads[i]);
        connect(search[i], SIGNAL(percentage()), this, SLOT(show_percentage()));
        connect(threads[i], SIGNAL(started()), search[i], SLOT(run()));
        connect(search[i], SIGNAL(finished()), threads[i], SLOT(quit()));
        qRegisterMetaType<QVector<QString> >("QVector<QString>");
        connect(search[i], SIGNAL(done(const QVector<QString> &)), this, SLOT(add_info(const QVector<QString> &)));
        threads[i]->start();
    }
}

void main_window::add_info(const QVector<QString> &paths) {
    finishedThreads++;
    for (auto path : paths)
        fileNames.append(path);
    if (finishedThreads == threads.size()) {
        for (auto &thread : threads)
            thread->quit();
        setWindowTitle("files of " + currentDir + " which contain " + "\"" + "kukarek" + "\" as a substring");
        show_current();
        return;
    }

}
void main_window::show_current() {
    QString title = QWidget::windowTitle();
    ui->treeWidget->clear();
    ui->treeWidget->hide();
    setWindowTitle(title);

    if (fileNames.empty()) {
        ui->treeWidget->show();
        return;
    }

    std::sort(fileNames.begin(), fileNames.end());
    QString prevFilename = "/home";

    QTreeWidgetItem *last = new QTreeWidgetItem(ui->treeWidget);
    last->setText(0, "home");

    for (const auto & filename : fileNames) {
        QStringList filenameParts = filename.split('/').mid(1);
        QStringList prevFilenameParts = prevFilename.split('/').mid(1);
        int cnt = 0;
        for (int i = 0; i < std::min(filenameParts.size(), prevFilenameParts.size()); i++) {
            if (filenameParts[i] != prevFilenameParts[i]) break;
            cnt++;
        }
        for (int i = 0; i < prevFilenameParts.size() - cnt; i++)
            last = last->parent();
        for (int i = cnt; i < filenameParts.size(); i++) {
            QTreeWidgetItem *treeItem = new QTreeWidgetItem();
            //treeItem->setIcon();
            treeItem->setText(0, filenameParts[i]);
            last->addChild(treeItem);
            last = treeItem;
        }
        prevFilename = filename;
    }
    ui->treeWidget->show();
}

void main_window::show_about_dialog()
{
    QMessageBox::aboutQt(this);
}
