#include "finder.h"
#include <QString>
#include <deque>
//#include <QDebug>
#include <string>
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

finder::finder(const QString &word) {
    aborted_flag = false;
    str = word;
}

void finder::add_file(const QString &path, const QSet<qint32> &trigrams) {
    info.push_back({path, trigrams});
};

void finder::get_prefix_function() {
    prefix_function.resize(1);
    prefix_function[0] = 0;
    int cur_pref = 0;
    for (int i = 1; i < str.size(); i++) {
        if (aborted_flag == true) break;
        while (cur_pref > 0 && str[i] != str[cur_pref])
            cur_pref = prefix_function[cur_pref - 1];
        if (str[cur_pref] == str[i]) cur_pref++;
        prefix_function.push_back(cur_pref);
    }
}

void finder::calc_indexes() {
    std::string s = str.toStdString();
    for (size_t i = 0; i + 2 < s.size(); i++) {
        if (aborted_flag == false) break;
        indexes.insert(int(s[i]) * 256 * 256 + int(s[i]) * 256 + int(s[i]));
    }
}

void finder::run() {
    data.clear();
    calc_indexes();
    get_prefix_function();
    find_word();
    if (aborted_flag == false)
        emit done(data);
    emit finished();
}

void finder::change_percentage() {
    emit percentage();
}

void finder::find_word() {
    for (auto fileinfo : info) {
      if (aborted_flag == true)
          break;

      bool ok = true;
      for (auto index : indexes) {
          if (fileinfo.trigrams.find(index) == fileinfo.trigrams.end()) {
              ok = false;
              break;
          }
      }

      if (!ok) {
          change_percentage();
          continue;
      }

      QFile file(fileinfo.path);
      if(!file.open(QIODevice::ReadOnly)) {
          change_percentage();
          continue;
      }
      qint64 cnt = file.size();
      qint64 get = 0;
      QSet<qint32> s;
      const qint64 max_len = qint64(1 << 20);
      char buffer[max_len + 100];
      int cur_pref = 0;
      ok = false;
      while (get < cnt) {
          qint64 k = file.read(buffer, std::min(cnt - get, max_len));
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

              while (cur_pref > 0 && (cur_pref == prefix_function.size() || buffer[i] != str[cur_pref])) {
                  cur_pref = prefix_function[cur_pref - 1];
              }
              if (buffer[i] == str[cur_pref])
                  cur_pref++;
              if (cur_pref == str.size()) {
                  ok = true;
                  break;
              }
          }
          if (ok) break;
      }
      if (ok)
          data.push_back(fileinfo.path);
      change_percentage();
    }
}


void finder::set_flag() {
    aborted_flag = true;
}
