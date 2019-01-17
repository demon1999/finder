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
    data.clear();
    load_data();
    if (aborted_flag == false)
        emit done(data);
    emit finished();
}

void scanner::change_percentage() {
    emit percentage();
}



void scanner::load_data() {
    for (auto path : paths) {
      //change_percentage();
      //continue;
      if (aborted_flag == true)
          break;
      QFile file(path);
      data[path] = {QDateTime(), QSet<qint32>()};
      if(!file.open(QIODevice::ReadOnly)) {
          change_percentage();
          continue;
      }
      qint64 cnt = file.size();
      qint64 get = 0;
      QSet<qint32> s;
      const qint64 max_len = qint64(1e5), max_count = qint64(2e5);
      std::deque<unsigned int> q;
      char str[max_len + 100];
      while (get < cnt) {
          qint64 k = file.read(str, std::min(cnt - get, max_len));
          if (k == -1 || k == 0) {
              s.clear();
              break;
          }
          if (aborted_flag == true) {
              s.clear();
              break;
          }
          get += k;
          for (int i = 0; i < k; i++) {
              if (q.size() == 3)
                  q.pop_front();
              q.push_back((unsigned int)str[i]);
              if (q.size() == 3) {
                  s.insert(q.front() * 256 * 256 + q.back() + (q.begin()[1]) * 256);
              }
          }
          if (s.size() >= max_count) {
              s.clear();
              break;
          }
      }
      data[path] = {QFileInfo(path).lastModified(), s};
      change_percentage();
    }
}


void scanner::set_flag() {
    aborted_flag = true;
}
