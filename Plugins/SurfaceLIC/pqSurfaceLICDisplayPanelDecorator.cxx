/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSurfaceLICDisplayPanelDecorator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqSurfaceLICDisplayPanelDecorator.h"
#include "ui_pqSurfaceLICDisplayPanelDecorator.h"

// Server Manager Includes.
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSmartPointer.h"

// Qt Includes.
#include <QVBoxLayout>

// ParaView Includes.
#include "pqDisplayProxyEditor.h"
#include "pqRepresentation.h"
#include "pqFieldSelectionAdaptor.h"
#include "pqPropertyLinks.h"

class pqSurfaceLICDisplayPanelDecorator::pqInternals : 
  public Ui::pqSurfaceLICDisplayPanelDecorator
{
public:
  pqPropertyLinks Links;
  vtkSMProxy* Representation;
  QWidget* Frame;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqInternals()
    {
    this->Representation = 0;
    this->Frame = 0;
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqSurfaceLICDisplayPanelDecorator::pqSurfaceLICDisplayPanelDecorator(
  pqDisplayPanel* _panel):Superclass(_panel)
{
  this->Internals = 0;
  pqDisplayProxyEditor *panel = qobject_cast<pqDisplayProxyEditor*>(_panel);

  vtkSMProxy* repr = panel->getRepresentation()->getProxy();
  vtkSMProperty* prop = repr->GetProperty("SelectLICVectors");
  if (prop)
    {
    this->Internals = new pqInternals();
    this->Internals->Representation = repr;
    QWidget* wid = new QWidget(panel);
    this->Internals->Frame = wid;
    this->Internals->setupUi(wid);
    QVBoxLayout* l = qobject_cast<QVBoxLayout*>(panel->layout());
    l->addWidget(wid);

    pqFieldSelectionAdaptor* adaptor= new pqFieldSelectionAdaptor(
      this->Internals->Vectors, prop);

    this->Internals->Links.addPropertyLink(
      adaptor, "selection", SIGNAL(selectionChanged()),
      repr, prop);
    this->Internals->Links.addPropertyLink(
      this->Internals->NumberOfSteps, "value", SIGNAL(valueChanged(int)),
      repr, repr->GetProperty("LICNumberOfSteps"));
    this->Internals->Links.addPropertyLink(
      this->Internals->StepSize, "value", SIGNAL(valueChanged(double)),
      repr, repr->GetProperty("LICStepSize"));
    this->Internals->Links.addPropertyLink(
      this->Internals->LICIntensity, "value", SIGNAL(valueChanged(double)),
      repr, repr->GetProperty("LICIntensity"));
    this->Internals->Links.addPropertyLink(
      this->Internals->UseLICForLOD, "checked", SIGNAL(toggled(bool)),
      repr, repr->GetProperty("UseLICForLOD"));

    repr->GetProperty("Input")->UpdateDependentDomains();
    prop->UpdateDependentDomains();

    this->Internals->VTKConnect->Connect(repr->GetProperty("Representation"),
      vtkCommand::ModifiedEvent,
      this, SLOT(representationTypeChanged()));
    this->representationTypeChanged();

    QObject::connect(&this->Internals->Links, SIGNAL(smPropertyChanged()),
      panel, SLOT(updateAllViews()), Qt::QueuedConnection);
    }

}

//-----------------------------------------------------------------------------
pqSurfaceLICDisplayPanelDecorator::~pqSurfaceLICDisplayPanelDecorator()
{
  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
void pqSurfaceLICDisplayPanelDecorator::representationTypeChanged()
{
  if (this->Internals)
    {
    const char* reprType = vtkSMPropertyHelper
        ( this->Internals->Representation, "Representation" ).GetAsString();
    if ( strcmp(  reprType, "Surface LIC"  ) == 0 )
      {
      this->Internals->Frame->setEnabled(true);
      vtkSMPropertyHelper(this->Internals->Representation,
        "InterpolateScalarsBeforeMapping").Set(0);
      this->Internals->Representation->UpdateVTKObjects();
      }
    else
      {
      this->Internals->Frame->setEnabled(false);
      }
    }
}
