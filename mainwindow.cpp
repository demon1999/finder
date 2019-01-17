#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scanner.h"

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
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    thread = new QThread;
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &main_window::prepare_menu);
    QCommonStyle style;
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionPrev_Group_Of_Dublicates->setIcon(style.standardIcon(QCommonStyle::SP_ArrowLeft));
    ui->actionNext_Group_Of_Dublicates->setIcon(style.standardIcon(QCommonStyle::SP_ArrowRight));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    ui->treeWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    connect(ui->actionDeleteSelected,&QAction::triggered,this, &main_window::delete_selected);
    connect(ui->actionStop, &QAction::triggered, this, &main_window::stop_scanning);
    connect(ui->actionPrev_Group_Of_Dublicates, &QAction::triggered, this, &main_window::show_prev_dublicates);
    connect(ui->actionNext_Group_Of_Dublicates, &QAction::triggered, this, &main_window::show_next_dublicates);
    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);

}

main_window::~main_window()
{}

void main_window::delete_selected() {
    QList<QTreeWidgetItem*> sel_items = ui->treeWidget->selectedItems();
    for(int i=0; i<sel_items.size(); i++){
        QFile(sel_items[i]->text(0)).remove();
    }
    try_to_show([this](){return this->increment();});
}
void main_window::prepare_menu(const QPoint & pos) {
    QTreeWidget *tree = ui->treeWidget;

    QTreeWidgetItem *current_item = tree->itemAt( pos );
    QAction *delete_action = new QAction(QIcon(":/Resource/warning32.ico"), tr("&Delete"), this);

    connect(delete_action, &QAction::triggered, this, [current_item, this]() {
                delete_element(current_item);});


    QMenu menu(this);
    menu.addAction(delete_action);

    QPoint pt(pos);
    menu.exec( tree->mapToGlobal(pos) );
}

void main_window::try_to_show(std::function<void()> change) {
    while (true) {
        if (data.empty()) {
            current = data.begin();
            break;
        }
        auto& v = *current;
        v.erase(std::remove_if(v.begin(), v.end(), [](const QString &path) {
            return !QFile(path).exists();
        }), v.end());
        if (v.size() > 1) {
            break;
        } else {
            auto it = current;
            it++;
            if (it == data.end() && current == data.begin()) {
                data.clear();
                current = data.begin();
                break;
            }
            it--;
            change();
            data.erase(it);
        }
    }
    show_current();
}

void main_window::delete_element(QTreeWidgetItem *deleted) {
    QString path = deleted->text(0);
    if (!QFile(path).exists()) {
        QMessageBox::information(nullptr, "error", "File doesn't exist.");
    } else
    if (!QFile(path).remove()) {
        QMessageBox::information(nullptr, "error", "Can't be deleted.");
    }
    try_to_show([this](){return this->increment();});
}

void main_window::increment() {
    current++;
    if (current == data.end())
        current = data.begin();
}

void main_window::decrement() {
    if (current == data.begin())
        current = data.end();
    current--;
}

void main_window::show_next_dublicates() {
    if (data.empty()) {
        current = data.begin();
    } else
        increment();
    try_to_show([this](){return this->increment();});
}

void main_window::show_prev_dublicates() {
    if (data.empty()) return;
    decrement();
    try_to_show([this](){return this->decrement();});
}

void main_window::select_directory()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    scan_directory(dir);
}

void main_window::stop_scanning() {
    if (scan == nullptr)
        return;
    scan->set_flag();
    progressBar->hide();
}

void main_window::scan_directory(QString const& dir)
{
    if (thread->isRunning()) {
        QMessageBox::information(nullptr, "info", "You can't start new scanning, before previous one has finished.");
        return;
    };

    progressBar->show();
    progressBar->setValue(0);
    QDir d(dir);

    scan = new scanner(dir);
    scan->moveToThread(thread);
    connect(scan, SIGNAL(percentage(int)), this, SLOT(show_percentage(int)));
    connect(thread, SIGNAL(started()), scan, SLOT(run()));
    connect(scan, SIGNAL(finished()), thread, SLOT(quit()));

    qRegisterMetaType<QMap<QString, QVector<QString> > >("QMap<QString, QVector<QString> >");
    qRegisterMetaType<QMap<QString, QString> >("QMap<QString, QString>");
    connect(scan, SIGNAL(done(const QMap<QString, QVector<QString> > &,
                              const QString&)), this,
                  SLOT(make_window(const QMap<QString, QVector<QString> > &, const QString&)));
    thread->start();
}

void main_window::show_percentage(int k) {
    progressBar->setValue(k);
    progressBar->show();
}

void main_window::make_window(const QMap<QString, QVector<QString> >  &_data, const QString &_dir) {
    ui->treeWidget->clear();

    progressBar->hide();
    setWindowTitle(QString("Directory Content - %1").arg(_dir));
    int cnt = 0;
    for (auto v : _data)
        cnt += v.size();
    data = _data;
    current = data.begin();
    try_to_show([this](){return this->increment();});
}
void main_window::show_current() {
    QString title = QWidget::windowTitle();
    ui->treeWidget->clear();
    setWindowTitle(title);

    if (current == data.end()) {
        return;
    }
    auto v = *current;
    for (auto eq_files : v) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, eq_files);
        item->setText(1, QString::number(QFile(eq_files).size()));
        ui->treeWidget->addTopLevelItem(item);
    }
}

void main_window::show_about_dialog()
{
    QMessageBox::aboutQt(this);
}
