
#include "pqChartContextMenu.h"

#include "pqFileDialog.h"
#include "pqLocalFileDialogModel.h"

#include <QAction>
#include <QMenu>
#include <QPrinter>
#include <QPrintDialog>
#include <QWidget>


pqChartContextMenu::pqChartContextMenu(QObject *parentObject)
  : QObject(parentObject)
{
}

void pqChartContextMenu::addMenuActions(QMenu &menu, QWidget *chart) const
{
  // Create the actions with the chart stored as data.
  QAction *action = menu.addAction("Print Chart", this, SLOT(printChart()));
  action->setData(qVariantFromValue<QWidget *>(chart));
  action = menu.addAction("Save .pdf", this, SLOT(savePDF()));
  action->setData(qVariantFromValue<QWidget *>(chart));
  action = menu.addAction("Save .png", this, SLOT(savePNG()));
  action->setData(qVariantFromValue<QWidget *>(chart));
}

void pqChartContextMenu::printChart()
{
  // Get the action from the sender.
  QAction *action = qobject_cast<QAction *>(this->sender());
  if(action)
    {
    // Get the chart from the action's data.
    QWidget *chart = qvariant_cast<QWidget *>(action->data());
    if(chart)
      {
      QPrinter printer(QPrinter::HighResolution);

      QPrintDialog print_dialog(&printer);
      if(print_dialog.exec() == QDialog::Accepted)
        {
        QMetaObject::invokeMethod(chart, "printChart",
            Qt::DirectConnection, QArgument<QPrinter>("QPrinter&", printer));
        }
      }
    }
}

void pqChartContextMenu::savePDF()
{
  // Get the action from the sender.
  QAction *action = qobject_cast<QAction *>(this->sender());
  if(action)
    {
    // Get the chart from the action's data.
    QWidget *chart = qvariant_cast<QWidget *>(action->data());
    if(chart)
      {
      pqFileDialog *file_dialog = new pqFileDialog(new pqLocalFileDialogModel(),
          chart, tr("Save .pdf File:"), QString(), "PDF files (*.pdf)");    
      file_dialog->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
      file_dialog->setObjectName("fileSavePDFDialog");
      file_dialog->setFileMode(pqFileDialog::AnyFile);
      QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)),
          chart, SLOT(savePDF(const QStringList&)));
        
      file_dialog->show();
      }
    }
}

void pqChartContextMenu::savePNG()
{
  // Get the action from the sender.
  QAction *action = qobject_cast<QAction *>(this->sender());
  if(action)
    {
    // Get the chart from the action's data.
    QWidget *chart = qvariant_cast<QWidget *>(action->data());
    if(chart)
      {
      pqFileDialog *file_dialog = new pqFileDialog(new pqLocalFileDialogModel(),
          chart, tr("Save .png File:"), QString(), "PNG files (*.png)");
      file_dialog->setAttribute(Qt::WA_DeleteOnClose);
      file_dialog->setObjectName("fileSavePNGDialog");
      file_dialog->setFileMode(pqFileDialog::AnyFile);
      QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)),
          chart, SLOT(savePNG(const QStringList&)));
        
      file_dialog->show();
      }
    }
}


