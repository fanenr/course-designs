#include "log.h"
#include "home.h"
#include "http.h"

#include <QJsonDocument>
#include <QJsonObject>

Type
Log::category ()
{
  if (ui.rbtn1->isChecked ())
    return Type::STUDENT;
  if (ui.rbtn2->isChecked ())
    return Type::TEACHER;
  abort ();
}

#include "ui_reg.h"

void
Log::on_pbtn1_clicked ()
{
  auto user = ui.ledit1->text ();
  auto pass = ui.ledit2->text ();

  if (user.isEmpty () || pass.isEmpty ())
    return (void)QMessageBox::warning (nullptr, tr ("提示"),
                                       tr ("请输入帐号密码"));

  auto dialog = QDialog (this);
  auto ui = Ui::Reg ();
  ui.setupUi (&dialog);

  connect (ui.pbtn1, &QPushButton::clicked, [&] { dialog.close (); });

  connect (ui.pbtn2, &QPushButton::clicked, [&, this] {
    auto type = category ();
    auto name = ui.ledit1->text ();
    auto date = ui.ledit2->text ();

    if (name.isEmpty () || date.isEmpty ())
      return (void)QMessageBox::warning (nullptr, tr ("提示"),
                                         tr ("请完整填写信息"));

    auto url = QString (type == Type::STUDENT ? URL_STUDENT_REGISTER
                                              : URL_TEACHER_REGISTER);

    auto data = QMap<QString, QVariant>{
      { "username", std::move (user) },
      { "password", std::move (pass) },
      { "start", std::move (date) },
      { "name", std::move (name) },
    };

    auto http = Http ();
    auto req = Request (url).form ();
    auto reply = http.post (req, data);

    if (!util::check_reply (reply))
      return;

    QMessageBox::information (nullptr, tr ("成功"),
                              tr ("注册成功，请返回登录"));
    dialog.close ();
  });

  dialog.exec ();
}

void
Log::on_pbtn2_clicked ()
{
  auto user = ui.ledit1->text ();
  auto pass = ui.ledit2->text ();

  if (user.isEmpty () || pass.isEmpty ())
    return (void)QMessageBox::warning (nullptr, tr ("提示"),
                                       tr ("请输入帐号密码"));

  auto type = category ();
  auto url = QString (type == Type::STUDENT ? URL_STUDENT_LOGIN
                                            : URL_TEACHER_LOGIN);

  auto data = QMap<QString, QVariant>{
    { "password", std::move (pass) },
    { "username", user },
  };

  auto http = Http ();
  auto req = Request (url).form ();
  auto reply = http.post (req, data);

  if (!util::check_reply (reply))
    return;

  auto obj = QJsonDocument::fromJson (reply->readAll ()).object ();
  auto info = Home::Info{
    .user = std::move (user),
    .name = obj["name"].toString (),
    .start = obj["start"].toString (),
    .token = obj["token"].toString (),
  };

  auto home = new Home (type, std::move (info));
  home->show ();
  close ();
}
