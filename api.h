#ifndef API_H
#define API_H

#include <QHttpServer>
#include <QObject>
#include <QTcpServer>

class API : public QObject
{
    Q_OBJECT
public:
    explicit API(QObject *parent = nullptr);

private:
    QHttpServer *m_httpServer;
    QTcpServer *m_tcpServer;
};

#endif // API_H
