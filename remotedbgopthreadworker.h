#ifndef REMOTEDBGOPTHREADWORKER_H
#define REMOTEDBGOPTHREADWORKER_H

#include <QObject>
#include <QUdpSocket>

class RemoteDbgOpThreadWorker : public QObject
{
    Q_OBJECT
public:
    explicit RemoteDbgOpThreadWorker(quint16 local_port, QObject *parent = nullptr);

    ~RemoteDbgOpThreadWorker() override;

    bool init_ok();

signals:
    void rmt_scan_sig(bool start, const QString &peer_ip, quint16 peer_port, const QString &cmd_str);
    void rmg_dbg_op_th_error_sig(QString msg);                         // 错误信号
    void rmt_dbg_op_th_stopped_sig();

private slots:
    void onReadyRead();

public slots:
    void thread_exit_clean();

private:
    bool m_init_ok = false;
    quint16 m_localPort;
    QUdpSocket *m_udpSocket;
};

#endif // REMOTEDBGOPTHREADWORKER_H
