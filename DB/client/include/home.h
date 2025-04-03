#ifndef HOME_H
#define HOME_H

#include "ui_home.h"
#include "util.h"

class Home : public QMainWindow
{
  Q_OBJECT

public:
  struct Info
  {
    QString user;
    QString name;
    QString start;
    QString token;
  };

private:
  Ui::Home ui = {};
  Info info;
  Type type;

public:
  Home (Type type, Info info);

private:
  void load_course ();
  void load_grade ();
  void course_new ();
  void grade_mark ();
  void grade_analy ();

private slots:
  void on_pbtn1_clicked ();
  void on_pbtn2_clicked ();
  void on_pbtn3_clicked ();
  void on_pbtn4_clicked ();
  void on_pbtn5_clicked ();
  void on_pbtn6_clicked ();
};

#endif
