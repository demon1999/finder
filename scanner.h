#ifndef SCANNER_H
#define SCANNER_H
#include <QMap>
#include <QString>
#include <QVector>
#include <QObject>
#include <functional>
#include <atomic>

class scanner : public QObject
{
        Q_OBJECT

private:
    std::atomic<bool> aborted_flag;
    int counter;
    int number_of_files;
    int now_percentage;
    QString dir;
    QMap<int, QVector<QString> > names;
    QMap<QString, bool> was;
    QMap<QString, QVector<QString> > data;
    void get_data(const QString &dir);
    void load_data();
    void change_percentage();
public:
    scanner(const QString &dir_name);
    void set_flag();
public slots:
    void run();
    QMap<QString, QVector<QString> > get_map_data();
signals:
    void percentage(int k);
    void done(const QMap<QString, QVector<QString> >  &_data, const QString &_dir);
    void finished();
};
#endif // SCANNER_H
