#include "home.h"
#include "http.h"
#include "item.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Home::Home (Type type, Info info)
    : QMainWindow (), type (type), info (std::move (info))
{
  ui.setupUi (this);
  setAttribute (Qt::WA_DeleteOnClose);

  ui.info1->setText (this->info.name);
  ui.info2->setText (this->info.user);
  ui.info3->setText (this->info.start);

  if (this->type == Type::TEACHER)
    {
      ui.pbtn5->setText (tr ("创建课程"));
      ui.pbtn6->setText (tr ("更新成绩"));
    }

  connect (ui.sort, &QCheckBox::toggled, [this] (bool dec) {
    ui.list->sortItems (dec ? Qt::DescendingOrder : Qt::AscendingOrder);
  });

  load_course ();
}

void
Home::load_course ()
{
  ui.list->clear ();
  ui.list->clearSelection ();
  ui.gbox2->setTitle (tr ("课程列表"));

  auto url = QString (type == Type::STUDENT ? URL_STUDENT_COURSE_LIST
                                            : URL_TEACHER_COURSE_LIST);

  auto http = Http ();
  auto req = Request (url).form ().jwt (info.token);

  auto reply = http.get (req);
  if (!util::check_reply (reply))
    return;

  auto array = QJsonDocument::fromJson (reply->readAll ()).array ();
  for (auto e : array)
    {
      auto obj = e.toObject ();
      auto item = Course{
        .id = obj["id"].toInt (),
        .name = obj["name"].toString (),
        .start = obj["start"].toString (),
      };
      item.teacher
          = (type == Type::STUDENT) ? obj["teacher"].toString () : info.name;
      new CourseItem (ui.list, std::move (item));
    }
}

void
Home::load_grade ()
{
  ui.list->clear ();
  ui.list->clearSelection ();
  ui.gbox2->setTitle (tr ("成绩列表"));

  auto url = QString (type == Type::STUDENT ? URL_STUDENT_GRADE_LIST
                                            : URL_TEACHER_GRADE_LIST);

  auto http = Http ();
  auto req = Request (url).form ().jwt (info.token);

  auto reply = http.get (req);
  if (!util::check_reply (reply))
    return;

  auto array = QJsonDocument::fromJson (reply->readAll ()).array ();
  for (auto e : array)
    {
      auto obj = e.toObject ();
      auto item = Grade{
        .id = obj["id"].toInt (),
        .score = obj["score"].toInt (),
        .name = obj["name"].toString (),
        .course = obj["course"].toString (),
      };
      new GradeItem (ui.list, std::move (item));
    }
}

void
Home::on_pbtn1_clicked ()
{
}

void
Home::on_pbtn2_clicked ()
{
  close ();
}

void
Home::on_pbtn3_clicked ()
{
  load_course ();
}

void
Home::on_pbtn4_clicked ()
{
  load_grade ();
}

#include "ui_form.h"

void
Home::course_new ()
{
  auto dialog = QDialog (this);
  auto ui = Ui::Form ();
  ui.setupUi (&dialog);

  dialog.setWindowTitle (tr ("创建课程"));
  ui.ledit2->setInputMask ("0000-00-00");
  ui.label->setText (tr ("课程信息"));
  ui.hint1->setText (tr ("课程名称"));
  ui.hint2->setText (tr ("开课时间"));

  connect (ui.pbtn1, &QPushButton::clicked, [&] { dialog.close (); });

  connect (ui.pbtn2, &QPushButton::clicked, [&, this] {
    auto name = ui.ledit1->text ();
    auto start = ui.ledit2->text ();
    if (name.isEmpty () || start.isEmpty ())
      return (void)QMessageBox::warning (nullptr, tr ("提示"),
                                         tr ("请完整填写信息"));

    auto url = QString (URL_TEACHER_COURSE_CREATE);
    auto data = QMap<QString, QVariant>{
      { "start", std::move (start) },
      { "name", std::move (name) },
    };

    auto http = Http ();
    auto req = Request (url).form ().jwt (info.token);

    auto reply = http.post (req, data);
    if (!util::check_reply (reply))
      return;

    QMessageBox::information (nullptr, tr ("成功"), tr ("创建成功"));
    dialog.close ();
    load_course ();
  });

  dialog.exec ();
}

void
Home::on_pbtn5_clicked ()
{
  if (type == Type::TEACHER)
    return course_new ();

  auto item = (CourseItem *)ui.list->currentItem ();
  if (item == nullptr)
    return;

  auto url = QString (URL_STUDENT_COURSE_TAKE);
  auto data = QMap<QString, QVariant>{
    { "cid", item->data.id },
  };

  auto http = Http ();
  auto req = Request (url).form ().jwt (info.token);

  auto reply = http.post (req, data);
  if (!util::check_reply (reply))
    return;

  QMessageBox::information (nullptr, tr ("成功"), tr ("选修成功"));
}

