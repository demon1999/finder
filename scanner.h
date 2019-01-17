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
    QVector<QString> paths;
    QMap<QString, QPair<QDateTime, QSet<qint32> > > data;
    void change_percentage();
    void load_data();
public:
    scanner();
    void set_flag();
    void add_file(const QString &path);
public slots:
    void run();
signals:
    void percentage();
    void done(const QMap<QString, QPair<QDateTime, QSet<qint32> > > &);
    void finished();
};
#endif // SCANNER_H
