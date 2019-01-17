#include "scanner.h"
#include <QString>
//#include <QDebug>
#include <iostream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QCryptographicHash>
#include <atomic>

scanner::scanner(const QString &dir_name) {
    dir = dir_name;
    aborted_flag = false;
    number_of_files = counter = 0;
    now_percentage = 0;
}

void scanner::run() {
    data.clear();
    get_data(dir);
    was.clear();
    if (number_of_files == 0) {
        now_percentage = 100;
        emit percentage(100);
    }
    was.clear();
    if (aborted_flag == false) {
        load_data();
    }
    if (aborted_flag == false)
        emit done(data, dir);
    emit finished();
}

void scanner::change_percentage() {
    counter++;
    while (now_percentage < 100 && (now_percentage + 1) * number_of_files <= counter * 100) {
        now_percentage++;
        emit percentage(now_percentage);
    }
}



void scanner::load_data() {
    const qint64 min_count = 10;
    for (auto v : names) {
        if (aborted_flag == true)
            break;
        if (v.size() == 1) {
            change_percentage();
            continue;
        }
        QVector<QPair<QByteArray, QString> > first_bytes;
        for (auto path : v) {
            if (aborted_flag == true) break;
            char s[min_count];
            QFile file(path);
            if(!file.open(QIODevice::ReadOnly)) {
                //ignore bad files
                change_percentage();
                continue;
            }
            qint64 cnt = std::min(min_count, file.size());
            qint64 get = 0;
            while (get < cnt) {
                qint64 k = file.read(s + get, cnt);
                if (k == -1 || k == 0) {
                    break;
                }
                get += k;
                cnt -= k;
            }
            if (get < std::min(min_count, file.size())) {
                change_percentage();
                continue;
            }
            first_bytes.append({QByteArray(s, get), path});
        }
        std::sort(first_bytes.begin(), first_bytes.end());
        QVector<QString> good_paths;
        for (size_t i = 0; i < first_bytes.size(); i++) {
            if ((i > 0 && first_bytes[i].first == first_bytes[i - 1].first) ||
                    (i + 1 < first_bytes.size() && first_bytes[i].first == first_bytes[i + 1].first)) {
                good_paths.append(first_bytes[i].second);
            }
        }
        QVector<QPair<QString, QString> > sha256;
        for (auto path : v) {
            if (aborted_flag == true)
                break;
            QCryptographicHash hs(QCryptographicHash::Algorithm::Sha256);
            QFile file(path);
            change_percentage();
            if(!file.open(QIODevice::ReadOnly)) {
                //ignore bad files
                continue;
            }

            if (!hs.addData(&file)) {
                continue;
            }

            sha256.push_back({QString(hs.result().toHex()), path});
            //data[SHA256].append(file_info.absoluteFilePath());
        }
        if (aborted_flag == true)
            break;
        std::sort(sha256.begin(), sha256.end());

        for (size_t i = 0; i < sha256.size(); i++) {
            if ((i > 0 && sha256[i].first == sha256[i - 1].first) || (i + 1 < sha256.size() && sha256[i].first == sha256[i + 1].first)) {
                data[sha256[i].first].push_back(sha256[i].second);
            }
        }
    }
}

void scanner::get_data(const QString &dir) {
    if (aborted_flag) return;
    QDir d(dir);
    if (was[d.canonicalPath()]) return;
    was[d.canonicalPath()] = true;


    QFileInfoList list = d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (QFileInfo file_info : list)
    {
        if (aborted_flag) return;
        if (file_info.isSymLink()) {
            get_data(file_info.absoluteFilePath());
        } else
        if (file_info.isDir()) {
            if (QDir(file_info.absoluteFilePath()).isReadable())
                get_data(file_info.absoluteFilePath());
        } else {
            names[QFile(file_info.absoluteFilePath()).size()].append(file_info.absoluteFilePath());
            number_of_files++;
        }
    }
}

QMap<QString, QVector<QString> > scanner::get_map_data() {
    return data;
}

void scanner::set_flag() {
    aborted_flag = true;
}
