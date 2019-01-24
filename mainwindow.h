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
#include <QFileSystemWatcher>
#include <memory>
#include <iterator>
#include "scanner.h"
#include "finder.h"
#include "data_saver.h"

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
    void show_percentage(int k);
    void select_directory(QString const &text);
    void add_info(const QVector<QString>  &_data);
    void find_word();
private:
    void show_current();

    bool is_running;
    QString currentDir;
    QString textToFind;
    QLabel *label;
    QProgressBar* progressBar;

    QMap<QString, QSet<qint32> > data;
    data_saver* myData;
    QThread* threadWithData;
    QVector<QString> fileNames;
    std::unique_ptr<Ui::MainWindow> ui;
signals:
    void start_scanning(QString);
    void give_files(QString);
};

#endif // MAINWINDOW_H
