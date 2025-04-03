#ifndef HTTP_H
#define HTTP_H

#include <QEventLoop>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

struct Request
{
  enum class ContentType
  {
    JSON,
    FORM,
  };

  QNetworkRequest req;
  ContentType type;

  Request (QString const &url) : req (url) {}

  Request &
  jwt (QString const &jwt)
  {
    auto token = QString ("Bearer ") + jwt;
    req.setRawHeader ("Authorization", token.toUtf8 ());
    return *this;
  }

  Request &
  form ()
  {
    type = ContentType::FORM;
    req.setHeader (QNetworkRequest::ContentTypeHeader,
                   "application/x-www-form-urlencoded");
    return *this;
  }

  Request &
  json ()
  {
    type = ContentType::JSON;
    req.setHeader (QNetworkRequest::ContentTypeHeader, "application/json");
    return *this;
  }
};

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

public:
  QNetworkReply *
  get (Request const &req)
  {
    auto reply = nam.get (req.req);
    loop.exec ();
    return reply;
  }

  QNetworkReply *
  post (Request const &req, QByteArray const &data)
  {
    auto reply = nam.post (req.req, data);
    loop.exec ();
    return reply;
  }

  QNetworkReply *
  post (Request const &req, QMap<QString, QVariant> const &map)
  {
    auto str = QString ();
    bool is_form = (req.type == Request::ContentType::FORM);

    for (auto it = map.cbegin (); it != map.cend ();)
      {
        str.append (it.key ()).append ("=");
        auto value = it.value ().toString ();
        str += is_form ? QUrl::toPercentEncoding (value) : value;

        if (++it != map.cend ())
          str.append ("&");
      }

    return post (req, str.toUtf8 ());
  }
};

#endif
