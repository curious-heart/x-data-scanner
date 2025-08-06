#include "remotedbgopthread.h"
#include "logger/logger.h"
#include <QDebug>

RemoteDbgOpThread::RemoteDbgOpThread(quint16 local_port, QObject *parent)
    : QThread(parent),
      m_localPort(local_port),
      m_udpSocket(nullptr)
{
}

RemoteDbgOpThread::~RemoteDbgOpThread()
{
    // 退出逻辑由主线程管理
}

void RemoteDbgOpThread::run()
{
    m_udpSocket = new QUdpSocket(this); // parent 为线程对象

    if (!m_udpSocket->bind(QHostAddress::Any, m_localPort))
    {
        QString err = QString("Rmt op thread: failed to bind UDP port %1: %2")
                          .arg(m_localPort)
                          .arg(m_udpSocket->errorString());

        DIY_LOG(LOG_ERROR, err);
        emit rmg_dbg_op_th_error_sig(err);

        emit rmt_dbg_op_th_stopped_sig();
        return;
    }

    QString addr_info = QString("%1:%2")
                            .arg(m_udpSocket->localAddress().toString())
                            .arg(m_udpSocket->localPort());
    emit rmt_dbg_op_th_started_sig(addr_info);

    connect(m_udpSocket, &QUdpSocket::readyRead,
            this, &RemoteDbgOpThread::onReadyRead, Qt::QueuedConnection);

    exec();

    m_udpSocket->close();
    emit rmt_dbg_op_th_stopped_sig();
}

void RemoteDbgOpThread::onReadyRead()
{
    while(m_udpSocket->hasPendingDatagrams())
    {
        QString log_str;
        QByteArray datagram;
        datagram.resize(int(m_udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(),
                                  &sender, &senderPort);

        QString cmd = QString::fromUtf8(datagram).trimmed();
        if (cmd == QLatin1String("scan start")) {
            emit rmt_scan_sig(true, sender.toString(), senderPort, cmd);
        } else if (cmd == QLatin1String("scan stop")) {
            emit rmt_scan_sig(false, sender.toString(), senderPort, cmd);
        } else {
            log_str = QString("Unknown command from %1:%2 -> %3")
                          .arg(sender.toString()).arg(senderPort).arg(cmd);
        }
    }
}
