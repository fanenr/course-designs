#include "ui_app.h"
#include <QUdpSocket>

QString resolveDomain (const QString &domain);
QByteArray createDnsQuery (const QString &domain);
QString parseDnsResponse (const QByteArray &response);

int
main (int argc, char **argv)
{
  auto app = QApplication (argc, argv);
  auto win = QMainWindow ();
  auto ui = Ui::window ();

  ui.setupUi (&win);
  win.show ();

  bool querying = false;
  QObject::connect (ui.btn, &QPushButton::clicked, [&] {
    if (querying)
      return;
    querying = true;

    auto domain = ui.edit->text ();
    auto ip = resolveDomain (domain);
    ui.text->setText (ip);

    querying = false;
  });

  return app.exec ();
}

QString
resolveDomain (const QString &domain)
{
  auto socket = QUdpSocket ();

  // 创建 DNS 查询包
  auto query = createDnsQuery (domain);

  // 请求 DNS 服务器
  socket.writeDatagram (query, QHostAddress ("8.8.8.8"), 53);

  // 等待响应，最多 5 秒
  if (!socket.waitForReadyRead (5000))
    return "查询超时或失败";

  auto response = QByteArray ();
  response.resize (socket.pendingDatagramSize ());
  socket.readDatagram (response.data (), response.size ());

  // 解析响应
  return parseDnsResponse (response);
}

QByteArray
createDnsQuery (const QString &domain)
{
  auto query = QByteArray ();
  auto stream = QDataStream (&query, QIODevice::WriteOnly);
  stream.setByteOrder (QDataStream::BigEndian);

  // 写入 DNS 头部 (12 字节)
  stream << (quint16) 0x0000; // 查询 ID (2 字节)
  stream << (quint16) 0x0100; // 标志：标准查询 (2 字节)
  stream << (quint16) 0x0001; // 问题数量：1 (2 字节)
  stream << (quint16) 0x0000; // 回答 RR 数量 (2 字节)
  stream << (quint16) 0x0000; // 授权 RR 数量 (2 字节)
  stream << (quint16) 0x0000; // 额外 RR 数量 (2 字节)

  // 将域名转换为 DNS 格式：每段前加长度字节
  auto parts = domain.split (".");
  for (auto const &part : parts)
    {
      auto bytes = part.toUtf8 ();
      stream << (quint8) bytes.length (); // 长度字节
      stream.writeRawData (bytes.data (),
			   bytes.size ()); // 写入部分域名
    }

  // 域名结束符
  stream << (quint8) 0;

  // 查询类型和查询类
  stream << (quint16) 0x0001; // A 记录类型 (2 字节)
  stream << (quint16) 0x0001; // IN 类 (2 字节)

  return query;
}

QString
parseDnsResponse (const QByteArray &response)
{
  // 响应长度检查
  if (response.size () < 12)
    return "响应格式错误: 数据包过短";

  // 创建数据流进行解析
  auto stream = QDataStream (response);
  stream.setByteOrder (QDataStream::BigEndian);

  // 跳过 Transaction ID (2 字节)
  stream.skipRawData (2);

  // 检查响应码 (RCODE 在标志字段的低 4 位)
  quint16 flags;
  stream >> flags;
  quint8 rcode = flags & 0x0F;
  if (rcode != 0)
    return QString ("DNS错误码: %1").arg (rcode);

  // 跳过问题数量 (2 字节)
  stream.skipRawData (2);

  // 读取回答记录数量
  quint16 ancount;
  stream >> ancount;
  if (ancount == 0)
    return "没有找到记录";

  // 跳过授权和附加记录数量 (4 字节)
  stream.skipRawData (4);

  // 跳过问题部分
  int pos = 12;
  while (pos < response.size ())
    {
      auto len = (quint8) response[pos];

      if (len == 0) // 域名结束
	{
	  pos++;
	  break;
	}

      pos += len + 1;
    }

  // 跳过查询类型和查询类 (4 字节)
  pos += 4;

  // 解析回答部分
  for (int i = 0; i < ancount; i++)
    {
      // 跳过域名指针 (2 字节)
      pos += 2;

      // 获取记录类型
      auto type = ((quint8) response[pos] << 8) | (quint8) response[pos + 1];
      pos += 2;

      // 跳过记录类 (2 字节)
      pos += 2;

      // 跳过 TTL (4 字节)
      pos += 4;

      // 获取数据长度
      auto len = ((quint8) response[pos] << 8) | (quint8) response[pos + 1];
      pos += 2;

      // 如果是 A 记录 (IPv4)
      if (type == 1 && len == 4)
	{
	  auto ip = QHostAddress (((quint8) response[pos] << 24)
				  | ((quint8) response[pos + 1] << 16)
				  | ((quint8) response[pos + 2] << 8)
				  | (quint8) response[pos + 3]);

	  return ip.toString ();
	}

      // 跳过该条目的数据部分
      pos += len;
    }

  return "没有找到 A 记录";
}
