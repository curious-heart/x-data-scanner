#include <QNetworkDatagram>
#include "remotedbgopthreadworker.h"
#include "logger/logger.h"

RemoteDbgOpThreadWorker::RemoteDbgOpThreadWorker(quint16 local_port, QObject *parent)
    : QObject{parent},m_init_ok(false),
      m_localPort(local_port),
      m_udpSocket(nullptr)
{
    m_udpSocket = new QUdpSocket(this);

    if(!m_udpSocket->bind(QHostAddress::Any, m_localPort))
    {
        QString err = QString("Rmt op thread: failed to bind UDP port %1: %2")
                          .arg(m_localPort)
                          .arg(m_udpSocket->errorString());

        DIY_LOG(LOG_ERROR, err);
        return;
    }

    /* Qt::QueuedConnection. can't be used here, or only the 1st packe can be received.
     * I don't know why...*/
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &RemoteDbgOpThreadWorker::onReadyRead);

    m_init_ok = true;
}

RemoteDbgOpThreadWorker::~RemoteDbgOpThreadWorker()
{
    if(m_init_ok && m_udpSocket)
    {
        m_udpSocket->close();
    }
    DIY_LOG(LOG_INFO, "remote debug obj destructed.");
}

bool RemoteDbgOpThreadWorker::init_ok()
{
    return m_init_ok;
}

void RemoteDbgOpThreadWorker::onReadyRead()
{
    while(m_udpSocket->hasPendingDatagrams())
    {
        QHostAddress sender;
        quint16 senderPort;
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        sender = datagram.senderAddress();
        senderPort = datagram.senderPort();

        QString cmd = QString::fromUtf8(datagram.data()).trimmed();

        DIY_LOG(LOG_INFO, QString("remote debug thread received data \"%1\" from peer %2:%3")
                               .arg(cmd, sender.toString()).arg(senderPort));

        if (cmd == QLatin1String("scan start"))
        {
            emit rmt_scan_sig(true, sender.toString(), senderPort, cmd);
        }
        else if (cmd == QLatin1String("scan stop"))
        {
            emit rmt_scan_sig(false, sender.toString(), senderPort, cmd);
        }
        else
        {
            DIY_LOG(LOG_WARN, QString("Unknown command from %1:%2 -> %3")
                          .arg(sender.toString()).arg(senderPort).arg(cmd));
        }
    }
}

void RemoteDbgOpThreadWorker::thread_exit_clean()
{
    DIY_LOG(LOG_INFO, "remote debug thread exit clean");
    deleteLater();
}
