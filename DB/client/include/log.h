#ifndef LOG_H
#define LOG_H

#include "ui_log.h"
#include "util.h"

class Log : public QDialog
{
  Q_OBJECT

  friend class Reg;

private:
  Ui::Log ui = {};

public:
  Log () : QDialog () { ui.setupUi (this); };
  Type category ();

private slots:
  void on_pbtn1_clicked ();
  void on_pbtn2_clicked ();
};

#endif
