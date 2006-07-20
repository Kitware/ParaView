/*=========================================================================

   Program: ParaView
   Module:    pqDisplayProxyEditor.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

// this include
#include "pqDisplayProxyEditor.h"
#include "ui_pqDisplayProxyEditor.h"

// Qt includes
#include <QMetaType>
#include <QPointer>
#include <QtDebug>
#include <QIcon>

// VTK includes
#include "QVTKWidget.h"

// ParaView Server Manager includes
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"

// ParaView widget includes
#include "pqSignalAdaptors.h"

// ParaView client includes
#include "pqApplicationCore.h"
#include "pqSMAdaptor.h"
#include "pqPropertyLinks.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"

class pqDisplayProxyEditorInternal : public Ui::pqDisplayProxyEditor
{
public:
  pqDisplayProxyEditorInternal()
    {
    this->Links = new pqPropertyLinks;
    this->ColorAdaptor = 0;
    this->RepresentationAdaptor = 0;
    this->InterpolationAdaptor = 0;
    }

  ~pqDisplayProxyEditorInternal()
    {
    delete this->Links;
    delete this->ColorAdaptor;
    delete this->RepresentationAdaptor;
    delete this->InterpolationAdaptor;
    }

  pqPropertyLinks* Links;

  // The display whose properties are being edited.
  QPointer<pqPipelineDisplay> Display;
  pqSignalAdaptorColor* ColorAdaptor;
  pqSignalAdaptorComboBox* RepresentationAdaptor;
  pqSignalAdaptorComboBox* InterpolationAdaptor;
  
};

//-----------------------------------------------------------------------------
/// constructor
pqDisplayProxyEditor::pqDisplayProxyEditor(QWidget* p)
  : QWidget(p), DisableSlots(0)
{
  this->Internal = new pqDisplayProxyEditorInternal;
  this->Internal->setupUi(this);
  this->setupGUIConnections();
}

//-----------------------------------------------------------------------------
/// destructor
pqDisplayProxyEditor::~pqDisplayProxyEditor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
/// set the proxy to display properties for
void pqDisplayProxyEditor::setDisplay(pqPipelineDisplay* display) 
{
  if(this->Internal->Display == display)
    {
    return;
    }

  vtkSMDataObjectDisplayProxy* displayProxy = 
    (display)? display->getDisplayProxy() : NULL;
  if(this->Internal->Display)
    {
    // break all old links.
    this->Internal->Links->removeAllPropertyLinks();
    }

  this->Internal->Display = display;
  if (!display )
    {
    return;
    }
  
  // The slots are already connected but we do not want them to execute
  // while we are initializing the GUI
  this->DisableSlots = 1;
  
  // setup for visibility
  this->Internal->Links->addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    displayProxy, displayProxy->GetProperty("Visibility"));

  this->updateColorByMenu(true);

  // set up actor color
  this->Internal->Links->addPropertyLink(
    this->Internal->ColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    displayProxy, displayProxy->GetProperty("Color"));

  // setup for representation
  this->Internal->StyleRepresentation->clear();
  vtkSMProperty* Property = displayProxy->GetProperty("Representation");
  Property->UpdateDependentDomains();
  QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(
    Property);
  foreach(QVariant item, items)
    {
    if (item == "Volume" && !displayProxy->GetHasVolumePipeline())
      {
      continue; // add volume only if volume representation is supported.
      }
    this->Internal->StyleRepresentation->addItem(item.toString());
    }
  this->Internal->Links->addPropertyLink(
    this->Internal->RepresentationAdaptor, "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    displayProxy, displayProxy->GetProperty("Representation"));

  // setup for interpolation
  this->Internal->StyleInterpolation->clear();
  Property = displayProxy->GetProperty("Interpolation");
  Property->UpdateDependentDomains();
  items = pqSMAdaptor::getEnumerationPropertyDomain(
    Property);
  foreach(QVariant item, items)
    {
    this->Internal->StyleInterpolation->addItem(item.toString());
    }
  this->Internal->Links->addPropertyLink(this->Internal->InterpolationAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    displayProxy, displayProxy->GetProperty("Interpolation"));

  // setup for point size
  this->Internal->Links->addPropertyLink(this->Internal->StylePointSize,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("PointSize"));

  // setup for line width
  this->Internal->Links->addPropertyLink(this->Internal->StyleLineWidth,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("LineWidth"));

  // setup for translate
  this->Internal->Links->addPropertyLink(this->Internal->TranslateX,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Position"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->TranslateY,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Position"), 1);
  
  this->Internal->Links->addPropertyLink(this->Internal->TranslateZ,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Position"), 2);

  // setup for scale
  this->Internal->Links->addPropertyLink(this->Internal->ScaleX,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Scale"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleY,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Scale"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleZ,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Scale"), 2);

  // setup for orientation
  this->Internal->Links->addPropertyLink(this->Internal->OrientationX,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Orientation"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationY,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Orientation"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationZ,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Orientation"), 2);

  // setup for origin
  this->Internal->Links->addPropertyLink(this->Internal->OriginX,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Origin"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->OriginY,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Origin"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->OriginZ,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Origin"), 2);

  // setup for opacity
  this->Internal->Links->addPropertyLink(this->Internal->Opacity,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Opacity"));

  // setup for map scalars
  this->Internal->Links->addPropertyLink(
    this->Internal->ColorMapScalars, "checked", SIGNAL(stateChanged(int)),
    displayProxy, displayProxy->GetProperty("MapScalars"));

  // setup for InterpolateScalarsBeforeMapping
  this->Internal->Links->addPropertyLink(
    this->Internal->ColorInterpolateColors, "checked", SIGNAL(stateChanged(int)),
    displayProxy, displayProxy->GetProperty("InterpolateScalarsBeforeMapping"));

  this->DisableSlots = 0;

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqPipelineDisplay* pqDisplayProxyEditor::getDisplay()
{
  return this->Internal->Display;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::updateView()
{
  if (!this->DisableSlots && this->getDisplay())
    {
    this->getDisplay()->renderAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::setupGUIConnections()
{
  // We are usinging Queues slot execution where ever possible,
  // This ensures that the updateView() slot is called 
  // only after the vtkSMProperty has been changed by the pqPropertyLinks.
  QObject::connect(
    this->Internal->ViewData, SIGNAL(stateChanged(int)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->ColorInterpolateColors, SIGNAL(stateChanged(int)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->ColorMapScalars, SIGNAL(stateChanged(int)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->ColorBy, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(colorByChanged(const QString&)));
  QObject::connect(
    this->Internal->StylePointSize,  SIGNAL(valueChanged(double)), 
    this, SLOT(updateView()));
  QObject::connect(
    this->Internal->StyleLineWidth, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->TranslateX, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->TranslateY, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->TranslateZ, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->ScaleX, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->ScaleY, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->ScaleZ, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->OrientationX, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->OrientationY, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->OrientationZ, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->OriginX, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->OriginY, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->OriginZ, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->Opacity, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),
    Qt::QueuedConnection);
  QObject::connect(
    this->Internal->ViewZoomToData, SIGNAL(clicked(bool)), 
    this, SLOT(zoomToData()));
  QObject::connect(
    this->Internal->DismissButton, SIGNAL(clicked(bool)),
    this, SIGNAL(dismiss()));
  QObject::connect(
    this->Internal->StyleRepresentation, SIGNAL(currentIndexChanged(int)),
    this, SLOT(updateColorByMenu()), 
    Qt::QueuedConnection);
  
  // Create an connect signal adaptors.
  if (!QMetaType::isRegistered(QMetaType::type("QVariant")))
    {
    qRegisterMetaType<QVariant>("QVariant");
    }

  this->Internal->ColorAdaptor = new pqSignalAdaptorColor(
    this->Internal->ColorActorColor, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)));
  this->Internal->ColorAdaptor->setObjectName("ActorColorAdaptor");
  QObject::connect(this->Internal->ColorAdaptor, 
    SIGNAL(colorChanged(const QVariant&)), this, SLOT(updateView()),
    Qt::QueuedConnection);

  this->Internal->RepresentationAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->StyleRepresentation);
  this->Internal->RepresentationAdaptor->setObjectName(
    "StyleRepresentationAdapator");
  QObject::connect(
    this->Internal->RepresentationAdaptor,
    SIGNAL(currentTextChanged(const QString&)), 
    this, SLOT(updateView()),
    Qt::QueuedConnection);
    
  this->Internal->InterpolationAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->StyleInterpolation);
  this->Internal->InterpolationAdaptor->setObjectName(
    "StyleInterpolationAdapator");
  QObject::connect(this->Internal->InterpolationAdaptor, 
    SIGNAL(currentTextChanged(const QString&)), this, SLOT(updateView()),
    Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::updateEnableState()
{
  QString val = this->Internal->ColorBy->currentText();

  if(val == "Solid Color")
    {
    this->Internal->ColorActorColor->setEnabled(true);
    this->Internal->ColorInterpolateColors->setEnabled(false);
    }
  else
    {
    this->Internal->ColorActorColor->setEnabled(false);
    this->Internal->ColorInterpolateColors->setEnabled(true);
    }

  vtkSMDataObjectDisplayProxy* display = 
    this->Internal->Display->getDisplayProxy();
  if (display)
    {
    vtkPVGeometryInformation* geomInfo = display->GetGeometryInformation();
    vtkPVDataSetAttributesInformation* attrInfo;
    if (display->GetScalarModeCM() == 
        vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)
      {
      attrInfo = geomInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = geomInfo->GetCellDataInformation();
      }
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(
      this->Internal->Display->getColorField(true).toStdString().c_str());

    if (arrayInfo && arrayInfo->GetDataType() == VTK_UNSIGNED_CHAR)
      {
      // Number of component restriction.
      if (arrayInfo->GetNumberOfComponents() == 3)
        {
        // One component causes more trouble than it is worth.
        this->Internal->ColorMapScalars->setEnabled(true);
        return;
        }
      }
    }

  this->Internal->ColorMapScalars->setCheckState(Qt::Checked);
  this->Internal->ColorMapScalars->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::colorByChanged(const QString& val)
{
  if (this->DisableSlots)
    {
    return;
    }
  if(val == "Solid Color")
    {
    this->Internal->Display->colorByArray(NULL, 0);
    }
  else
    {
    this->Internal->Display->setColorField(val);
    }
  this->updateEnableState();
  this->updateView();
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::updateColorByMenu(bool forceUpdate)
{
  if (this->DisableSlots && !forceUpdate)
    {
    return;
    }
  
  // changing the colorby combo will cause slots to be executed and change
  // the array to be colored by to Solid Color. disabling slots to prevent
  // that
  this->DisableSlots = 1;
  this->Internal->ColorBy->clear();

  QRegExp regExpCell("\\(cell\\)\\w*$");
  QRegExp regExpPoint("\\(point\\)\\w*$");
  QList<QString> arrayNames = this->Internal->Display->getColorFields();
  foreach (const QString& name, arrayNames)
    {
    if (regExpPoint.indexIn(name) != -1)
      {
      this->Internal->ColorBy->addItem(QIcon(":/pqWidgets/Icons/pqPointData16.png"),
        name);
      }
    else if (regExpCell.indexIn(name)!= -1)
      {
      this->Internal->ColorBy->addItem(QIcon(":/pqWidgets/Icons/pqCellData16.png"),
        name);
      }
    else
      {
      this->Internal->ColorBy->addItem(QIcon(":/pqWidgets/Icons/pqSolidColor16.png"),
        name);
      }
    }
  //this->Internal->ColorBy->addItems();
  this->DisableSlots = 0;
  QString currentArray = this->Internal->Display->getColorField();
  int index = this->Internal->ColorBy->findText(currentArray);
  if (index == -1)
    {
    // Menu changed. The array is no longer available
    // Color by solid color
    currentArray = "Solid Color";
    this->Internal->ColorBy->setCurrentIndex(0);
    this->colorByChanged(currentArray);
    }
  else
    {
    this->Internal->ColorBy->setCurrentIndex(index);
    }
  
  if(currentArray == "Solid Color")
    {
    this->Internal->ColorActorColor->setEnabled(true);
    }
  else
    {
    this->Internal->ColorActorColor->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::zoomToData()
{
  if (this->DisableSlots)
    {
    return;
    }

  vtkSMDataObjectDisplayProxy* display = 
    this->Internal->Display->getDisplayProxy();

  if(!display)
    {
    qDebug() << "Cannot zoom to data, failed to locate display proxy.";
    return;
    }
  double bounds[6];
  display->GetGeometryInformation()->GetBounds(bounds);
  if (bounds[0]<=bounds[1] && bounds[2]<=bounds[3] && bounds[4]<=bounds[5])
    {
    unsigned int numRenModules;
    numRenModules = this->Internal->Display->getNumberOfRenderModules();
    for(unsigned int i=0; i<numRenModules; i++)
      {
      pqRenderModule* renModule = this->Internal->Display->getRenderModule(i);
      vtkSMRenderModuleProxy* rm = renModule->getRenderModuleProxy();
      rm->ResetCamera(bounds);
      rm->ResetCameraClippingRange();
      renModule->render();
      }
    }
}

