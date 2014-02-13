#include <QDebug>
#include <QHostAddress>

#include "httpparser.h"

HTTPParser::HTTPParser(QObject *parent) : QObject(parent), isParsedHeader(false)
{
    requestData.contentLength = -1;
}

void HTTPParser::parsePostData()
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "DATA:" << data;
    if(data.isEmpty()){
        return;
    }

    //TODO: take into account more content types
    if(requestData.contentType.contains("multipart/form-data")){
        requestData.rawPostData = data;
        bytesToParse = 0;
        return;
    }

    QString postBody = data;
    QStringList pairs = postBody.split("&", QString::SkipEmptyParts);

    foreach(QString pair, pairs){
        QStringList keyVal = pair.split("=");

        if(2 != keyVal.size()){
            emit parseError("Invalid POST data!");
            return;
        }

        requestData.postData.insert(QUrl::fromPercentEncoding(keyVal[0].toUtf8()),
                QUrl::fromPercentEncoding(keyVal[1].toUtf8()));
    }

    bytesToParse = 0;
}

void HTTPParser::parseRequestHeader(const QByteArray &h)
{

    qDebug() << Q_FUNC_INFO;
    qDebug() << "HEADER:" << h;
    QString header(h);

    QStringList fields = header.replace("\r", "").split("\n", QString::SkipEmptyParts);

    if(fields.isEmpty()){
        emit parseError("Empty request!");
        return;
    }

    QStringList statusLine = fields[0].split(" ");

    if(3 != statusLine.size()){
        emit parseError("Invalid status line!");
        return;
    }

    if("GET" != statusLine[0] && "POST" != statusLine[0] &&
            "HEAD" != statusLine[0]){
        emit parseError("Invalid method!");
        return;
    }

    if(statusLine[1].isEmpty()){
        emit parseError("Path cannot be empty!");
        return;
    }

    QStringList protocol = statusLine[2].split("/");
    bool ok;

    if(2 != protocol.size()){
        emit parseError("Invalid protocol!");
        return;
    }

    double ver = protocol[1].toDouble(&ok);

    if("HTTP" != protocol[0] || !ok || ver < 0.9 || ver > 1.1){
        emit parseError("Invalid protocol!");
        return;
    }

    requestData.url.setUrl(statusLine[1]);

    if(!requestData.url.isValid()){
        emit parseError("Invalid URL!");
        return;
    }

    requestData.method = statusLine[0].toUpper();
    requestData.protocol = protocol[0];
    requestData.protocolVersion = ver;

    //TODO: refactor the above into a different method: parseStatusLine

    fields.removeAt(0);
    qDebug() << "FIELDS:" << fields;
    int colonPos;
    foreach(QString line, fields){
        colonPos = line.indexOf(":");

        if(-1 != colonPos){
            //TODO: move the custom checks from below here so that I don't add and then remove an entry from the fields
            requestData.fields.insert(line.left(colonPos).trimmed(), line.right(line.size()-colonPos-1).trimmed());
        }
    }

    if(requestData.fields.contains("Host")){
        if(requestData.fields["Host"].toString().size()){
            QStringList hostLine = requestData.fields["Host"].toString().split(":");

            if(hostLine.size() == 2){
                requestData.port = hostLine[1].toUInt(&ok);
                if(!ok){
                    emit parseError("Invalid port number!");
                    return;
                }
            }

            requestData.host = QHostAddress(hostLine[0]);
            requestData.fields.remove("Host");
        }
    }

    if(requestData.fields.contains("Content-Length")){
        requestData.contentLength = requestData.fields["Content-Length"].toUInt(&ok);

        if(!ok){
            emit parseError("Invalid Content-Length value!");
            return;
        }

        requestData.fields.remove("Content-Length");
    }

    if(requestData.fields.contains("Content-Type")){
        requestData.contentType = requestData.fields["Content-Type"].toString();
        requestData.fields.remove("Content-Type");
    }

    if(requestData.fields.contains("Cookie")){
        //replace spaces and semicolons with newlines so that the parsing is done properly
        //ugly hack, but it's needed since parseCookies() is designed to work on server-set "Set-Cookie:" headers,
        //not on "Cookie:" headers, set by clients
        requestData.cookieJar = QNetworkCookie::parseCookies(
            requestData.fields["Cookie"].toByteArray().replace(" ", "\n").replace(";", "\n"));

        if(requestData.cookieJar.empty()){
            emit parseError("Invalid Cookie value!");
            return;
        }

        requestData.fields.remove("Cookie");
    }

    bytesToParse = requestData.contentLength;

    isParsedHeader = true;
}

HTTPParser &HTTPParser::operator<<(const QString &chunk)
{
    return *this << chunk.toUtf8();
}

HTTPParser &HTTPParser::operator<<(const QByteArray &chunk)
{
    data.append(chunk);
    parse();

    return *this;
}

void HTTPParser::parse()
{
    //TODO: handle the case when there is no header and body distinction
    if(!isParsedHeader){
        int pos = data.indexOf("\r\n\r\n");
        int len = 4;

        if(-1 == pos){
            pos = data.indexOf("\n\n");
            len = 2;
        }

        if(-1 != pos){
            parseRequestHeader(data.left(pos));
            data = data.right(data.size()-pos-len);
        }
    }

    if(isParsedHeader && "POST" == requestData.method){
        if(bytesToParse < 0){
            //a POST request with no Content-Length is bogus as per standard
            emit parseError("POST request with erroneous Content-Length field!");
            return;
        }

        if(data.size() == bytesToParse){
            parsePostData();
        }
    }

    if(isParsedHeader && (0 == bytesToParse || "GET" == requestData.method ||
                          "HEAD" == requestData.method)){
        emit parsed(requestData);
    }
}
