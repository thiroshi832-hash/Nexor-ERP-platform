// =============================================================================
// Http — minimal HTTP/1.1 server built on QTcpServer.
//
// This is intentionally small: enough to serve the four package-registry
// endpoints (/api/v1/health, GET/POST /api/v1/packages, GET package by id +
// version) and to gracefully reject anything else with 404 / 405.
//
// Wire shape:
//
//   HttpServer (listens) ──► HttpConnection (per-socket parser)
//                                   │
//                                   ▼
//                        HttpRequest  ──► Router lookup ──► Handler
//                        HttpResponse ◄────────────────────────┘
//
// We do not keep connections alive between requests; each socket handles
// exactly one request → one response → close.  That keeps the parser
// trivial and is more than fast enough for an admin-facing registry.
// =============================================================================
#ifndef NEXOR_CORE_HTTP_H
#define NEXOR_CORE_HTTP_H

#include <QObject>
#include <QTcpServer>
#include <QHash>
#include <QByteArray>
#include <QString>
#include <QMap>
#include <functional>

class QTcpSocket;

namespace nx {

struct HttpRequest {
    QString  method;                              // "GET", "POST", …
    QString  path;                                // request-target (no host)
    QString  query;                               // raw query string
    QMap<QString, QString> headers;               // case-insensitive on lookup
    QByteArray body;

    // Captured path parameters (e.g. ":id" / ":version" segments).
    QMap<QString, QString> pathParams;

    QString header(const QString &name, const QString &fallback = {}) const;
};

class HttpResponse {
public:
    int                    status { 200 };
    QString                statusText { "OK" };
    QMap<QString, QString> headers;
    QByteArray             body;

    void setStatus(int code, const QString &text);
    void setBody (const QByteArray &b, const QString &contentType);
    void setText (const QString &t);
    void setJson (const QByteArray &json);

    QByteArray toWire() const;       // serialises status + headers + body
};

using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

// Minimal route table.  Patterns may include ":name" capture segments:
//   "/api/v1/packages/:id/:version" matches "/api/v1/packages/sales/0.4.1"
//   and exposes pathParams["id"]="sales", pathParams["version"]="0.4.1".
class Router {
public:
    void route(const QString &method, const QString &pattern, HttpHandler h);
    bool dispatch(HttpRequest &req, HttpResponse &res) const;
    bool hasPath(const QString &path) const;       // for 404 vs 405 routing

private:
    struct Entry {
        QString     method;
        QStringList segments;     // pattern split by '/'
        HttpHandler handler;
    };
    QVector<Entry> m_entries;

    static bool match(const QStringList &pattern,
                      const QStringList &actual,
                      QMap<QString, QString> &captures);
};

class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(Router *router, QObject *parent = nullptr);

    bool start(quint16 port);
    void stop();

private slots:
    void onNewConnection();

private:
    QTcpServer *m_server;
    Router     *m_router;
};

// One HTTP transaction over a single socket.  Self-deletes when finished.
class HttpConnection : public QObject {
    Q_OBJECT
public:
    HttpConnection(QTcpSocket *sock, Router *router, QObject *parent = nullptr);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    bool tryParse();             // returns true if a full request is buffered
    void respond();              // dispatch + write + close

    QTcpSocket *m_sock;
    Router     *m_router;
    QByteArray  m_buffer;
    HttpRequest m_req;
    bool        m_headersDone   { false };
    bool        m_responded     { false };
    int         m_contentLength { 0 };
};

} // namespace nx

#endif // NEXOR_CORE_HTTP_H
