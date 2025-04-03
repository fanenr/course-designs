#ifndef NEW_H
#define NEW_H

#include "ui_new.h"
#include "util.h"

class Home;

class New : public QDialog
{
  Q_OBJECT

private:
  oper op;
  Home *parent;
  Ui::New ui = {};

public:
  New (Home *parent, oper op);

private:
  qint64 get_id ();

private slots:
  void on_pbtn1_clicked ();
  void on_pbtn2_clicked ();
  void on_pbtn3_clicked ();
};

#endif
