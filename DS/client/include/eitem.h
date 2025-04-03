#ifndef EITEM_H
#define EITEM_H

#include "ui_eitem.h"
#include <QListWidgetItem>

struct Eval
{
  int64_t id;
  double grade;
  QString user;
  QString uname;
  QString evaluation;

  Eval &operator= (Eval const &) = default;
  Eval &operator= (Eval &&) = default;
  Eval (Eval const &) = default;
  Eval (Eval &&) = default;
  ~Eval () = default;
};

class EvalItemWidget : public QWidget
{
  Q_OBJECT

private:
  Ui::Eitem ui = {};

public:
  EvalItemWidget (Eval const &data) : QWidget ()
  {
    ui.setupUi (this);
    ui.label1->setText (data.uname);
    ui.label3->setText (data.evaluation);
    ui.label2->setText (QString::number (data.grade, 'f', 1) + " 分");
  }
};

class EvalItem : public QListWidgetItem
{
public:
  Eval data;

  EvalItem (QListWidget *list, Eval eval)
      : QListWidgetItem (list), data (std::move (eval))
  {
    auto widget = new EvalItemWidget (data);
    QListWidgetItem::setSizeHint (widget->sizeHint ());
    list->setItemWidget (this, widget);
  }

  bool
  operator< (QListWidgetItem const &other) const override
  {
    return data.grade < static_cast<EvalItem const &> (other).data.grade;
  }
};

#endif
