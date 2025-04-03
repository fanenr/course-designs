#include "mod.h"
#include "home.h"
#include "http.h"
#include "util.h"

Mod::Mod (Home *parent) : QDialog (parent)
{
  ui.setupUi (this);
  this->parent = parent;

  switch (parent->typ)
    {
    case type::STUDENT:
      ui.label3->setText (tr ("新昵称"));
      ui.label5->hide ();
      ui.ledit4->hide ();
      break;

    case type::MERCHANT:
      ui.label3->setText (tr ("新名称"));
      break;
    }
}

int
Mod::exec ()
{
  ui.ledit1->setText (parent->info["pass"]);
  ui.ledit2->setText (parent->info["name"]);
  ui.ledit3->setText (parent->info["number"]);

  if (parent->typ == type::MERCHANT)
    ui.ledit4->setText (parent->info["position"]);

  return QDialog::exec ();
}

void
Mod::on_pbtn1_clicked ()
{
  close ();
}

void
Mod::on_pbtn2_clicked ()
{
  auto req_url = QString ();
  auto req_data = QJsonObject ();

  req_data["user"] = parent->info["user"];
  req_data["pass"] = parent->info["pass"];

  switch (parent->typ)
    {
    case type::STUDENT:
      req_url = URL_STUDENT_DEL;
      break;

    case type::MERCHANT:
      req_url = URL_MERCHANT_DEL;
      break;
    }

  auto http = Http ();
  auto reply = http.post (req_url, req_data);
  if (!Http::get_data (reply, this).has_value ())
    return;

  QMessageBox::information (this, tr ("提示"), tr ("注销成功"));
  parent->close ();
}

void
Mod::on_pbtn3_clicked ()
{
  auto npass = ui.ledit1->text ();
  auto nname = ui.ledit2->text ();
  auto nnumber = ui.ledit3->text ();
  auto nposition = ui.ledit4->text ();

  auto typ = parent->typ;

  if (npass.isEmpty () || nname.isEmpty () || nnumber.isEmpty ()
      || (nposition.isEmpty () && typ == type::MERCHANT))
    {
      QMessageBox::warning (this, tr ("提示"), tr ("请完整填写信息"));
      return;
    }

  auto &info = parent->info;
  auto req_url = QString ();
  auto req_data = QJsonObject ();

  req_data["user"] = info["user"];
  req_data["pass"] = info["pass"];

  req_data["npass"] = npass;
  req_data["nname"] = nname;
  req_data["nnumber"] = nnumber;

  switch (typ)
    {
    case type::STUDENT:
      req_url = URL_STUDENT_MOD;
      break;

    case type::MERCHANT:
      req_url = URL_MERCHANT_MOD;
      req_data["nposition"] = nposition;
      break;
    }

  auto http = Http ();
  auto reply = http.post (req_url, req_data);
  if (!Http::get_data (reply, this).has_value ())
    return;

  info["pass"] = std::move (npass);
  info["name"] = std::move (nname);
  info["number"] = std::move (nnumber);
  if (typ == type::MERCHANT)
    info["position"] = std::move (nposition);

  parent->load_info ();
  parent->load_dish ();
  close ();
}
