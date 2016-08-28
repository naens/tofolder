#ifndef FOLDERDIALOG_H
#define FOLDERDIALOG_H

#include <QDialog>

namespace Ui {
class FolderDialog;
}

class FolderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FolderDialog(QWidget *parent = 0);
    ~FolderDialog();

    void setFolders(QStringList folders);

    QString getSelectedFolder();

protected:
  void showEvent (QShowEvent * event);
  void keyPressEvent(QKeyEvent* event);
  void changeEvent(QEvent *event);

private slots:
  void on_listWidget_clicked(const QModelIndex &);

private:
    Ui::FolderDialog *ui;

    void selectExit();
};

#endif // FOLDERDIALOG_H
