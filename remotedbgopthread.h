#ifndef REMOTEDBGOPTHREAD_H
#define REMOTEDBGOPTHREAD_H

#include <QThread>
#include <QUdpSocket>
#include <QHostAddress>

class RemoteDbgOpThread : public QThread
{
    Q_OBJECT
public:
    explicit RemoteDbgOpThread(quint16 local_port, QObject *parent = nullptr);
    ~RemoteDbgOpThread() override;

signals:
    void rmt_scan_sig(bool start, const QString &peer_ip, quint16 peer_port, const QString &cmd_str);
    void rmt_dbg_op_th_started_sig(QString addr_port);                 // 成功绑定时发出监听地址信息
    void rmg_dbg_op_th_error_sig(QString msg);                         // 错误信号
    void rmt_dbg_op_th_stopped_sig();

protected:
    void run() override;

private slots:
    void onReadyRead();

private:
    quint16 m_localPort;
    QUdpSocket *m_udpSocket; // 有 parent，会自动销毁
};

#endif // REMOTEDBGOPTHREAD_H
