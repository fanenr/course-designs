#ifndef HTTP_H
#define HTTP_H

#include <optional>

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class Http : QObject
{
  Q_OBJECT

private:
  QEventLoop loop;
  QNetworkAccessManager nam;

public:
  Http (QObject *parent = nullptr) : QObject (parent)
  {
    connect (&nam, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
  }

  ~Http () = default;

public:
  static QNetworkRequest
  json_req (QString const &url)
  {
    auto result = QNetworkRequest (QUrl (url));
    result.setHeader (QNetworkRequest::ContentTypeHeader, "application/json");
    return result;
  }

  QNetworkReply *
  post (QString const &url, QJsonObject const &data)
  {
    auto req = json_req (url);
    auto dat = QJsonDocument (data).toJson ();
    auto reply = nam.post (req, dat);
    loop.exec ();
    return reply;
  }

public:
  template <typename Slot>
  static QNetworkAccessManager *
  post (QString const &url, QJsonObject const &data, QObject const *recv,
        Slot &&slot, QNetworkAccessManager *nam = nullptr)
  {
    if (!nam)
      {
        nam = new QNetworkAccessManager;
        connect (nam, &QNetworkAccessManager::finished, recv,
                 std::forward<Slot> (slot));
      }

    auto req = json_req (url);
    auto dat = QJsonDocument (data).toJson ();
    auto reply = nam->post (req, dat);
    return nam;
  }

  template <typename Slot>
  static QNetworkAccessManager *
  post (QString const &url, QJsonObject const &data, Slot &&slot,
        QNetworkAccessManager *nam = nullptr)
  {
    if (!nam)
      {
        nam = new QNetworkAccessManager;
        connect (nam, &QNetworkAccessManager::finished,
                 std::forward<Slot> (slot));
      }

    auto req = json_req (url);
    auto dat = QJsonDocument (data).toJson ();
    auto reply = nam->post (req, dat);
    return nam;
  }

  static std::optional<QJsonObject>
  get_data (QNetworkReply *reply, QWidget *ctx)
  {
    if (reply->error ())
      {
        QMessageBox::warning (ctx, tr ("失败"), tr ("无法发送网络请求"));
        return std::nullopt;
      }

    auto obj = QJsonDocument::fromJson (reply->readAll ()).object ();
    if (obj["code"] != 0)
      {
        QMessageBox::warning (ctx, tr ("失败"),
                              obj["data"].toString (tr ("信息丢失")));
        return std::nullopt;
      }

    return obj;
  }
};

#endif