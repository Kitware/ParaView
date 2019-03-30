/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqFindDataDialog.h"
#include "ui_pqFindDataDialog.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelectionNode.h"

#include <QMessageBox>

#include <cassert>

class pqFindDataDialog::pqInternals
{
public:
  Ui::FindDataDialog Ui;

  pqInternals(pqFindDataDialog* self)
  {
    // Setup the UI.
    this->Ui.setupUi(self);
    self->connect(
      this->Ui.currentSelectionFrame, SIGNAL(showing(pqOutputPort*)), SLOT(showing(pqOutputPort*)));

    self->connect(this->Ui.freezeSelection, SIGNAL(clicked()), SLOT(freezeSelection()));
    self->connect(this->Ui.extractSelection, SIGNAL(clicked()), SLOT(extractSelection()));
    self->connect(
      this->Ui.extractSelectionOverTime, SIGNAL(clicked()), SLOT(extractSelectionOverTime()));
  }
};

//-----------------------------------------------------------------------------
pqFindDataDialog::pqFindDataDialog(QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
  , Internals(new pqInternals(this))
{
  this->showing(this->Internals->Ui.currentSelectionFrame->showingPort());
}

//-----------------------------------------------------------------------------
pqFindDataDialog::~pqFindDataDialog()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqFindDataDialog::showing(pqOutputPort* port)
{
  if (port)
  {
    this->Internals->Ui.currentSelectionLabel->setText(QString("Current Selection (%1 : %2)")
                                                         .arg(port->getSource()->getSMName())
                                                         .arg(port->getPortNumber()));
  }
  else
  {
    this->Internals->Ui.currentSelectionLabel->setText(QString("Current Selection (none)"));
  }

  // enable extraction buttons only if we have an active selection.
  this->Internals->Ui.freezeSelection->setEnabled(port != NULL);
  this->Internals->Ui.extractSelection->setEnabled(port != NULL);
  this->Internals->Ui.extractSelectionOverTime->setEnabled(port != NULL);
}

//-----------------------------------------------------------------------------
void pqFindDataDialog::freezeSelection()
{
  pqOutputPort* port = this->Internals->Ui.currentSelectionFrame->showingPort();
  assert(port != NULL);

  vtkSMSourceProxy* curSelSource = static_cast<vtkSMSourceProxy*>(port->getSelectionInput());

  if (curSelSource && port->getServer()->isRemote())
  {
    // BUG: 6783. Warn user when converting a Frustum|Threshold|Query selection to
    // an id based selection.
    if (strcmp(curSelSource->GetXMLName(), "FrustumSelectionSource") == 0 ||
      strcmp(curSelSource->GetXMLName(), "ThresholdSelectionSource") == 0 ||
      strcmp(curSelSource->GetXMLName(), "SelectionQuerySource") == 0)
    {
      // We need to determine how many ids are present approximately.
      vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy());
      vtkPVDataInformation* selectedInformation =
        sourceProxy->GetSelectionOutput(port->getPortNumber())->GetDataInformation();
      int fdType = vtkSMPropertyHelper(curSelSource, "FieldType").GetAsInt();
      if ((fdType == vtkSelectionNode::POINT && selectedInformation->GetNumberOfPoints() > 10000) ||
        (fdType == vtkSelectionNode::CELL && selectedInformation->GetNumberOfCells() > 10000))
      {
        if (QMessageBox::warning(this, tr("Convert Selection"),
              tr("This selection conversion can potentially result in fetching a "
                 "large amount of data to the client.\n"
                 "Are you sure you want to continue?"),
              QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok)
        {
          return;
        }
      }
    }
  }
  if (!curSelSource)
  {
    return;
  }

  vtkSMSourceProxy* selSource = vtkSMSourceProxy::SafeDownCast(
    vtkSMSelectionHelper::ConvertSelection(vtkSelectionNode::INDICES, curSelSource,
      vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy()), port->getPortNumber()));
  if (selSource)
  {
    if (selSource != curSelSource)
    {
      selSource->UpdateVTKObjects();
      port->setSelectionInput(selSource, 0);
    }
    selSource->Delete();
  }
}

//-----------------------------------------------------------------------------
void pqFindDataDialog::extractSelection()
{
  pqOutputPort* port = this->Internals->Ui.currentSelectionFrame->showingPort();
  assert(port != NULL);
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "ExtractSelection", port->getSource(), port->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqFindDataDialog::extractSelectionOverTime()
{
  pqOutputPort* port = this->Internals->Ui.currentSelectionFrame->showingPort();
  assert(port != NULL);
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter(
    "filters", "ExtractSelectionOverTime", port->getSource(), port->getPortNumber());
}
