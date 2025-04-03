#ifndef MOD_H
#define MOD_H

#include "ui_mod.h"

class Home;

class Mod : public QDialog
{
  Q_OBJECT

private:
  Home *parent;
  Ui::Mod ui = {};

public:
  Mod (Home *parent);
  ~Mod () { setParent (nullptr); }
  int exec () override;

private slots:
  void on_pbtn1_clicked ();
  void on_pbtn2_clicked ();
  void on_pbtn3_clicked ();
};

#endif
