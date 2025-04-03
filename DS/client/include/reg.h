#ifndef REG_H
#define REG_H

#include "ui_reg.h"

class Log;

class Reg : public QDialog
{
  Q_OBJECT

private:
  Log *parent;
  Ui::Reg ui = {};

public:
  Reg (Log *parent);
  int exec () override;

private slots:
  void on_pbtn1_clicked ();
  void on_pbtn2_clicked ();
};

#endif
