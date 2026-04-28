// =============================================================================
// AuditDialog — table viewer for the append-only audit log served by Core's
// GET /api/v1/admin/audit endpoint.  Optionally filtered by package id.
// =============================================================================
#ifndef NEXOR_COMMAND_AUDITDIALOG_H
#define NEXOR_COMMAND_AUDITDIALOG_H

#include <QDialog>
#include "CoreClient.h"

class QTableWidget;
class QLabel;

namespace nx {

class AuditDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuditDialog(QWidget *parent = nullptr);

    void setEvents(const QString &filterPackage,
                   const QVector<AuditRow> &events);

private:
    QLabel       *m_header;
    QTableWidget *m_table;
};

} // namespace nx

#endif // NEXOR_COMMAND_AUDITDIALOG_H
