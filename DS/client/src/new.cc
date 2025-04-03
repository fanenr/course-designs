#include "new.h"
#include "ditem.h"
#include "home.h"
#include "http.h"
#include "util.h"

New::New (Home *parent, oper op) : QDialog (parent), parent (parent), op (op)
{
  ui.setupUi (this);

  switch (op)
    {
    case oper::NEW:
      ui.pbtn2->setVisible (false);
      setWindowTitle (tr ("新建菜品"));
      ui.pbtn3->setText (tr ("创建"));
      ui.label1->setText (tr ("新建菜品"));
      break;

    case oper::MOD:
      ui.pbtn2->setVisible (true);
      setWindowTitle (tr ("修改菜品"));
      ui.pbtn3->setText (tr ("修改"));
      ui.label1->setText (tr ("修改菜品"));
      break;
    }
}

qint64
New::get_id ()
{
  auto item = parent->ui.list->currentItem ();
  return dynamic_cast<DishItem *> (item)->data.id;
}

void
New::on_pbtn1_clicked ()
{
  close ();
}

void
New::on_pbtn2_clicked ()
{
  auto req_data = QJsonObject ();
  auto req_url = QString (URL_MENU_DEL);

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
New::on_pbtn3_clicked ()
{
  auto name = ui.ledit1->text ();
  auto price_str = ui.ledit2->text ();

  if (name.isEmpty () || price_str.isEmpty ())
    {
      QMessageBox::warning (this, tr ("提示"), tr ("请完整填写信息"));
      return;
    }

  auto req_url = QString ();
  auto req_data = QJsonObject ();
  auto price = price_str.toDouble ();

  req_data["user"] = parent->info["user"];
  req_data["pass"] = parent->info["pass"];

  switch (op)
    {
    case oper::NEW:
      req_data["name"] = name;
      req_data["price"] = price;
      req_url = URL_MENU_NEW;
      break;

    case oper::MOD:
      req_data["nname"] = name;
      req_data["id"] = get_id ();
      req_data["nprice"] = price;
      req_url = URL_MENU_MOD;
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
