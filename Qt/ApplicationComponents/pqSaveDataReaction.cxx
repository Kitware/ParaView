/*=========================================================================

   Program: ParaView
   Module:    pqSaveDataReaction.cxx

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
#include "pqSaveDataReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqOptions.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "pqTestUtility.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMWriterFactory.h"
#include "vtkSmartPointer.h"

#include <QDebug>
#include <QMessageBox>

//-----------------------------------------------------------------------------
pqSaveDataReaction::pqSaveDataReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(portChanged(pqOutputPort*)), this, SLOT(updateEnableState()));

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSaveDataReaction::updateEnableState()
{
  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  // TODO: also is there's a pending accept.
  pqOutputPort* port = activeObjects.activePort();
  bool enable_state = (port != nullptr);
  if (enable_state)
  {
    vtkSMWriterFactory* writerFactory = vtkSMProxyManager::GetProxyManager()->GetWriterFactory();
    enable_state = writerFactory->CanWrite(
      vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy()), port->getPortNumber());

    if (!enable_state)
    {
      QObject::connect(port->getSource(), SIGNAL(dataUpdated(pqPipelineSource*)), this,
        SLOT(dataUpdated(pqPipelineSource*)));
    }
  }
  this->parentAction()->setEnabled(enable_state);
}
//-----------------------------------------------------------------------------
void pqSaveDataReaction::dataUpdated(pqPipelineSource* source)
{
  QObject::disconnect(
    source, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(dataUpdated(pqPipelineSource*)));
  updateEnableState();
}

//-----------------------------------------------------------------------------
bool pqSaveDataReaction::saveActiveData()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  // TODO: also is there's a pending accept.
  pqOutputPort* port = pqActiveObjects::instance().activePort();
  if (!server || !port)
  {
    qCritical("No active source located.");
    return false;
  }

  vtkSMWriterFactory* writerFactory = vtkSMProxyManager::GetProxyManager()->GetWriterFactory();
  QString filters = writerFactory->GetSupportedFileTypes(
    vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy()), port->getPortNumber());
  if (filters.isEmpty())
  {
    qCritical("Cannot determine writer to use.");
    return false;
  }

  pqFileDialog fileDialog(
    server, pqCoreUtilities::mainWidget(), tr("Save File:"), QString(), filters);
  fileDialog.setRecentlyUsedExtension(
    pqSaveDataReaction::defaultExtension(port->getDataInformation()));
  fileDialog.setObjectName("FileSaveDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    QString fname = fileDialog.getSelectedFiles()[0];
    pqSaveDataReaction::setDefaultExtension(port->getDataInformation(), QFileInfo(fname).suffix());
    return pqSaveDataReaction::saveActiveData(fname);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqSaveDataReaction::saveActiveData(const QString& filename)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  // TODO: also is there's a pending accept.
  pqOutputPort* port = pqActiveObjects::instance().activePort();
  if (!server || !port)
  {
    qCritical("No active source located.");
    return false;
  }

  vtkSMWriterFactory* writerFactory = vtkSMProxyManager::GetProxyManager()->GetWriterFactory();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(writerFactory->CreateWriter(filename.toUtf8().data(),
    vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy()), port->getPortNumber()));
  vtkSMSourceProxy* writer = vtkSMSourceProxy::SafeDownCast(proxy);
  if (!writer)
  {
    qCritical() << "Failed to create writer for: " << filename;
    return false;
  }

  if (writer->IsA("vtkSMPSWriterProxy") && port->getServer()->getNumberOfPartitions() > 1)
  {
    bool result = pqCoreUtilities::promptUser(
      // Let's try to warn separately for each type of writer.
      QString("SerialWriterWarning_%1").arg(writer->GetXMLName()), QMessageBox::Warning,
      tr("Serial Writer Warning"),
      QString(tr("This writer (%1) will collect all of the data to the first node before "
                 "writing because it does not support parallel IO. This may cause the "
                 "first node to run out of memory if the data is large.\n"
                 "Are you sure you want to continue?"))
        .arg(writer->GetXMLLabel()),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Save, pqCoreUtilities::mainWidget());
    if (!result)
    {
      return false;
    }
  }

  pqProxyWidgetDialog dialog(writer, pqCoreUtilities::mainWidget());
  dialog.setObjectName("WriterSettingsDialog");
  dialog.setEnableSearchBar(dialog.hasAdvancedProperties());
  dialog.setApplyChangesImmediately(true);
  dialog.setWindowTitle(QString("Configure Writer (%1)").arg(writer->GetXMLLabel()));

  // Check to see if this writer has any properties that can be configured by
  // the user. If it does, display the dialog.
  if (dialog.hasVisibleWidgets())
  {
    dialog.exec();
    if (dialog.result() == QDialog::Rejected)
    {
      // The user pressed Cancel so don't write
      return false;
    }
  }
  writer->UpdateVTKObjects();
  writer->UpdatePipeline();

  SM_SCOPED_TRACE(SaveData)
    .arg("writer", writer)
    .arg("filename", filename.toLocal8Bit().data())
    .arg("source", port->getSource()->getProxy())
    .arg("port", port->getPortNumber());
  return true;
}

//-----------------------------------------------------------------------------
QString pqSaveDataReaction::defaultExtension(vtkPVDataInformation* dataInfo)
{
  if (dataInfo)
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    const int dataType = dataInfo->GetCompositeDataSetType() == -1
      ? dataInfo->GetDataSetType()
      : dataInfo->GetCompositeDataSetType();
    const char* defaultExt = dataType == VTK_TABLE ? "csv" : "pvd";
    return settings->value(QString("extensions/SaveDataExtension/%1").arg(dataType), defaultExt)
      .toString();
  }
  return QString("pvd");
}

//-----------------------------------------------------------------------------
void pqSaveDataReaction::setDefaultExtension(vtkPVDataInformation* dataInfo, const QString& ext)
{
  if (dataInfo)
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    const int dataType = dataInfo->GetCompositeDataSetType() == -1
      ? dataInfo->GetDataSetType()
      : dataInfo->GetCompositeDataSetType();
    settings->setValue(QString("extensions/SaveDataExtension/%1").arg(dataType), ext);
  }
}
