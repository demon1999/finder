#include "scanner.h"
#include <QString>
#include <deque>
//#include <QDebug>
#include <iostream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QCryptographicHash>
#include <QSet>
#include <QPair>
#include <QDateTime>
#include <atomic>

scanner::scanner() {
    aborted_flag = false;
}

void scanner::add_file(const QString &path) {
    paths.push_back(path);
}

void scanner::run() {
    indexing();
    if (aborted_flag == false)
        emit indexing_finished();
    emit finished();
}

void scanner::change_percentage() {
    emit percentage();
}



void scanner::indexing() {
    for (auto path : paths) {
      if (aborted_flag == true)
          break;
      QFile file(path);
      if(!file.open(QIODevice::ReadOnly)) {
          change_percentage();
          emit done(path, {QDateTime(), QSet<qint32>()});
          continue;
      }
      qint64 cnt = file.size();
      qint64 get = 0;
      QSet<qint32> s;
      const qint64 max_len = qint64(1 << 20), max_count = qint64(2e5);
      char str[max_len];
      int cntt = 0, ck = 0;
      while (get < cnt) {
          qint64 k = file.read(str, std::min(cnt - get, max_len - 1));
          if (k == -1 || k == 0) {
              s.clear();
              break;
          }
          if (aborted_flag == true) {
              s.clear();
              break;
          }
          get += k;
          bool bad_file = false;
          for (int i = 0; i < k; i++) {
              if (str[i] == 0) {
                  bad_file = true;
                  break;
              }
              cntt++;
              ck = (ck & ((1 << 16) - 1)) * 256 + int(str[i]);
              if (cntt >= 3) {
                  cntt = 3;
                  s.insert(ck);
              }
          }
          if (s.size() >= max_count || bad_file == true) {
              s.clear();
              break;
          }
      }
      emit done(path, {QFileInfo(path).lastModified(), s});
      change_percentage();
    }
}


void scanner::set_flag() {
    aborted_flag = true;
}
