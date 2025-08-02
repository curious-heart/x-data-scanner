#include "serialsniffer.h"

SerialSniffer::SerialSniffer(QSerialPort *port, QObject *parent)
    : QThread(parent), m_port(port)
{
    m_logFile.setFileName("serial_debug.log");
    if (!m_logFile.open(QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Cannot open log file in sniffer!";
        return;
    }

    // 连接信号
    connect(m_port, &QSerialPort::readyRead, this, &SerialSniffer::onReadyRead);
    connect(m_port, &QSerialPort::bytesWritten, this, &SerialSniffer::onBytesWritten);
}

SerialSniffer::~SerialSniffer()
{
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void SerialSniffer::onReadyRead()
{
    int avail = m_port->bytesAvailable();
    if (avail > 0) {
        QByteArray data = m_port->peek(avail); // 不消耗数据
        logLine("RX", data);
    }
}

void SerialSniffer::onBytesWritten(qint64 bytes)
{
    // 这里只能知道发出去多少字节，但无法直接拿到数据内容
    // 如果要拿到 TX 数据，需要在调用 write() 的地方做记录
    logLine(QString("TX %1 bytes").arg(bytes), QByteArray());
}

void SerialSniffer::logLine(const QString &direction, const QByteArray &data)
{
    QString hexStr = data.isEmpty() ? "" : data.toHex(' ');
    QString line = QString("[%1] %2: %3\n")
                   .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                   .arg(direction)
                   .arg(hexStr);

    m_logFile.write(line.toUtf8());
    m_logFile.flush();
    qDebug().noquote() << line.trimmed();
}
