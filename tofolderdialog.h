#ifndef TOFOLDERDIALOG_H
#define TOFOLDERDIALOG_H

#include <QDialog>

#include "folderdialog.h"

namespace Ui {
class ToFolderDialog;
}

class ToFolderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToFolderDialog(QWidget *parent = 0);
    ~ToFolderDialog();

protected:
  void showEvent (QShowEvent * event);

private slots:
  void on_buttonBox_rejected();

  void on_fileLineEdit_textEdited(const QString);

  void on_buttonBox_accepted();

  void on_listWidget_itemSelectionChanged();

  void on_toolButton_clicked();

  void on_folderLineEdit_textEdited(const QString);

  void folderDialogSelection();

private:
    Ui::ToFolderDialog *ui;
    FolderDialog * folderDialog;

    QIcon fileIcon(const QString &filename);
    void updateList();
};

#endif // TOFOLDERDIALOG_H
