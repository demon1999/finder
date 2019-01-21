#ifndef FINDER_H
#define FINDER_H


#include <QMap>
#include <QString>
#include <QVector>
#include <QObject>
#include <QSet>
#include <QDateTime>
#include <QPair>
#include <functional>
#include <atomic>

class finder : public QObject
{
        Q_OBJECT

private:
    std::atomic<bool> aborted_flag;
    QVector<qint32> prefix_function;
    QSet<qint32> indexes;
    QVector<QString> data;
    struct MyInfo {
      QString path;
      QSet<qint32> trigrams;
    };
    QVector<MyInfo> info;
    QString str;
    void calc_indexes();
    void get_prefix_function();
    void change_percentage();
    void find_word();
public:
    finder(const QString& word);
    void set_flag();
    void add_file(const QString &path, const QSet<qint32> &trigrams);
public slots:
    void run();
signals:
    void percentage();
    void done(const QVector<QString> &);
    void finished();
};

#endif // FINDER_H
