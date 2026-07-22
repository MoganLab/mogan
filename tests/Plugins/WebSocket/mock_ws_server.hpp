#ifndef MOCK_WS_SERVER_HPP
#define MOCK_WS_SERVER_HPP

#include <QByteArray>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

class MockWsServer : public QTcpServer {
public:
  MockWsServer (QObject* parent= nullptr) : QTcpServer (parent) {}

  QTcpSocket* clientSocket= nullptr;

protected:
  void incomingConnection (qintptr socketDescriptor) override {
    clientSocket= new QTcpSocket (this);
    clientSocket->setSocketDescriptor (socketDescriptor);
    connect (clientSocket, &QTcpSocket::readyRead, this,
             &MockWsServer::onReadyRead);
  }

public:
  void onReadyRead () {
    if (!clientSocket) return;
    QByteArray data= clientSocket->readAll ();

    if (data.contains ("Upgrade: websocket")) {
      // Handshake
      QRegularExpression      re ("Sec-WebSocket-Key:\\s*(.+)");
      QRegularExpressionMatch match= re.match (QString::fromUtf8 (data));
      if (match.hasMatch ()) {
        QString    key      = match.captured (1).trimmed ();
        QString    magic    = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        QByteArray hash     = QCryptographicHash::hash ((key + magic).toUtf8 (),
                                                        QCryptographicHash::Sha1);
        QString    acceptKey= hash.toBase64 ();

        QString response= "HTTP/1.1 101 Switching Protocols\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Accept: " +
                          acceptKey + "\r\n\r\n";
        clientSocket->write (response.toUtf8 ());
        clientSocket->flush ();
      }
    }
    else {
      if (data.size () < 2) return;

      unsigned char byte0= data[0];
      unsigned char byte1= data[1];

      int opcode    = byte0 & 0x0F;
      int payloadLen= byte1 & 0x7F;

      int offset= 2;
      if (payloadLen == 126) {
        offset+= 2;
      }
      else if (payloadLen == 127) {
        offset+= 8;
      }

      bool       masked= (byte1 & 0x80) != 0;
      QByteArray maskingKey;
      if (masked) {
        maskingKey= data.mid (offset, 4);
        offset+= 4;
      }

      QByteArray payload= data.mid (offset);
      if (masked) {
        for (int i= 0; i < payload.size (); i++) {
          payload[i]= payload[i] ^ maskingKey[i % 4];
        }
      }

      QByteArray out;
      out.append (char (0x80 | opcode)); // FIN + opcode
      if (payload.size () < 126) {
        out.append (char (payload.size ()));
      }
      else if (payload.size () < 65536) {
        out.append (char (126));
        out.append (char ((payload.size () >> 8) & 0xFF));
        out.append (char (payload.size () & 0xFF));
      }
      out.append (payload);
      clientSocket->write (out);
      clientSocket->flush ();
    }
  }
};

#endif
