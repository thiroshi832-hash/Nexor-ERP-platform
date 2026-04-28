// =============================================================================
// DiffDialog — side-by-side viewer for the result of GET .../packages/:id/diff.
//
// Shows two panes:
//   • Entries — added / removed / changed / unchanged across Activities,
//               Forms, Sheets, Processes, Resources.
//   • Schema migration — per-Sheet field changes (added / removed /
//                        type-changed / flags-changed) so the admin can
//                        eyeball ALTER-TABLE risk before deploying.
// =============================================================================
#ifndef NEXOR_COMMAND_DIFFDIALOG_H
#define NEXOR_COMMAND_DIFFDIALOG_H

#include <QDialog>
#include "CoreClient.h"

class QTreeWidget;
class QLabel;

namespace nx {

class DiffDialog : public QDialog {
    Q_OBJECT
public:
    explicit DiffDialog(QWidget *parent = nullptr);

    void setDiff(const DiffResult &d);

private:
    QLabel      *m_header;
    QTreeWidget *m_entries;
    QTreeWidget *m_sheets;
};

} // namespace nx

#endif // NEXOR_COMMAND_DIFFDIALOG_H
