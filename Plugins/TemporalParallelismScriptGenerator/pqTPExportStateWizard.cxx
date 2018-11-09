/*=========================================================================

   Program: ParaView
   Module:    pqTPExportStateWizard.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqTPExportStateWizard.h"
#include "pqSGExportStateWizardInternals.h"

#include <vtkPVXMLElement.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>

#include <pqApplicationCore.h>
#include <pqFileDialog.h>
#include <pqImageOutputInfo.h>
#include <pqPipelineSource.h>
#include <pqServerManagerModel.h>
#include <pqView.h>

#include <QMessageBox>

extern const char* tp_export_py;

//-----------------------------------------------------------------------------
pqTPExportStateWizard::pqTPExportStateWizard(QWidget* parentObject, Qt::WindowFlags parentFlags)
  : Superclass(parentObject, parentFlags)
{
}

//-----------------------------------------------------------------------------
pqTPExportStateWizard::~pqTPExportStateWizard()
{
}

//-----------------------------------------------------------------------------
void pqTPExportStateWizard::customize()
{
  // for spatio-temporal scripts we don't care about frequency or fitting
  // the image to screen
  QList<pqImageOutputInfo*> infos = this->getImageOutputInfos();
  for (QList<pqImageOutputInfo*>::iterator it = infos.begin(); it != infos.end(); it++)
  {
    (*it)->hideFrequencyInput();
    (*it)->hideFitToScreen();
  }

  this->Internals->wizardPage1->setTitle("Export Spatio-Temporal Parallel Script");
  this->Internals->label->setText("This wizard will guide you through the steps required to export "
                                  "the current visualization state as a Python script that can be "
                                  "run with spatio-temporal parallelism with ParaView.  Make sure "
                                  "to add appropriate writers for the desired pipelines to be used "
                                  "in the Writers menu.");
  QStringList labels;
  labels << "Pipeline Name"
         << "File Location";
  this->Internals->nameWidget->setHorizontalHeaderLabels(labels);
  this->Internals->liveViz->hide();
  this->Internals->rescaleDataRange->hide();
  this->Internals->outputCinema->hide();
  this->Internals->wCinemaTrackSelection->hide();
}

//-----------------------------------------------------------------------------
bool pqTPExportStateWizard::getCommandString(QString& command)
{
  command.clear();

  QString export_rendering = "True";
  QString rendering_info; // a map from the render view name to render output params
  if (this->Internals->outputRendering->isChecked() == 0)
  {
    export_rendering = "False";
    // check to make sure that there is a writer hooked up since we aren't
    // exporting an image
    vtkSMSessionProxyManager* proxyManager =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
    bool haveSomeWriters = false;
    QStringList filtersWithoutConsumers;
    for (unsigned int i = 0; i < proxyManager->GetNumberOfProxies("sources"); i++)
    {
      if (vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(
            proxyManager->GetProxy("sources", proxyManager->GetProxyName("sources", i))))
      {
        vtkPVXMLElement* proxyHint = proxy->GetHints();
        if (proxyHint && proxyHint->FindNestedElementByName("WriterProxy"))
        {
          haveSomeWriters = true;
        }
        else
        {
          pqPipelineSource* input = smModel->findItem<pqPipelineSource*>(proxy);
          if (input && input->getNumberOfConsumers() == 0)
          {
            filtersWithoutConsumers << proxyManager->GetProxyName("sources", i);
          }
        }
      }
    }
    if (!haveSomeWriters)
    {
      QMessageBox messageBox;
      QString message(tr("No output writers specified. Either add writers in the pipeline or check "
                         "<b>Output rendering components</b>."));
      messageBox.setText(message);
      messageBox.exec();
      return false;
    }
    if (filtersWithoutConsumers.size() != 0)
    {
      QMessageBox messageBox;
      QString message(tr("The following filters have no consumers and will not be saved:\n"));
      for (QStringList::const_iterator iter = filtersWithoutConsumers.constBegin();
           iter != filtersWithoutConsumers.constEnd(); iter++)
      {
        message.append("  ");
        message.append(iter->toLocal8Bit().constData());
        message.append("\n");
      }
      messageBox.setText(message);
      messageBox.exec();
    }
  }
  else // we are creating an image so we need to get the proper information from there
  {
    QString itemFormat = " '%1' : ['%2', '%5', '%6', '%7']";
    rendering_info = this->Internals->wViewSelection->getSelectionAsString(itemFormat);
  }

  QString filters = "ParaView Python State Files (*.py);;All files (*)";

  pqFileDialog file_dialog(NULL, this, tr("Save Server State:"), QString(), filters);
  file_dialog.setObjectName("ExportSpatio-TemporalStateFileDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (!file_dialog.exec())
  {
    return false;
  }

  QString filename = file_dialog.getSelectedFiles()[0];
#ifdef _WIN32
  // Convert to forward slashes. The issue is that the path is interpreted as a
  // Python string when passed to the interpreter, so a path such as "C:\tests"
  // is read as "C:<TAB>ests" which isn't what we want. Since Windows is
  // flexible anyways, just use Unix separators.
  filename.replace('\\', '/');
#endif

  // mapping from readers and their filenames on the current machine
  // to the filenames on the remote machine
  QString reader_inputs_map;
  for (int cc = 0; cc < this->Internals->nameWidget->rowCount(); cc++)
  {
    QTableWidgetItem* item0 = this->Internals->nameWidget->item(cc, 0);
    QTableWidgetItem* item1 = this->Internals->nameWidget->item(cc, 1);
    reader_inputs_map += QString(" '%1' : '%2',").arg(item0->text()).arg(item1->text());
  }
  // remove last ","
  reader_inputs_map.chop(1);

  int timeCompartmentSize = this->Internals->timeCompartmentSize->value();
  command = QString(tp_export_py)
              .arg(export_rendering)
              .arg(reader_inputs_map)
              .arg(rendering_info)
              .arg(timeCompartmentSize)
              .arg(filename);

  return true;
}
