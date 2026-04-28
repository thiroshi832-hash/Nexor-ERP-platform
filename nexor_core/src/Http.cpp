#include "Http.h"

#include <QTcpSocket>
#include <QUrl>
#include <QDateTime>
#include <QDebug>

namespace nx {

// ─── HttpRequest ─────────────────────────────────────────────────────────
QString HttpRequest::header(const QString &name, const QString &fallback) const {
    QString lo = name.toLower();
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        if (it.key().toLower() == lo) return it.value();
    return fallback;
}

// ─── HttpResponse ────────────────────────────────────────────────────────
void HttpResponse::setStatus(int code, const QString &text) {
    status = code; statusText = text;
}
void HttpResponse::setBody(const QByteArray &b, const QString &contentType) {
    body = b;
    headers.insert("Content-Type",   contentType);
    headers.insert("Content-Length", QString::number(b.size()));
}
void HttpResponse::setText(const QString &t) {
    setBody(t.toUtf8(), "text/plain; charset=utf-8");
}
void HttpResponse::setJson(const QByteArray &json) {
    setBody(json, "application/json");
}

QByteArray HttpResponse::toWire() const {
    QByteArray out;
    out += "HTTP/1.1 " + QByteArray::number(status) + " "
        + statusText.toUtf8() + "\r\n";
    QMap<QString, QString> h = headers;
    if (!h.contains("Server"))     h.insert("Server", "NexorCore/0.1");
    if (!h.contains("Date"))       h.insert("Date",
        QLocale::c().toString(QDateTime::currentDateTimeUtc(),
                              "ddd, dd MMM yyyy hh:mm:ss") + " GMT");
    if (!h.contains("Connection")) h.insert("Connection", "close");
    if (!h.contains("Content-Length"))
        h.insert("Content-Length", QString::number(body.size()));
    for (auto it = h.constBegin(); it != h.constEnd(); ++it) {
        out += it.key().toUtf8()  + ": " + it.value().toUtf8() + "\r\n";
    }
    out += "\r\n";
    out += body;
    return out;
}

// ─── Router ──────────────────────────────────────────────────────────────
void Router::route(const QString &method, const QString &pattern, HttpHandler h) {
    Entry e;
    e.method   = method.toUpper();
    e.segments = pattern.split('/', Qt::SkipEmptyParts);
    e.handler  = std::move(h);
    m_entries.append(e);
}

bool Router::match(const QStringList &pattern,
                   const QStringList &actual,
                   QMap<QString, QString> &captures) {
    if (pattern.size() != actual.size()) return false;
    for (int i = 0; i < pattern.size(); ++i) {
        if (pattern[i].startsWith(':')) {
            captures.insert(pattern[i].mid(1),
                QUrl::fromPercentEncoding(actual[i].toUtf8()));
        } else if (pattern[i] != actual[i]) {
            return false;
        }
    }
    return true;
}

bool Router::hasPath(const QString &path) const {
    QStringList actual = path.split('/', Qt::SkipEmptyParts);
    for (const Entry &e : m_entries) {
        QMap<QString, QString> caps;
        if (match(e.segments, actual, caps)) return true;
    }
    return false;
}

bool Router::dispatch(HttpRequest &req, HttpResponse &res) const {
    QStringList actual = req.path.split('/', Qt::SkipEmptyParts);
    for (const Entry &e : m_entries) {
        QMap<QString, QString> caps;
        if (!match(e.segments, actual, caps)) continue;
        if (e.method != req.method) continue;
        req.pathParams = caps;
        e.handler(req, res);
        return true;
    }
    return false;
}

// ─── HttpServer ──────────────────────────────────────────────────────────
HttpServer::HttpServer(Router *router, QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this)), m_router(router) {
    connect(m_server, &QTcpServer::newConnection,
            this,     &HttpServer::onNewConnection);
}

bool HttpServer::start(quint16 port) {
    return m_server->listen(QHostAddress::Any, port);
}
void HttpServer::stop() { m_server->close(); }

void HttpServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *sock = m_server->nextPendingConnection();
        new HttpConnection(sock, m_router, this);   // self-managing
    }
}

// ─── HttpConnection ──────────────────────────────────────────────────────
HttpConnection::HttpConnection(QTcpSocket *sock, Router *router, QObject *parent)
    : QObject(parent), m_sock(sock), m_router(router) {
    m_sock->setParent(this);
    connect(m_sock, &QTcpSocket::readyRead,    this, &HttpConnection::onReadyRead);
    connect(m_sock, &QTcpSocket::disconnected, this, &HttpConnection::onDisconnected);
}

void HttpConnection::onReadyRead() {
    m_buffer.append(m_sock->readAll());
    if (!m_headersDone) {
        int hdrEnd = m_buffer.indexOf("\r\n\r\n");
        if (hdrEnd < 0) return;       // wait for more
        // Parse request line + headers.
        QByteArray hdrs = m_buffer.left(hdrEnd);
        m_buffer.remove(0, hdrEnd + 4);

        QList<QByteArray> lines = hdrs.split('\n');
        if (lines.isEmpty()) { m_sock->disconnectFromHost(); return; }

        QByteArray reqLine = lines.first().trimmed();
        QList<QByteArray> parts = reqLine.split(' ');
        if (parts.size() < 2) { m_sock->disconnectFromHost(); return; }
        m_req.method = QString::fromUtf8(parts[0]).toUpper();
        QByteArray target = parts[1];
        int q = target.indexOf('?');
        if (q >= 0) {
            m_req.path  = QString::fromUtf8(target.left(q));
            m_req.query = QString::fromUtf8(target.mid(q + 1));
        } else {
            m_req.path = QString::fromUtf8(target);
        }

        for (int i = 1; i < lines.size(); ++i) {
            QByteArray ln = lines[i].trimmed();
            if (ln.isEmpty()) continue;
            int c = ln.indexOf(':');
            if (c < 0) continue;
            QString key = QString::fromUtf8(ln.left(c)).trimmed();
            QString val = QString::fromUtf8(ln.mid(c + 1)).trimmed();
            m_req.headers.insert(key, val);
        }

        m_contentLength = m_req.header("Content-Length", "0").toInt();
        m_headersDone = true;
    }

    // Body — for GET/HEAD usually 0; for POST whatever Content-Length says.
    if (m_buffer.size() >= m_contentLength) {
        m_req.body = m_buffer.left(m_contentLength);
        respond();
    }
}

void HttpConnection::respond() {
    if (m_responded) return;
    m_responded = true;

    HttpResponse res;
    bool handled = m_router && m_router->dispatch(m_req, res);
    if (!handled) {
        if (m_router && m_router->hasPath(m_req.path)) {
            res.setStatus(405, "Method Not Allowed");
            res.setText("Method Not Allowed: " + m_req.method + " " + m_req.path);
        } else {
            res.setStatus(404, "Not Found");
            res.setText("Not Found: " + m_req.path);
        }
    }

    qInfo().noquote() << QString("[%1] %2 %3 → %4")
        .arg(m_sock->peerAddress().toString())
        .arg(m_req.method, m_req.path)
        .arg(res.status);

    m_sock->write(res.toWire());
    m_sock->disconnectFromHost();
}

void HttpConnection::onDisconnected() { deleteLater(); }

} // namespace nx
