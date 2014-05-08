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
#include "pqPointSpriteControls.h"
#include "ui_pqPointSpriteControls.h"

#include "pqApplicationCore.h"
#include "pqFieldSelectionAdaptor.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
#include "pqWidgetRangeDomain.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMPointSpriteRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkWeakPointer.h"

// PointSprite plugin includes
#include "pqTransferFunctionDialog.h"
#include "pqTransferFunctionEditor.h"


#include <QPointer>

class pqPointSpriteControls::pqInternals : public Ui::PointSpriteControls
{
public:
  pqPropertyLinks Links;
  vtkWeakPointer<vtkSMProxy> RepresentationProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqPipelineRepresentation> PipelineRepresentation;

  QPointer<pqTransferFunctionDialog> TransferFunctionDialog;
  QPointer<pqWidgetRangeDomain> MaxPixelSizeRangeDomain;
  QPointer<pqWidgetRangeDomain> OpacityRangeDomain;
  QPointer<pqWidgetRangeDomain> RadiusRangeDomain;

  pqInternals(QWidget* parent)
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->TransferFunctionDialog = new pqTransferFunctionDialog(parent);

    this->setupUi(parent);
    this->gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    }
};

//-----------------------------------------------------------------------------
pqPointSpriteControls::pqPointSpriteControls(
    vtkSMProxy* smproxy, vtkSMPropertyGroup*, QWidget* parentObject)
  : Superclass(smproxy, parentObject),
  Internals(new pqPointSpriteControls::pqInternals(this))
{
  this->setShowLabel(true);

  pqPipelineRepresentation* repr =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<
    pqPipelineRepresentation*>(smproxy);

  this->initialize(repr);
  QObject::connect(&this->Internals->Links, SIGNAL(smPropertyChanged()),
                    this, SIGNAL(changeFinished()));
}

