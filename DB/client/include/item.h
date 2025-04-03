#ifndef ITEM_H
#define ITEM_H

#include "ui_item.h"
#include <QListWidgetItem>

struct Grade
{
  int id;
  int score;
  QString name;
  QString course;
};

struct Course
{
  int id;
  QString name;
  QString start;
  QString teacher;
};

class GradeWidget : public QWidget
{
  Q_OBJECT

private:
  Ui::Item ui = {};

public:
  GradeWidget (Grade const &data)
  {
    ui.setupUi (this);
    ui.label2->setText (data.name);
    ui.label1->setText (data.course);
    ui.label3->setText (QString::number (data.score));
  }
};

class CourseWidget : public QWidget
{
  Q_OBJECT

private:
  Ui::Item ui = {};

public:
  CourseWidget (Course const &data)
  {
    ui.setupUi (this);
    ui.label1->setText (data.name);
    ui.label2->setText (data.start);
    ui.label3->setText (data.teacher);
  }
};

class GradeItem : public QListWidgetItem
{
public:
  Grade data;

  GradeItem (QListWidget *list, Grade data)
      : QListWidgetItem (list), data (std::move (data))
  {
    auto widget = new GradeWidget (this->data);
    QListWidgetItem::setSizeHint (widget->sizeHint ());
    list->setItemWidget (this, widget);
  }

  bool
  operator< (QListWidgetItem const &other) const override
  {
    return data.score < ((GradeItem const &)other).data.score;
  }
};

class CourseItem : public QListWidgetItem
{
public:
  Course data;

  CourseItem (QListWidget *list, Course data)
      : QListWidgetItem (list), data (std::move (data))
  {
    auto widget = new CourseWidget (this->data);
    QListWidgetItem::setSizeHint (widget->sizeHint ());
    list->setItemWidget (this, widget);
  }

  bool
  operator< (QListWidgetItem const &other) const override
  {
    return data.name < ((CourseItem const &)other).data.name;
  }
};

#endif
