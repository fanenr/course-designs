#include "log.h"
#include "home.h"
#include "http.h"
#include "reg.h"

type
Log::category ()
{
  if (ui.rbtn1->isChecked ())
    return type::STUDENT;
  if (ui.rbtn2->isChecked ())
    return type::MERCHANT;
  abort ();
}

void
Log::on_pbtn1_clicked ()
{
  auto user = ui.ledit1->text ();
  auto pass = ui.ledit2->text ();

  if (user.isEmpty () || pass.isEmpty ())
    return (void)QMessageBox::warning (this, tr ("提示"),
                                       tr ("请输入帐号密码"));

  auto reg = Reg (this);
  reg.exec ();
}

void
Log::on_pbtn2_clicked ()
{
  auto user = ui.ledit1->text ();
  auto pass = ui.ledit2->text ();

  if (user.isEmpty () || pass.isEmpty ())
    return (void)QMessageBox::warning (this, tr ("提示"),
                                       tr ("请输入帐号密码"));

  auto typ = category ();
  auto req_url = QString ();

  switch (typ)
    {
    case type::STUDENT:
      req_url = URL_STUDENT_LOG;
      break;

    case type::MERCHANT:
      req_url = URL_MERCHANT_LOG;
      break;
    }

  auto req_data = QJsonObject ();
  req_data["user"] = user;
  req_data["pass"] = pass;

  auto http = Http ();
  auto reply = http.post (req_url, req_data);

  auto res = Http::get_data (reply, this);
  if (!res.has_value ())
    return;

  auto info = QMap<QString, QString> ();
  auto map = res.value ()["data"].toObject ().toVariantMap ();
  for (auto it = map.cbegin (); it != map.cend (); it++)
    info.insert (it.key (), it.value ().toString ());

  auto home = new Home (typ, std::move (info));
  home->show ();
  close ();
}
