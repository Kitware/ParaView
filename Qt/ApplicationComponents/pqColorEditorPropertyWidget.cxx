/*=========================================================================

   Program: ParaView
   Module: pqColorEditorPropertyWidget.h

   Copyright (c) 2005-2012 Kitware Inc.
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
#include "pqColorEditorPropertyWidget.h"
#include "ui_pqColorEditorPropertyWidget.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqEditColorMapReaction.h"
#include "pqPropertiesPanel.h"
#include "pqResetScalarRangeReaction.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqServerManagerModel.h"

class pqColorEditorPropertyWidget::pqInternals
{
public:
  Ui::ColorEditorPropertyWidget Ui;
  QPointer<QAction> ScalarBarVisibilityAction;
};

//-----------------------------------------------------------------------------
pqColorEditorPropertyWidget::pqColorEditorPropertyWidget(vtkSMProxy *smProxy,
  QWidget *parentObject) :
  Superclass(smProxy, parentObject),
  Internals(new pqColorEditorPropertyWidget::pqInternals())
{
  this->setShowLabel(true);

  Ui::ColorEditorPropertyWidget &Ui = this->Internals->Ui;
  Ui.setupUi(this);
  Ui.gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
  Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

  // Setup various widget properties.
  pqServerManagerModel *smm = pqApplicationCore::instance()->getServerManagerModel();
  pqProxy *pqproxy = smm->findItem<pqProxy *>(smProxy);
  pqDataRepresentation *representation = qobject_cast<pqDataRepresentation*>(pqproxy);
  Ui.DisplayColorWidget->setRepresentation(representation);

  // show scalar bar button
  QAction *scalarBarAction = new QAction(this);
  this->Internals->ScalarBarVisibilityAction = scalarBarAction;
  QObject::connect(Ui.ShowScalarBar, SIGNAL(clicked(bool)), scalarBarAction, SLOT(trigger()));
  QObject::connect(scalarBarAction, SIGNAL(changed()),
                   this, SLOT(updateEnableState()));
  QObject::connect(scalarBarAction, SIGNAL(toggled(bool)),
                   Ui.ShowScalarBar, SLOT(setChecked(bool)));
  new pqScalarBarVisibilityReaction(scalarBarAction);


  // edit color map button
  QAction *editColorMapAction = new QAction(this);
  QObject::connect(Ui.EditColorMap, SIGNAL(clicked()), editColorMapAction, SLOT(trigger()));
  new pqEditColorMapReaction(editColorMapAction);

  // reset range button
  QAction *resetRangeAction = new QAction(this);
  QObject::connect(Ui.Rescale, SIGNAL(clicked()), resetRangeAction, SLOT(trigger()));
  new pqResetScalarRangeReaction(resetRangeAction);

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqColorEditorPropertyWidget::~pqColorEditorPropertyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqColorEditorPropertyWidget::updateEnableState()
{
  this->Internals->Ui.ShowScalarBar->setEnabled(
    this->Internals->ScalarBarVisibilityAction->isEnabled());
}
