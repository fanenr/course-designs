#ifndef DITEM_H
#define DITEM_H

#include "ui_ditem.h"
#include <QListWidgetItem>

struct Dish
{
  int64_t id;
  double price;
  QString name;
  QString user;
  QString uname;
  QString position;

  Dish &operator= (Dish const &) = default;
  Dish &operator= (Dish &&) = default;
  Dish (Dish const &) = default;
  Dish (Dish &&) = default;
  ~Dish () = default;
};

class DishItemWidget : public QWidget
{
  Q_OBJECT

private:
  Ui::Ditem ui = {};

public:
  DishItemWidget (Dish const &data) : QWidget ()
  {
    ui.setupUi (this);
    ui.label1->setText (data.name);
    ui.label3->setText (data.uname);
    ui.label4->setText (data.position);
    ui.label2->setText (QString::number (data.price, 'f', 1) + " 元");
  }
};

class DishItem : public QListWidgetItem
{
public:
  Dish data;

  DishItem (QListWidget *list, Dish dish)
      : QListWidgetItem (list), data (std::move (dish))
  {
    auto widget = new DishItemWidget (data);
    QListWidgetItem::setSizeHint (widget->sizeHint ());
    list->setItemWidget (this, widget);
  }

  bool
  operator< (QListWidgetItem const &other) const override
  {
    return data.price < static_cast<DishItem const &> (other).data.price;
  }
};

#endif
