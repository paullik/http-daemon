#include <QDebug>
#include <QThread>
#include <QThreadPool>

#include "httpserver.h"
#include "httpconnectionmanager.h"

HTTPServer::HTTPServer(const QString &docRoot, const QString &pluginRoot, QObject *parent) :
    QTcpServer(parent), docRoot(docRoot), pluginRoot(pluginRoot)
{
    //TODO: check the network and the files owned by my program to see what
    //QTcpServer does
}

//TODO: convert everything to C++11
void HTTPServer::incomingConnection(int socketDescriptor)
{
    HTTPConnectionManager *httpConnectionManager =
            new HTTPConnectionManager(socketDescriptor, docRoot, pluginRoot);

    connect(httpConnectionManager, SIGNAL(connectionClosed()),
            httpConnectionManager, SLOT(deleteLater()));

    httpConnectionManager->setAutoDelete(false);

    QThreadPool::globalInstance()->start(httpConnectionManager);
}