//-----------------------------------------------------------------------------
pqPointSpriteControls::~pqPointSpriteControls()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::initialize(pqPipelineRepresentation* repr)
{
  vtkSMProxy* reprProxy = repr->getProxy();

  BEGIN_UNDO_EXCLUDE();

  // This is not advisable, but we do it nonetheless since that's what the old
  // code was doing. At some point we need to clean this up.
  vtkSMPointSpriteRepresentationProxy::InitializeDefaultValues(reprProxy);

  this->Internals->RepresentationProxy = reprProxy;

  // setup the scaleBy and radiusBy menus
  this->Internals->ScaleBy->setConstantVariableName("Constant Radius");
  this->Internals->ScaleBy->setPropertyArrayName("RadiusArray");
  this->Internals->ScaleBy->setPropertyArrayComponent("RadiusVectorComponent");
  this->Internals->ScaleBy->setToolTip(
    "select method for scaling the point sprites.");

  this->Internals->OpacityBy->setConstantVariableName("Constant Opacity");
  this->Internals->OpacityBy->setPropertyArrayName("OpacityArray");
  this->Internals->OpacityBy->setPropertyArrayComponent(
    "OpacityVectorComponent");
  this->Internals->OpacityBy->setToolTip(
    "select method for setting the opacity of the point sprites.");

  this->Internals->ScaleBy->reloadGUI();
  this->Internals->OpacityBy->reloadGUI();

  this->setupGUIConnections();

  this->setRepresentation(repr);

  this->connect(this->Internals->OpacityMapping, SIGNAL(clicked()), this,
      SLOT(showOpacityDialog()));

  this->connect(this->Internals->RadiusMapping, SIGNAL(clicked()), this,
      SLOT(showRadiusDialog()));

  this->Internals->TransferFunctionDialog->setRepresentation(repr);

  this->reloadGUI();

  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::representationTypeChanged()
{
  vtkSMEnumerationDomain* enumDomain = (this->Internals->RepresentationProxy ?
    vtkSMEnumerationDomain::SafeDownCast(
      this->Internals->RepresentationProxy->GetProperty("Representation")->GetDomain("enum")) : 0);
  if (enumDomain)
    {
    int found = 0;
    unsigned int entry;
    for(entry = 0; entry < enumDomain->GetNumberOfEntries(); entry++)
      {
      const char* text = enumDomain->GetEntryText(entry);
      if(strcmp(text , "Point Sprite") == 0)
        {
        found = 1;
        break;
        }
      }
    int reprType = vtkSMPropertyHelper(this->Internals->RepresentationProxy, "Representation").GetAsInt();
    if (found && reprType == enumDomain->GetEntryValue(entry))
      {
      this->setEnabled(true);
      vtkSMPropertyHelper(this->Internals->RepresentationProxy,
        "InterpolateScalarsBeforeMapping").Set(0);
      if (this->Internals->PipelineRepresentation)
        {
        this->Internals->TextureCombo->setRenderMode(
          this->Internals->RenderMode->currentIndex());
        }
      this->Internals->RepresentationProxy->UpdateVTKObjects();
      }
    else
      {
      if (this->Internals->PipelineRepresentation)
        {
        this->Internals->TextureCombo->setRenderMode(-1);
        }
      this->Internals->TransferFunctionDialog->hide();
      this->setEnabled(false);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::setupGUIConnections()
{
  this->Internals->VTKConnect->Connect(
      this->Internals->RepresentationProxy->GetProperty("Representation"),
      vtkCommand::ModifiedEvent, this, SLOT(representationTypeChanged()));

  this->connect(this->Internals->ScaleBy,
      SIGNAL(variableChanged(const QString&)),
      SLOT(updateRadiusArray()));
  this->connect(
    this->Internals->ScaleBy, SIGNAL(componentChanged(int, int)),
    SLOT(updateRadiusArray()));
  this->connect(
    this->Internals->OpacityBy, SIGNAL(variableChanged(const QString&)),
    SLOT(updateOpacityArray()));
  this->connect(
    this->Internals->OpacityBy, SIGNAL(componentChanged(int, int)),
    SLOT(updateOpacityArray()));

  this->connect(this->Internals->RenderMode, SIGNAL(activated(int)),
      this->Internals->TextureCombo, SLOT(setRenderMode(int)));
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::setRepresentation(pqPipelineRepresentation* repr)
{
  this->Internals->PipelineRepresentation = repr;
  vtkSMProperty* prop;
  if (!repr)
    {
    return;
    }

  this->Internals->TextureCombo->setRepresentation(repr);
  this->Internals->TransferFunctionDialog->radiusEditor()->setRepresentation(
      repr);
  this->Internals->TransferFunctionDialog->opacityEditor()->setRepresentation(
      repr);

  this->Internals->ScaleBy->setRepresentation(repr);
  QObject::connect(this->Internals->ScaleBy, SIGNAL(modified()), this,
      SLOT(updateEnableState()), Qt::QueuedConnection);

  this->Internals->OpacityBy->setRepresentation(repr);
  QObject::connect(this->Internals->ScaleBy, SIGNAL(modified()), this,
      SLOT(updateEnableState()), Qt::QueuedConnection);

  // setup for render mode
  if ((prop = this->Internals->RepresentationProxy->GetProperty("RenderMode")))
    {
    QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(prop);
    foreach(QVariant item, items)
        {
        this->Internals->RenderMode->addItem(item.toString());
        }
    this->Internals->Links.addPropertyLink(this->Internals->RenderMode,
        "currentText", SIGNAL(currentIndexChanged(int)),
        this->Internals->RepresentationProxy, prop);
    this->Internals->RenderMode->setEnabled(true);
    }
  else
    {
    this->Internals->RenderMode->setEnabled(false);
    }

  this->LinkWithRange(this->Internals->MaxPixelSize, SIGNAL(valueChanged(int)),
      this->Internals->RepresentationProxy->GetProperty("MaxPixelSize"),
      this->Internals->MaxPixelSizeRangeDomain);

  this->LinkWithRange(this->Internals->RadiusEdit,
      SIGNAL(valueChanged(double)),
      this->Internals->RepresentationProxy->GetProperty("ConstantRadius"),
      this->Internals->RadiusRangeDomain);

  this->LinkWithRange(this->Internals->OpacitySpin,
      SIGNAL(valueChanged(double)),
      this->Internals->RepresentationProxy->GetProperty("Opacity"),
      this->Internals->OpacityRangeDomain);

  this->representationTypeChanged();
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::LinkWithRange(QWidget* widget,
    const char* signal, vtkSMProperty* prop,
    QPointer<pqWidgetRangeDomain>& widgetRangeDomain)
{
  if (!prop || !widget)
    return;

  if (widgetRangeDomain != NULL)
    {
    delete widgetRangeDomain;
    }
  widgetRangeDomain = new pqWidgetRangeDomain(widget, "minimum", "maximum",
      prop);

  this->Internals->Links.addPropertyLink(widget, "value", signal,
      this->Internals->RepresentationProxy, prop);
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::reloadGUI()
{
  pqPipelineRepresentation* repr = this->Internals->PipelineRepresentation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    {
    return;
    }

  vtkSMProperty* prop = reprProxy->GetProperty("RenderMode");
  QVariant value = pqSMAdaptor::getEnumerationProperty(prop);
  QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(prop);
  int index;
  for (index = 0; index < items.size(); index++)
    {
    if (items.at(index) == value)
      {
      this->Internals->RenderMode->setCurrentIndex(index);
      this->Internals->TextureCombo->setRenderMode(index);
      break;
      }
    }

  this->Internals->OpacityBy->reloadGUI();
  this->Internals->ScaleBy->reloadGUI();
  this->Internals->TransferFunctionDialog->radiusEditor()->needReloadGUI();
  this->Internals->TransferFunctionDialog->opacityEditor()->needReloadGUI();

}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::updateRadiusArray()
{
  pqPipelineRepresentation* repr = this->Internals->PipelineRepresentation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    {
    return;
    }

  QString array = this->Internals->ScaleBy->currentVariableName();
  if (array.isEmpty())
    {
    pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty("RadiusMode"),
      "Constant");// this is to set the vtkPointSpriteProperty radius mode to constant
    // disable the transfer function filter
    pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
        "RadiusTransferFunctionEnabled"), 0);
    }
  else
    {
    pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty("RadiusMode"),
        "Scalar");
    // enable the transfer function filter
    pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
        "RadiusTransferFunctionEnabled"), 1);
    }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    reprProxy->GetProperty("RadiusArray"));
  svp->SetElement(0, "0"); // idx
  svp->SetElement(1, "0"); //port
  svp->SetElement(2, "0"); //connection
  svp->SetElement(3, "0" /* vtkDataObject::FIELD_ASSOCIATION_POINTS */); //type
  svp->SetElement(4, array.toLatin1().data()); //name
  reprProxy->UpdateVTKObjects();

  pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
      "RadiusVectorComponent"),
    this->Internals->ScaleBy->currentComponent());
  this->Internals->TransferFunctionDialog->radiusEditor()->needReloadGUI();
  this->Internals->ScaleBy->reloadGUI();
  emit this->changeFinished();
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::updateOpacityArray()
{
  pqPipelineRepresentation* repr = this->Internals->PipelineRepresentation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    {
    return;
    }

  double opacity = pqSMAdaptor::getElementProperty(
    reprProxy->GetProperty("Opacity")).toDouble();
  QString array = this->Internals->OpacityBy->currentVariableName();

  if (array.isEmpty())
    {
    // disable the transfer function filter
    pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
        "OpacityTransferFunctionEnabled"), 0);
    // disable the painter that merges the color and the opacity
    pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
        "OpacityPainterEnabled"), 0);
    // HACK : this is to tell the renderer that this actor is no longer translucent
    if (opacity == 0.9999)
      pqSMAdaptor::setElementProperty(reprProxy->GetProperty("Opacity"), 1.0);
    }
  else
    {
    // enable the transfer function filter
    pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
        "OpacityTransferFunctionEnabled"), 1);
    // enable the painter that merges the color and the opacity
    pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
        "OpacityPainterEnabled"), 1);
    // HACK : this is to tell the renderer that this actor is translucent
    if (opacity == 1.0)
      pqSMAdaptor::setElementProperty(reprProxy->GetProperty("Opacity"), 0.9999);
    }
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      reprProxy->GetProperty("OpacityArray"));
  svp->SetElement(0, "0"); // idx
  svp->SetElement(1, "0"); //port
  svp->SetElement(2, "0"); //connection
  svp->SetElement(3, "0" /* vtkDataObject::FIELD_ASSOCIATION_POINTS */); //type
  svp->SetElement(4, array.toLatin1().data()); //name

  pqSMAdaptor::setElementProperty(
    reprProxy->GetProperty("OpacityVectorComponent"),
    this->Internals->OpacityBy->currentComponent());
  reprProxy->UpdateVTKObjects();

  this->Internals->TransferFunctionDialog->opacityEditor()->needReloadGUI();
  this->Internals->OpacityBy->reloadGUI();

  emit this->changeFinished();
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::updateEnableState()
{
  if (this->Internals->ScaleBy->currentVariableName().isEmpty())
    {
    this->Internals->RadiusStack->setCurrentWidget(
        this->Internals->ConstantRadiusPage);
    this->Internals->TransferFunctionDialog->radiusEditor()->setEnabled(false);
    }
  else
    {
    this->Internals->RadiusStack->setCurrentWidget(
        this->Internals->MappingRadiusPage);
    this->Internals->TransferFunctionDialog->radiusEditor()->setEnabled(true);
    }

  if (this->Internals->OpacityBy->currentVariableName().isEmpty())
    {
    this->Internals->OpacityStack->setCurrentWidget(
        this->Internals->ConstantOpacityPage);
    this->Internals->TransferFunctionDialog->opacityEditor()->setEnabled(false);
    }
  else
    {
    this->Internals->OpacityStack->setCurrentWidget(
        this->Internals->MappingOpacityPage);
    this->Internals->TransferFunctionDialog->opacityEditor()->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::showOpacityDialog()
{
  this->Internals->TransferFunctionDialog->show(
      this->Internals->TransferFunctionDialog->opacityEditor());
}

//-----------------------------------------------------------------------------
void pqPointSpriteControls::showRadiusDialog()
{
  this->Internals->TransferFunctionDialog->show(
      this->Internals->TransferFunctionDialog->radiusEditor());
}
