#include "eva.h"
#include "ditem.h"
#include "home.h"
#include "http.h"
#include "util.h"

Eva::Eva (Home *parent, oper op) : QDialog (parent), parent (parent), op (op)
{
  ui.setupUi (this);

  switch (op)
    {
    case oper::NEW:
      ui.pbtn2->setVisible (false);
      setWindowTitle (tr ("添加评价"));
      ui.label1->setText (tr ("添加评价"));
      break;

    case oper::MOD:
      ui.pbtn2->setVisible (true);
      setWindowTitle (tr ("修改评价"));
      ui.label1->setText (tr ("修改评价"));
      break;
    }
}

qint64
Eva::get_id ()
{
  auto item = parent->ui.list->currentItem ();
  return dynamic_cast<DishItem *> (item)->data.id;
}

void
Eva::on_pbtn1_clicked ()
{
  close ();
}

void
Eva::on_pbtn2_clicked ()
{
  auto req_data = QJsonObject ();
  auto req_url = QString (URL_EVA_DEL);

  req_data["id"] = get_id ();
  req_data["user"] = parent->info["user"];
  req_data["pass"] = parent->info["pass"];

  auto http = Http ();
  auto reply = http.post (req_url, req_data);
  if (!Http::get_data (reply, this).has_value ())
    return;

  QMessageBox::information (this, tr ("提示"), tr ("操作成功，返回查看"));
  parent->load_dish ();
  close ();
}

void
Eva::on_pbtn3_clicked ()
{
  auto grade = ui.spin->value ();
  auto evaluation = ui.edit->toPlainText ();

  if (evaluation.isEmpty ())
    {
      QMessageBox::warning (this, tr ("提示"), tr ("请完整填写信息"));
      return;
    }

  auto req_url = QString ();
  auto req_data = QJsonObject ();

  req_data["id"] = get_id ();
  req_data["user"] = parent->info["user"];
  req_data["pass"] = parent->info["pass"];

  switch (op)
    {
    case oper::NEW:
      req_data["grade"] = grade;
      req_data["evaluation"] = evaluation;
      req_url = URL_EVA_NEW;
      break;

    case oper::MOD:
      req_data["ngrade"] = grade;
      req_data["nevaluation"] = evaluation;
      req_url = URL_EVA_MOD;
      break;
    }

  auto http = Http ();
  auto reply = http.post (req_url, req_data);
  if (!Http::get_data (reply, this).has_value ())
    return;

  QMessageBox::information (this, tr ("提示"), tr ("操作成功，返回查看"));
  parent->load_dish ();
  close ();
}
