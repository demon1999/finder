#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scanner.h"
#include "finder.h"
#include "data_saver.h"

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
    QCommonStyle style;
    is_running = false;
    threadWithData = new QThread;
    myData = new data_saver();
    myData->moveToThread(threadWithData);
    qRegisterMetaType<QVector<QString> >("QVector<QString>");
    connect(this, SIGNAL(give_files(QString)), myData, SLOT(search(QString)));
    connect(myData, SIGNAL(s_indexing_finished()), this, SLOT(find_word()));
    connect(this, SIGNAL(start_scanning(QString)), myData, SLOT(scan_directory(QString)));
    connect(myData, SIGNAL(percentage(int)), this, SLOT(show_percentage(int)));
    connect(myData, SIGNAL(done(const QVector<QString> &)), this, SLOT(add_info(const QVector<QString> &)));
    threadWithData->start();
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
    myData->set_flag();
    progressBar->hide();
    is_running = false;
}

void main_window::scan_directory(QString const& dir, QString const& text)
{
    if (dir.size() == 0)
        return;
    if (is_running) {
        QMessageBox::information(nullptr, "info", "You can't start new search, before previous one has finished.");
        return;
    }
    currentDir = dir;
    is_running = true;
    textToFind = text;
    progressBar->setValue(0);
    progressBar->setFormat("indexing progress: " + QString::number(0) + "%");
    progressBar->show();
    myData->reset_flag();
    emit start_scanning(dir);
}

void main_window::show_percentage(int k) {

    QString text = progressBar->text();
    QString p = "";

    for (int i = 0; i < text.size(); i++) {
        if (text[i] >= '0' && text[i] <= '9') break;
        p += text[i];
    }
    progressBar->setValue(k);;
    progressBar->setFormat(p + QString::number(k) + "%");

    //std::cout << p.toStdString() + QString::number(progressBar->value()).toStdString() + "%" << "\n";
    if (progressBar->isVisible())
        progressBar->show();
}

void main_window::find_word() {
    progressBar->hide();
    progressBar->setValue(0);
    progressBar->setFormat("search progress: " + QString::number(0) + "%");
    progressBar->show();
    fileNames.resize(0);
    emit give_files(textToFind);
}

void main_window::add_info(const QVector<QString> &paths) {
    fileNames.append(paths);
    setWindowTitle("files of " + currentDir + " which contain " + "\"" + textToFind + "\" as a substring");
    show_current();
    progressBar->hide();
    is_running = false;
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
