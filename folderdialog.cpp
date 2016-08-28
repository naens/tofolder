#include "folderdialog.h"
#include "ui_folderdialog.h"

#include <QFileIconProvider>
#include <QKeyEvent>

QStringList folderList;
QString selectedFolder;
QWidget *p;
FolderDialog::FolderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FolderDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    p = parent;
}

FolderDialog::~FolderDialog()
{
    delete ui;
}

void FolderDialog::showEvent(QShowEvent *)
{
    move(
       p->window()->frameGeometry().topLeft() +
       p->window()->rect().center() - rect().center()
    );
    ui->listWidget->clear();
    foreach (QString folder, folderList) {
        QListWidgetItem *pItem = new QListWidgetItem;
        QFileIconProvider ip;
        pItem->setIcon(ip.icon(QFileIconProvider::Folder));
        pItem->setText(folder);
        ui->listWidget->addItem(pItem);
    }
    ui->listWidget->setCurrentItem(NULL);
}

void FolderDialog::setFolders(QStringList folders)
{
    folderList = folders;
}


QString FolderDialog::getSelectedFolder()
{
    return selectedFolder;
}

void FolderDialog::selectExit()
{
    QList <QListWidgetItem *> selectedItems = ui->listWidget->selectedItems();
    QListWidgetItem *pItem = NULL;
    if (selectedItems.size() > 0) {
        pItem = selectedItems.first();
    } else {
        pItem = ui->listWidget->currentItem();
    }
    if (pItem != NULL) {
        selectedFolder = pItem->text();
        emit accepted();
        this->hide();
    }
}

void FolderDialog::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
        if(!this->isActiveWindow()) {
            this->hide();
        }
    }
}

void FolderDialog::keyPressEvent(QKeyEvent* event)
{
    switch ((event->key())) {
    case Qt::Key_Escape:
        this->hide();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        selectExit();
        break;
    default:
        break;
    }
}

void FolderDialog::on_listWidget_clicked(const QModelIndex &)
{
    selectExit();
}
