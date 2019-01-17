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
#include <functional>
#include <memory>
#include <iterator>
#include "scanner.h"

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
    //void stop_scanning();
    void select_directory();
    void scan_directory(QString const& dir);
    void show_about_dialog();
    void show_percentage();
    void add_info(const QMap<QString, QPair<QDateTime, QSet<qint32> > >  &_data);
    void add_info(const QVector<QString>  &_data);
private:
    bool is_running();
    void find_word();
    void get_files(const QString &dir, QMap<QString, bool> &was, QVector<QString> &fileList, bool isSearch);
    void show_current();
    QString currentDir;
    int cnt, currentCnt, finishedThreads;
    QProgressBar* progressBar;
    QVector<scanner*> scan;
    QVector<finder*> search;
    QVector<QThread*> threads;
    QMap<QString, QPair<QDateTime, QSet<qint32> > > data;
    QVector<QString> fileNames;
    std::unique_ptr<Ui::MainWindow> ui;
};

#endif // MAINWINDOW_H
