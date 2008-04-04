/*=========================================================================

   Program: ParaView
   Module:    pqChartPrintSave.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqChartPrintSave.h"

#include <QAction>
#include <QMenu>
#include <QPrinter>
#include <QPrintDialog>
#include <QWidget>
#include <QFileDialog>


pqChartPrintSave::pqChartPrintSave(QObject *parentObject)
  : QObject(parentObject)
{
}

void pqChartPrintSave::addMenuActions(QMenu &menu, QWidget *chart) const
{
  // Create the actions with the chart stored as data.
  QAction *action = menu.addAction("Print Chart", this, SLOT(printChart()));
  action->setData(qVariantFromValue<QWidget *>(chart));
  action = menu.addAction("Save .pdf", this, SLOT(savePDF()));
  action->setData(qVariantFromValue<QWidget *>(chart));
  action = menu.addAction("Save .png", this, SLOT(savePNG()));
  action->setData(qVariantFromValue<QWidget *>(chart));
}

void pqChartPrintSave::printChart()
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

void pqChartPrintSave::savePDF()
{
  // Get the action from the sender.
  QAction *action = qobject_cast<QAction *>(this->sender());
  if(action)
    {
    // Get the chart from the action's data.
    QWidget *chart = qvariant_cast<QWidget *>(action->data());
    if(chart)
      {
      QFileDialog* file_dialog = new QFileDialog(chart, tr("Save .pdf File:"),
                                       QString(), "PDF files (*.pdf)");
      file_dialog->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
      file_dialog->setObjectName("fileSavePDFDialog");
      file_dialog->setFileMode(QFileDialog::AnyFile);
      QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)),
          chart, SLOT(saveChart(const QStringList&)));
        
      file_dialog->show();
      }
    }
}

void pqChartPrintSave::savePNG()
{
  // Get the action from the sender.
  QAction *action = qobject_cast<QAction *>(this->sender());
  if(action)
    {
    // Get the chart from the action's data.
    QWidget *chart = qvariant_cast<QWidget *>(action->data());
    if(chart)
      {
      QFileDialog* file_dialog = new QFileDialog(chart, tr("Save .png File:"),
                                       QString(), "PNG files (*.png)");
      file_dialog->setAttribute(Qt::WA_DeleteOnClose);
      file_dialog->setObjectName("fileSavePNGDialog");
      file_dialog->setFileMode(QFileDialog::AnyFile);
      QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)),
          chart, SLOT(saveChart(const QStringList&)));
        
      file_dialog->show();
      }
    }
}