#include "ui_grade.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

void
Home::grade_analy ()
{
  load_grade ();

  auto dialog = QDialog (this);
  auto ui = Ui::Grade ();
  ui.setupUi (&dialog);

  auto list = this->ui.list;
  auto count = list->count ();
  auto items = QList<GradeItem *> ();

  for (int i = 0; i < count; i++)
    items.append ((GradeItem *)list->item (i));

  long sum_grade = 0;
  double sum_point = 0;

  auto chart = new QChart ();
  auto view = new QChartView ();
  auto series = new QPieSeries ();

  auto points = QMap<double, int> ();
  points[0.0] = 0;
  points[1.0] = 0;
  points[1.5] = 0;
  points[2.0] = 0;
  points[2.3] = 0;
  points[2.7] = 0;
  points[3.0] = 0;
  points[3.3] = 0;
  points[3.7] = 0;
  points[4.0] = 0;

  for (auto i : items)
    {
      auto s = i->data.score;
      sum_grade += s;

      if (s < 60)
        points[0.0]++;
      else if (s < 64)
        points[1.0]++;
      else if (s < 68)
        points[1.5]++;
      else if (s < 72)
        points[2.0]++;
      else if (s < 75)
        points[2.3]++;
      else if (s < 78)
        points[2.7]++;
      else if (s < 82)
        points[3.0]++;
      else if (s < 85)
        points[3.3]++;
      else if (s < 90)
        points[3.7]++;
      else
        points[4.0]++;
    }

  auto on_slice_hovered = [this] (bool sts) {
    auto slice = (QPieSlice *)QObject::sender ();
    slice->setExploded (sts);
  };

  for (auto it = points.cbegin (); it != points.cend (); it++)
    {
      auto key = it.key ();
      auto value = it.value ();
      sum_point += key * value;

      if (value != 0)
        {
          auto rate = value / (double)count;
          auto label = QString::number (key, 'f', 1);
          auto slice = new QPieSlice (label, rate, series);
          connect (slice, &QPieSlice::hovered, this, on_slice_hovered);

          series->append (slice);
        }
    }

  auto avg_grade = count ? sum_grade / count : 0;
  auto avg_point = count ? sum_point / (double)count : 0.0;
  auto rate = count ? (count - points[0.0]) / (double)count : 0.0;

  ui.info1->setText (QString::number (count));
  ui.info2->setText (QString::number (avg_grade));
  ui.info3->setText (QString::number (rate, 'f', 1));
  ui.info4->setText (QString::number (avg_point, 'f', 1));

  series->setHoleSize (0.3);
  series->setLabelsVisible (true);

  chart->addSeries (series);
  chart->setTitle (tr ("成绩分布"));
  chart->legend ()->setVisible (true);
  chart->legend ()->setAlignment (Qt::AlignRight);

  view->setChart (chart);
  view->setRenderHint (QPainter::Antialiasing);

  ui.vlay2->addWidget (view);

  dialog.exec ();
}

void
Home::grade_mark ()
{
  auto item = (GradeItem *)ui.list->currentItem ();
  if (item == nullptr)
    return;

  auto dialog = QDialog (this);
  auto ui = Ui::Form ();
  ui.setupUi (&dialog);

  dialog.setWindowTitle (tr ("更新成绩"));
  ui.label->setText (tr ("成绩信息"));
  ui.hint1->setText (tr ("新成绩"));
  ui.ledit2->setVisible (false);
  ui.hint2->setVisible (false);

  connect (ui.pbtn1, &QPushButton::clicked, [&] { dialog.close (); });

  connect (ui.pbtn2, &QPushButton::clicked, [&, this] {
    auto score = ui.ledit1->text ();
    if (score.isEmpty ())
      return (void)QMessageBox::warning (nullptr, tr ("提示"),
                                         tr ("请完整填写信息"));

    auto url = QString (URL_TEACHER_GRADE_UPDATE);
    auto data = QMap<QString, QVariant>{
      { "score", std::move (score) },
      { "gid", item->data.id },
    };

    auto http = Http ();
    auto req = Request (url).form ().jwt (info.token);

    auto reply = http.post (req, data);
    if (!util::check_reply (reply))
      return;

    QMessageBox::information (nullptr, tr ("成功"), tr ("更新成功"));
    dialog.close ();
    load_grade ();
  });

  dialog.exec ();
}

void
Home::on_pbtn6_clicked ()
{
  type == Type::STUDENT ? grade_analy () : grade_mark ();
}
