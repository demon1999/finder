#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QMap>
#include <QSet>
#include <QPair>
#include <QDateTime>
#include <QProgressBar>
#include <QThread>
#include <QString>
#include <QVector>
#include <QLabel>
#include <functional>
#include <memory>
#include <iterator>
#include "scanner.h"
#include "finder.h"

namespace Ui {
class MainWindow;
}

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = 0);
    ~main_window();

private slots:
    void stop_scanning();
    void select_options();
    void scan_directory(QString const& dir, QString const& text);
    void show_about_dialog();
    void show_percentage();
    void select_directory(QString const &text);
    void add_info(const QMap<QString, QPair<QDateTime, QSet<qint32> > >  &_data);
    void add_info(const QVector<QString>  &_data);
private:
    bool is_running();
    void find_word();
    void get_files(const QString &dir, QMap<QString, bool> &was, QVector<QString> &fileList, bool isSearch);
    void show_current();
    QString currentDir;
    QString textToFind;
    QLabel *label;
    int cnt, currentCnt, numberOfThreads, finishedThreads;
    QProgressBar* progressBar;
    QVector<scanner*> scan;
    QVector<finder*> search;
    QVector<QThread*> threadsForScanning, threadsForSearch;
    QMap<QString, QPair<QDateTime, QSet<qint32> > > data;
    QVector<QString> fileNames;
    std::unique_ptr<Ui::MainWindow> ui;
};

#endif // MAINWINDOW_H
