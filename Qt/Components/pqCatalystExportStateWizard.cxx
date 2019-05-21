/*=========================================================================

   Program: ParaView
   Module:    pqCatalystExportStateWizard.cxx

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
#include "pqCatalystExportStateWizard.h"
#include "pqSGExportStateWizardInternals.h"

#include <vtkCamera.h>
#include <vtkPVXMLElement.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>

#include <pqApplicationCore.h>
#include <pqCinemaTrack.h>
#include <pqFileDialog.h>
#include <pqImageOutputInfo.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>
#include <pqView.h>

#include <QMessageBox>

namespace
{
static const char* cp_python_export_code = "from paraview import cpexport\n"
                                           "cpexport.DumpCoProcessingScript(export_rendering=%1,\n"
                                           "   simulation_input_map={%2},\n"
                                           "   screenshot_info={%3},\n"
                                           "   padding_amount=%4,\n"
                                           "   rescale_data_range=%5,\n"
                                           "   enable_live_viz=%6,\n"
                                           "   live_viz_frequency=%7,\n"
                                           "   cinema_tracks={%8},\n"
                                           "   cinema_arrays = {%10},\n"
                                           "   filename='%9')\n";
}

//-----------------------------------------------------------------------------
pqCatalystExportStateWizard::pqCatalystExportStateWizard(
  QWidget* parentObject, Qt::WindowFlags parentFlags)
  : Superclass(parentObject, parentFlags)
{
}

//-----------------------------------------------------------------------------
pqCatalystExportStateWizard::~pqCatalystExportStateWizard()
{
}

//-----------------------------------------------------------------------------
void pqCatalystExportStateWizard::customize()
{
  this->Internals->timeCompartmentSize->hide();
  this->Internals->label_2->hide();
}

//-----------------------------------------------------------------------------
bool pqCatalystExportStateWizard::getCommandString(QString& command)
{
  QString export_rendering = this->Internals->outputRendering->isChecked() ? "True" : "False";
  QString rendering_info; // a map from the render view name to render output params
  if (this->Internals->outputRendering->isChecked() == 0 &&
    this->Internals->liveViz->isChecked() == 0)
  {
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
        vtkPVXMLElement* writerProxyHint = proxy->GetHints();
        if (writerProxyHint && writerProxyHint->FindNestedElementByName("WriterProxy"))
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
      QString message(
        tr("No output specified. Generated script should be modified to output information."));
      messageBox.setText(message);
      messageBox.exec();
    }
    else if (filtersWithoutConsumers.size() != 0)
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
  else if (this->Internals->outputRendering->isChecked())
  {
    // Format as defined in cpstate.py
    QString format("'%1' : ['%2', %3, '%4', '%5', '%6', '%7', '%8', -1]");
    rendering_info = this->Internals->wViewSelection->getSelectionAsString(format);
  }

  QString cinema_tracks;
  QString array_selection;
  if (this->Internals->outputCinema->isChecked())
  {
    // Format as defined in pv_introspect.make_cinema_store.make_cinema_store (_userDefinedValues)
    QString format("'%1' : %2");
    cinema_tracks = this->Internals->wCinemaTrackSelection->getTrackSelectionAsString(format);
    array_selection = this->Internals->wCinemaTrackSelection->getArraySelectionAsString(format);
  }

  QString filters = "ParaView Python State Files (*.py);;All files (*)";

  pqFileDialog file_dialog(NULL, this, tr("Save Server State:"), QString(), filters);
  file_dialog.setObjectName("ExportCoprocessingStateFileDialog");
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

  // the map from the simulation inputs in the paraview gui
  // to the adaptor's named inputs (usually 'input')
  QString sim_inputs_map;
  for (int cc = 0; cc < this->Internals->nameWidget->rowCount(); cc++)
  {
    QTableWidgetItem* item0 = this->Internals->nameWidget->item(cc, 0);
    QTableWidgetItem* item1 = this->Internals->nameWidget->item(cc, 1);
    sim_inputs_map += QString(" '%1' : '%2',").arg(item0->text()).arg(item1->text());
  }
  // remove last ","
  sim_inputs_map.chop(1);

  QString paddingAmount = this->Internals->fileNamePaddingAmountSpinBox->cleanText();
  if (paddingAmount.isEmpty())
  {
    paddingAmount = "0";
  }

  QString rescale_data_range =
    (this->Internals->rescaleDataRange->isChecked() == true ? "True" : "False");

  QString live_visualization = (this->Internals->liveViz->isChecked() == true ? "True" : "False");

  command = cp_python_export_code;
  // may be set by the user in the future
  const int live_visualization_frequency = 1;
  command = command.arg(export_rendering)
              .arg(sim_inputs_map)
              .arg(rendering_info)
              .arg(paddingAmount)
              .arg(rescale_data_range)
              .arg(live_visualization)
              .arg(live_visualization_frequency)
              .arg(cinema_tracks)
              .arg(filename)
              .arg(array_selection);

  return true;
}
