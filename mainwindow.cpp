#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scanner.h"
#include "finder.h"

#include <QtAlgorithms>
#include <iostream>
#include <QMap>
#include <QInputDialog>
#include <QVector>
#include <QString>
#include <QFile>
#include <QTextEdit>
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
    if (k <= 0) k = 2;
    numberOfThreads = k;
    QCommonStyle style;
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_FileDialogContentsView));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    connect(ui->actionStop, &QAction::triggered, this, &main_window::stop_scanning);
    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::select_options);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);

}

main_window::~main_window()
{}

void main_window::select_options()
{
    QInputDialog *inputDialog = new QInputDialog(ui->centralWidget);
    inputDialog->setLabelText("write text to find");
    inputDialog->setOkButtonText("select directory");
    inputDialog->show();
    QObject::connect(inputDialog, SIGNAL(textValueSelected(const QString &)), this, SLOT(select_directory(const QString &)));
    inputDialog->exec();
}

void main_window::select_directory(const QString &text) {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    scan_directory(dir, text);
}

void main_window::stop_scanning() {
    for (auto v : scan) {
        if (v != nullptr)
            v->set_flag();
    }
    for (auto v : search)
        if (v != nullptr)
            v->set_flag();
    progressBar->hide();
}

void main_window::get_files(const QString &dir, QMap<QString, bool> &was, QVector<QString> &fileList, bool isSearch) {
    QDir d(dir);
    if (was[d.canonicalPath()]) return;
    was[d.canonicalPath()] = true;
    //std::cout << d.canonicalPath().toStdString() << "\n";
    QFileInfoList list = d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    if (!isSearch)
        watcher->addPath(dir);
    for (QFileInfo file_info : list)
    {
        if (file_info.isSymLink()) {
            //ignore
        } else
        if (file_info.isDir()) {
            if (QDir(file_info.absoluteFilePath()).isReadable())
                get_files(file_info.absoluteFilePath(), was, fileList, isSearch);
        } else {
            QString path = file_info.canonicalFilePath();
            if ((!isSearch) ||
                (data.find(path) != data.end() && !data[path].empty() && isSearch)) {
                fileList.append(path);
                if (!isSearch)
                    watcher->addPath(path);
                //std::cout << path.toStdString();
            }
        }
    }
}

bool main_window::is_running() {
    auto is_running = [](QThread *q) {return q->isRunning();};
    return std::any_of(threadsForScanning.begin(), threadsForScanning.end(), is_running) ||
           std::any_of(threadsForSearch.begin(), threadsForSearch.end(), is_running);
}

void main_window::directoryChanged(const QString &path) {
    if (QFileInfo(path).isDir()) {
        QMap<QString, bool> was;
        get_files(path, was, changed, false);
    }
    if (QFileInfo(path).isFile()) {
        changed.push_back(path);
        watcher->addPath(path);
    }
}

void main_window::scan_directory(QString const& dir, QString const& text)
{

    if (is_running()) {
        QMessageBox::information(nullptr, "info", "You can't start new search, before previous one has finished.");
        return;
    }

    textToFind = text;
    progressBar->setValue(0);
    progressBar->setFormat("indexing progress: " + QString::number(0) + "%");
    progressBar->show();

    finishedThreads = 0;
    QVector<QString> fileList;
    QMap<QString, bool> was;
    if (currentDir == dir) {
        qSwap(fileList, changed);
        for (auto v : fileList) {
            auto it = data.find(v);
            if (it != data.end())
                data.erase(it);
        }
    } else {
        currentDir = dir;
        watcher = new QFileSystemWatcher;
        get_files(dir, was, fileList, false);
    }
    changed.clear();

    connect(watcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(directoryChanged(const QString &)));
    connect(watcher, SIGNAL(fileChanged(const QString &)), this, SLOT(directoryChanged(const QString &)));

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
    if (progressBar->isVisible())
        progressBar->show();
}

void main_window::add_info(const QString& path, const QSet<qint32> &_data) {
    data[path] = _data;
}

void main_window::indexing_finished() {
    finishedThreads++;
    if (finishedThreads == numberOfThreads) {
        find_word();
        progressBar->hide();
        return;
    }
}

void main_window::find_word() {

    progressBar->setValue(0);
    progressBar->setFormat("search progress: " + QString::number(0) + "%");
    progressBar->show();
    finishedThreads = 0;
    for (auto v : threadsForSearch)
        v->quit();
    threadsForSearch.resize(0);
    for (int i = 0; i < numberOfThreads; i++)
        threadsForSearch.push_back(new QThread);

    QVector<QString> fileList;
    QMap<QString, bool> was;
    get_files(currentDir, was, fileList, true);
    cnt = fileList.size();
    currentCnt = 0;
    search.resize(0);
    fileNames.resize(0);

    for (int i = 0; i < threadsForSearch.size(); i++) {
        search.push_back(new finder(textToFind));
        for (int j = i; j < fileList.size(); j += threadsForSearch.size()) {
            search[i]->add_file(fileList[j], data[fileList[j]]);
        }
    }

    for (int i = 0; i < threadsForSearch.size(); i++) {
        search[i]->moveToThread(threadsForSearch[i]);
        connect(search[i], SIGNAL(percentage()), this, SLOT(show_percentage()));
        connect(threadsForSearch[i], SIGNAL(started()), search[i], SLOT(run()));
        connect(search[i], SIGNAL(finished()), threadsForSearch[i], SLOT(quit()));
        qRegisterMetaType<QVector<QString> >("QVector<QString>");
        connect(search[i], SIGNAL(done(const QVector<QString> &)), this, SLOT(add_info(const QVector<QString> &)));
        threadsForSearch[i]->start();
    }
}

void main_window::add_info(const QVector<QString> &paths) {
    finishedThreads++;
    fileNames.append(paths);
    if (finishedThreads == numberOfThreads) {
        setWindowTitle("files of " + currentDir + " which contain " + "\"" + textToFind + "\" as a substring");
        show_current();
        progressBar->hide();
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
