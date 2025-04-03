#ifndef UTIL_H
#define UTIL_H

#define URL_BASE "http://127.0.0.1:5000"
#define URL_STUDENT URL_BASE "/student"
#define URL_TEACHER URL_BASE "/teacher"

#define URL_STUDENT_LOGIN URL_STUDENT "/login"
#define URL_STUDENT_REGISTER URL_STUDENT "/register"
#define URL_STUDENT_GRADE_LIST URL_STUDENT "/grade/list"
#define URL_STUDENT_COURSE_LIST URL_STUDENT "/course/list"
#define URL_STUDENT_COURSE_TAKE URL_STUDENT "/course/take"

#define URL_TEACHER_LOGIN URL_TEACHER "/login"
#define URL_TEACHER_REGISTER URL_TEACHER "/register"
#define URL_TEACHER_GRADE_LIST URL_TEACHER "/grade/list"
#define URL_TEACHER_COURSE_LIST URL_TEACHER "/course/list"
#define URL_TEACHER_GRADE_UPDATE URL_TEACHER "/grade/update"
#define URL_TEACHER_COURSE_CREATE URL_TEACHER "/course/create"

enum class Type
{
  STUDENT,
  TEACHER,
};

#include <QMessageBox>
#include <QtNetwork/QNetworkReply>

namespace util
{

inline bool
check_reply (QNetworkReply *reply)
{
  if (reply->error () == QNetworkReply::NoError)
    return true;

  auto sts = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute);
  if (!sts.isValid ())
    {
      QMessageBox::warning (nullptr, QObject::tr ("错误"),
                            QObject::tr ("网络错误：无法发送网络请求"));
      return false;
    }

  auto msg = QString::fromUtf8 (reply->readAll ());
  auto body = QString ("%1（%2）：%3")
                  .arg (QObject::tr ("响应错误"))
                  .arg (sts.toInt ())
                  .arg (msg);

  QMessageBox::warning (nullptr, QObject::tr ("错误"), body);
  return false;
}

};

#endif
