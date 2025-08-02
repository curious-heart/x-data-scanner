#ifndef SERIALSNIFFER_H
#define SERIALSNIFFER_H

#include <QThread>
#include <QSerialPort>
#include <QFile>
#include <QDateTime>
#include <QDebug>

class SerialSniffer : public QThread
{
    Q_OBJECT
public:
    explicit SerialSniffer(QSerialPort *port, QObject *parent = nullptr);
    ~SerialSniffer();

private slots:
    void onReadyRead();
    void onBytesWritten(qint64 bytes);

private:
    void logLine(const QString &direction, const QByteArray &data);

    QSerialPort *m_port;
    QFile m_logFile;
};

#endif // SERIALSNIFFER_H
