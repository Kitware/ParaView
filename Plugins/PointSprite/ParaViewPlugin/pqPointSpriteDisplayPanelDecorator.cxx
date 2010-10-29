/*=========================================================================

 Program:   Visualization Toolkit
 Module:    pqPointSpriteDisplayPanelDecorator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

// .NAME pqPointSpriteDisplayPanelDecorator
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "pqPointSpriteDisplayPanelDecorator.h"
#include "ui_pqPointSpriteDisplayPanelDecorator.h"

// Server Manager Includes.
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPointSpriteRepresentationProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMEnumerationDomain.h"

// Qt Includes.
#include <QVBoxLayout>
#include <QComboBox>

// ParaView Includes.
#include "pqDisplayProxyEditor.h"
#include "pqRepresentation.h"
#include "pqFieldSelectionAdaptor.h"
#include "pqPropertyLinks.h"
#include "pqSMAdaptor.h"
#include "pqPipelineRepresentation.h"
#include "pqVariableType.h"
#include "pqScalarsToColors.h"
#include "pqWidgetRangeDomain.h"

// PointSprite plugin includes
#include "pqTransferFunctionDialog.h"
#include "pqTransferFunctionEditor.h"

class pqPointSpriteDisplayPanelDecorator::pqInternals: public Ui::pqPointSpriteDisplayPanelDecorator
{
public:
  pqPropertyLinks Links;
  vtkSMProxy* RepresentationProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqPipelineRepresentation* PipelineRepresentation;

  pqTransferFunctionDialog* TransferFunctionDialog;
  pqWidgetRangeDomain* MaxPixelSizeRangeDomain;
  pqWidgetRangeDomain* OpacityRangeDomain;
  pqWidgetRangeDomain* RadiusRangeDomain;

  pqInternals(QWidget* parent)
  {
    this->RepresentationProxy = 0;
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->TransferFunctionDialog = new pqTransferFunctionDialog(parent);
    this->MaxPixelSizeRangeDomain = NULL;
    this->OpacityRangeDomain = NULL;
    this->RadiusRangeDomain = NULL;
  }
};

//-----------------------------------------------------------------------------
pqPointSpriteDisplayPanelDecorator::pqPointSpriteDisplayPanelDecorator(
    pqDisplayPanel* disp_panel) :
  Superclass(disp_panel)
{
  pqDisplayProxyEditor* panel =
      qobject_cast<pqDisplayProxyEditor*> (disp_panel);
  pqRepresentation* repr = panel->getRepresentation();
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  // vtkSMProperty* prop;
  this->Internals = NULL;

  if (!reprProxy || 
    !reprProxy->GetXMLName() ||
    (strcmp(reprProxy->GetXMLName(), "GeometryRepresentation") != 0  &&
     strcmp(reprProxy->GetXMLName(), "UnstructuredGridRepresentation") != 0  &&
     strcmp(reprProxy->GetXMLName(), "UniformGridRepresentation") != 0))
    {
    return;
    }

  if (!pqSMAdaptor::getEnumerationPropertyDomain(
      reprProxy->GetProperty("Representation")).contains("Point Sprite"))
    {
    return;
    }

  // This is not advisable, but we do it nonetheless since that's what the old
  // code was doing. At some point we need to clean this up.
  vtkSMPointSpriteRepresentationProxy::InitializeDefaultValues(reprProxy);

  this->Internals = new pqInternals(this);
  QVBoxLayout* vlayout = dynamic_cast<QVBoxLayout*> (panel->layout());
  if (vlayout)
    {
    vlayout->insertWidget(2, this);
    }
  else
    {
    panel->layout()->addWidget(this);
    }
  this->Internals->setupUi(this);
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

  this->setRepresentation(
    static_cast<pqPipelineRepresentation*> (panel->getRepresentation()));
  QObject::connect(&this->Internals->Links, SIGNAL(smPropertyChanged()), panel,
      SLOT(updateAllViews()), Qt::QueuedConnection);

  this->connect(this->Internals->OpacityMapping, SIGNAL(clicked()), this,
      SLOT(showOpacityDialog()));

  this->connect(this->Internals->RadiusMapping, SIGNAL(clicked()), this,
      SLOT(showRadiusDialog()));

  this->Internals->TransferFunctionDialog->setRepresentation(
      static_cast<pqPipelineRepresentation*> (panel->getRepresentation()));

  this->reloadGUI();

}

//-----------------------------------------------------------------------------
pqPointSpriteDisplayPanelDecorator::~pqPointSpriteDisplayPanelDecorator()
{
  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
void pqPointSpriteDisplayPanelDecorator::representationTypeChanged()
{
  if (this->Internals)
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
}

void pqPointSpriteDisplayPanelDecorator::setupGUIConnections()
{
  this->Internals->VTKConnect->Connect(
      this->Internals->RepresentationProxy->GetProperty("Representation"),
      vtkCommand::ModifiedEvent, this, SLOT(representationTypeChanged()));

  this->connect(this->Internals->ScaleBy,
      SIGNAL(variableChanged(pqVariableType, const QString&)), this,
      SLOT(onRadiusArrayChanged(pqVariableType, const QString&)));

  this->connect(this->Internals->ScaleBy, SIGNAL(componentChanged(int, int)),
      this, SLOT(onRadiusComponentChanged(int, int)));

  this->connect(this->Internals->OpacityBy,
      SIGNAL(variableChanged(pqVariableType, const QString&)), this,
      SLOT(onOpacityArrayChanged(pqVariableType, const QString&)));

  this->connect(this->Internals->OpacityBy, SIGNAL(componentChanged(int, int)),
      this, SLOT(onOpacityComponentChanged(int, int)));

  this->connect(this->Internals->RenderMode, SIGNAL(activated(int)),
      this->Internals->TextureCombo, SLOT(setRenderMode(int)));

}

void pqPointSpriteDisplayPanelDecorator::setRepresentation(
    pqPipelineRepresentation* repr)
{
  if (this->Internals->PipelineRepresentation == repr)
    {
    return;
    }

  //vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  vtkSMProperty* prop;
  //QWidget* widget;
  if (this->Internals->PipelineRepresentation)
    {
    // break all old links.
    this->Internals->Links.removeAllPropertyLinks();
    }

  this->Internals->PipelineRepresentation = repr;
  if (!repr)
    {
    this->Internals->TransferFunctionDialog->hide();
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
    prop->UpdateDependentDomains();
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

  representationTypeChanged();
}

void pqPointSpriteDisplayPanelDecorator::LinkWithRange(QWidget* widget,
    const char* signal, vtkSMProperty* prop,
    pqWidgetRangeDomain*& widgetRangeDomain)
{
  if (!prop || !widget)
    return;

  prop->UpdateDependentDomains();

  if (widgetRangeDomain != NULL)
    {
    delete widgetRangeDomain;
    }
  widgetRangeDomain = new pqWidgetRangeDomain(widget, "minimum", "maximum",
      prop);

  this->Internals->Links.addPropertyLink(widget, "value", signal,
      this->Internals->RepresentationProxy, prop);
}

void pqPointSpriteDisplayPanelDecorator::reloadGUI()
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

void pqPointSpriteDisplayPanelDecorator::onRadiusArrayChanged(
    pqVariableType type, const QString& name)
{
  pqPipelineRepresentation* repr = this->Internals->PipelineRepresentation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    {
    return;
    }

  if (type == VARIABLE_TYPE_NONE)
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
  svp->SetElement(0, 0); // idx
  svp->SetElement(1, 0); //port
  svp->SetElement(2, 0); //connection
  svp->SetElement(3, (int) vtkDataObject::FIELD_ASSOCIATION_POINTS); //type
  svp->SetElement(4, name.toAscii().data()); //name

  this->Internals->TransferFunctionDialog->radiusEditor()->needReloadGUI();

  reprProxy->UpdateVTKObjects();
  this->updateAllViews();
}

void pqPointSpriteDisplayPanelDecorator::onOpacityArrayChanged(
    pqVariableType type, const QString& name)
{
  pqPipelineRepresentation* repr = this->Internals->PipelineRepresentation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    return;

  double opacity = pqSMAdaptor::getElementProperty(reprProxy->GetProperty(
      "Opacity")).toDouble();

  if (type == VARIABLE_TYPE_NONE)
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
  svp->SetElement(0, 0); // idx
  svp->SetElement(1, 0); //port
  svp->SetElement(2, 0); //connection
  svp->SetElement(3, (int) vtkDataObject::FIELD_ASSOCIATION_POINTS); //type
  svp->SetElement(4, name.toAscii().data()); //name

  this->Internals->TransferFunctionDialog->opacityEditor()->needReloadGUI();

  reprProxy->UpdateVTKObjects();
  this->updateAllViews();
}

void pqPointSpriteDisplayPanelDecorator::onRadiusComponentChanged(
    int vectorMode, int comp)
{
  pqPipelineRepresentation* repr = this->Internals->PipelineRepresentation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    return;

  if (vectorMode == pqScalarsToColors::MAGNITUDE)
    {
    comp = -1;
    }

  pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
      "RadiusVectorComponent"), comp);

  this->Internals->TransferFunctionDialog->radiusEditor()->needReloadGUI();

  reprProxy->UpdateVTKObjects();
  this->updateAllViews();
}

void pqPointSpriteDisplayPanelDecorator::onOpacityComponentChanged(
    int vectorMode, int comp)
{
  pqPipelineRepresentation* repr = this->Internals->PipelineRepresentation;
  vtkSMProxy* reprProxy = (repr) ? repr->getProxy() : NULL;
  if (!reprProxy)
    return;

  if (vectorMode == pqScalarsToColors::MAGNITUDE)
    {
    comp = -1;
    }

  pqSMAdaptor::setElementProperty(reprProxy->GetProperty(
      "OpacityVectorComponent"), comp);

  this->Internals->TransferFunctionDialog->opacityEditor()->needReloadGUI();

  reprProxy->UpdateVTKObjects();
  this->updateAllViews();
}

void pqPointSpriteDisplayPanelDecorator::updateEnableState()
{
  if (this->Internals->ScaleBy->getCurrentText() == "Constant Radius")
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

  if (this->Internals->OpacityBy->getCurrentText() == "Constant Opacity")
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

void pqPointSpriteDisplayPanelDecorator::updateAllViews()
{
  if (this->Internals->PipelineRepresentation)
    {
    this->Internals->PipelineRepresentation->renderViewEventually();
    }
}

void pqPointSpriteDisplayPanelDecorator::showOpacityDialog()
{
  this->Internals->TransferFunctionDialog->show(
      this->Internals->TransferFunctionDialog->opacityEditor());
}

void pqPointSpriteDisplayPanelDecorator::showRadiusDialog()
{
  this->Internals->TransferFunctionDialog->show(
      this->Internals->TransferFunctionDialog->radiusEditor());
}

