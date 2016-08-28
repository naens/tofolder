#include "tofolderdialog.h"
#include "ui_tofolderdialog.h"

#include <QFileInfo>
#include <QFileIconProvider>
#include <QString>
#include <QDir>
#include <QRegularExpression>
#include <QMessageBox>
#include <QInputDialog>

#include <wchar.h>

#include "gst/gst.h"

QStringList selFiles;
QStringList dirFiles;
ToFolderDialog::ToFolderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ToFolderDialog)
{
    ui->setupUi(this);
    QStringList arguments = QCoreApplication::arguments();
    arguments.removeAt(0);
    foreach (QString filePath, arguments) {
        QFileInfo info(filePath);
        QString fn = info.fileName();
        selFiles.append(fn);
    }

    /* load directory file names */
    dirFiles = QDir::current().entryList(QDir::Files, QDir::LocaleAware);

    QStringList folders = QDir::current().entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::LocaleAware);
    if (folders.size() == 0) {
        ui->toolButton->setVisible(false);
    }
}

/*
 * 1) get files matching filter
 * 2) remove from selFiles files not matching filter
 * 3) clear list widget
 * 4) fill list widget */
bool sel_active;
bool folder_from_list;
void ToFolderDialog::updateList()
{
    sel_active = false;
    QString filter = ui->fileLineEdit->text();

    QStringList newSel = selFiles.filter(filter, Qt::CaseInsensitive);

    ui->listWidget->clear();                //clear list widget

    foreach (QString fp, dirFiles) {     //insert back in list widget
        QFileInfo info(fp);
        QString fn = info.fileName();
        if (fn.toLower().contains(filter.toLower())) {
            QFileIconProvider ip;
            QListWidgetItem *pItem = new QListWidgetItem;
            pItem->setIcon(ip.icon(info));
            pItem->setText(fn);
            ui->listWidget->addItem(pItem);
            if (newSel.contains(fn)) {
                pItem->setSelected(true);
                ui->listWidget->setCurrentItem(pItem);
            }
        }
    }
    selFiles = newSel;
    sel_active = true;
    folder_from_list = false;
}

/*
 * init: gst on selected files
 *    -> gst string to file line edit
 *    -> gst string to folder line edit
 *    -> files in folder containing gst in list
 *    -> arg files in list selected (add buttons select all / none)
 * on edit file line edit text:
 *    -> display files in list containing file line edit string
 *    -> keep selected previously selected files
 * on ok
 *    -> if folder exists, ask merge
 *    -> if merge ok and selected files exist in folder ask confirm replace
 *    -> on not confirm, return to dialog
 * on cancel
 *    -> exit
 * */
void ToFolderDialog::showEvent(QShowEvent *)
{
    struct gst *gst;
    gst = new_gst();

    foreach (QString fn, selFiles) {

        int len = fn.length();
        wchar_t *str = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
        fn.toLower().toWCharArray(str);
        str[len] = 0;

        add_string(gst, str);
        free(str);
    }

    int str_count;
    wchar_t **strings;
    longest_strings(gst, &str_count, &strings);

    if (str_count)
    {
        QString longString = QString::fromWCharArray(strings[0], -1);
        longString.remove(QRegExp("[.\\- ]*$"));
        longString.remove(QRegExp("^[.\\- ]*"));

        /* find original case string */
        QString origCaseString;
        foreach (QString fp, dirFiles) {     //insert back in list widget
            QFileInfo info(fp);
            QString fn = info.fileName();
            int index = fn.indexOf(longString, 0, Qt::CaseInsensitive);
            if (index >= 0) {
                origCaseString = fn.mid(index, longString.length());
                break;
            }
        }

        ui->fileLineEdit->setText(origCaseString);
        ui->folderLineEdit->setText(origCaseString);
        updateList();
    }

    for (int j = 0; j < str_count; j++) {
        free(strings[j]);
    }

    if (str_count) {
        free(strings);
    }

    del_gst(gst);
    sel_active = true;
}

ToFolderDialog::~ToFolderDialog()
{
    delete ui;
}

void ToFolderDialog::on_buttonBox_rejected()
{
    QApplication::quit();
}

void ToFolderDialog::on_fileLineEdit_textEdited(const QString)
{
    updateList();
}

void ToFolderDialog::on_buttonBox_accepted()
{
    if (selFiles.size() < 1) {
        return;
    }
    QString dirName = ui->folderLineEdit->text();
    QRegularExpression regex("^[\\w\\-. ]+$");
    if (!regex.match(dirName).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid folder name"), tr("The folder name is not valid."));
        return;
    }
    QString baseDir = QDir::currentPath();

    QFileInfo dir(baseDir, dirName);
    if (dir.exists()) {
        if (dir.isFile()) {
            QMessageBox::warning(this, tr("Invalid folder name"), tr("File exists."));
            return;
        } else if (dir.isDir()) {
            /* directory already exists */
            QStringList commonFiles;
            foreach (QString fn, selFiles) {
                QFileInfo fdestInfo(dir.absoluteFilePath(), fn);
                if (fdestInfo.exists()) {
                    commonFiles.append(fn);
                }
            }

            if (commonFiles.size() > 0) { /* confirm merge & replace */
                QMessageBox msgBox(this);
                msgBox.setInformativeText("Confirm replace the following files in destination folder?\n\n" + commonFiles.join("\n"));
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                msgBox.setDefaultButton(QMessageBox::Ok);
                msgBox.setMinimumWidth(1200);
                int ret = msgBox.exec();
                if (ret != QMessageBox::Ok) {
                    return;
                }
            } else {
                if (!folder_from_list) {    /* confirm move to existing folder */
                    QMessageBox msgBox(this);
                    msgBox.setInformativeText("Confirm move files to existing folder " + dirName);
                    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Ok);
                    msgBox.setMinimumWidth(1200);
                    int ret = msgBox.exec();
                    if (ret != QMessageBox::Ok) {
                        return;
                    }
                }
            }
        }
    } else if (!QDir().mkdir(dir.absoluteFilePath())) {
        QMessageBox::warning(this, tr("Folder not created"), tr("The folder could not be created."));
        return;
    }

    /* move files! */
    foreach (QString fn, selFiles) {
        QFileInfo fpathInfo(baseDir, fn);
        QFileInfo fdestInfo(dir.absoluteFilePath(), fn);
        QString fpath = fpathInfo.absoluteFilePath();
        QString fdest = fdestInfo.absoluteFilePath();
        QFile f(fpath);
        f.rename(fdest);
        QApplication::quit();
    }
}

void ToFolderDialog::on_listWidget_itemSelectionChanged()
{
   if (sel_active)
    {
        selFiles.clear();
        foreach (QListWidgetItem *pItem, ui->listWidget->selectedItems()) {
            selFiles.append(pItem->text());
        }
    }
}

void ToFolderDialog::folderDialogSelection()
{
    ui->folderLineEdit->setText(folderDialog->getSelectedFolder());
}

void ToFolderDialog::on_toolButton_clicked()
{
    if (!folderDialog)
    {
        folderDialog = new FolderDialog(this);
        QStringList folders = QDir::current().entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::LocaleAware);
        folderDialog->setFolders(folders);
        connect(folderDialog, SIGNAL(accepted()), this, SLOT(folderDialogSelection()));
        folder_from_list = true;
    }
    folderDialog->show();
}

void ToFolderDialog::on_folderLineEdit_textEdited(const QString)
{
    folder_from_list = false;
}
