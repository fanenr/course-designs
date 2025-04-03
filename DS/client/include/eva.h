#ifndef EVA_H
#define EVA_H

#include "ui_eva.h"
#include "util.h"

class Home;

class Eva : public QDialog
{
  Q_OBJECT

private:
  oper op;
  Home *parent;
  Ui::Eva ui = {};

public:
  Eva (Home *parent, oper op);

private:
  qint64 get_id ();

private slots:
  void on_pbtn1_clicked ();
  void on_pbtn2_clicked ();
  void on_pbtn3_clicked ();
};

#endif