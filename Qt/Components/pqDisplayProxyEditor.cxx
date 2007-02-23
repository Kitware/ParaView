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
#include <QFileInfo>

// VTK includes
#include "QVTKWidget.h"

// ParaView Server Manager includes
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkMaterialLibrary.h"

// ParaView widget includes
#include "pqSignalAdaptors.h"

// ParaView client includes
#include "pqApplicationCore.h"
#include "pqColorMapEditor.h"
#include "pqSMAdaptor.h"
#include "pqPropertyLinks.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqRenderViewModule.h"
#include "pqFileDialog.h"

class pqDisplayProxyEditorInternal : public Ui::pqDisplayProxyEditor
{
public:
  pqDisplayProxyEditorInternal()
    {
    this->Links = new pqPropertyLinks;
    this->InterpolationAdaptor = 0;
    this->ColorAdaptor = 0;
    }

  ~pqDisplayProxyEditorInternal()
    {
    delete this->Links;
    delete this->InterpolationAdaptor;
    }

  pqPropertyLinks* Links;

  // The display whose properties are being edited.
  QPointer<pqPipelineDisplay> Display;
  pqSignalAdaptorComboBox* InterpolationAdaptor;
  pqSignalAdaptorColor*    ColorAdaptor;

  // map of <material labels, material files>
  static QMap<QString, QString> MaterialMap;
  
};

QMap<QString, QString> pqDisplayProxyEditorInternal::MaterialMap;

//-----------------------------------------------------------------------------
/// constructor
pqDisplayProxyEditor::pqDisplayProxyEditor(pqPipelineDisplay* display, QWidget* p)
  : pqDisplayPanel(display, p), DisableSlots(0)
{
  this->Internal = new pqDisplayProxyEditorInternal;
  this->Internal->setupUi(this);
  this->setupGUIConnections();

  // setting a display proxy will enable this
  this->setEnabled(false);

  this->setDisplay(display);
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
    this->setEnabled(false);
    return;
    }
  else
    {
    this->setEnabled(true);
    }
  
  // The slots are already connected but we do not want them to execute
  // while we are initializing the GUI
  this->DisableSlots = 1;
  
  // setup for visibility
  this->Internal->Links->addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    displayProxy, displayProxy->GetProperty("Visibility"));

  // setup for choosing color
  this->Internal->Links->addPropertyLink(this->Internal->ColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    displayProxy, displayProxy->GetProperty("AmbientColor"));
  this->Internal->Links->addPropertyLink(this->Internal->ColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    displayProxy, displayProxy->GetProperty("DiffuseColor"));

  // setup for specular lighting
  QObject::connect(this->Internal->SpecularWhite, SIGNAL(toggled(bool)),
                   this, SIGNAL(specularColorChanged()));
  QObject::connect(this->Internal->ColorAdaptor,
                   SIGNAL(colorChanged(const QVariant&)),
                   this, SIGNAL(specularColorChanged()));
  this->Internal->Links->addPropertyLink(this->Internal->SpecularIntensity,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Specular"));
  this->Internal->Links->addPropertyLink(this,
    "specularColor", SIGNAL(specularColorChanged()),
    displayProxy, displayProxy->GetProperty("SpecularColor"));
  this->Internal->Links->addPropertyLink(this->Internal->SpecularPower,
    "value", SIGNAL(valueChanged(int)),
    displayProxy, displayProxy->GetProperty("SpecularPower"));
  QObject::connect(this->Internal->SpecularIntensity, SIGNAL(valueChanged(double)),
                   this, SLOT(updateView()),
                   Qt::QueuedConnection);
  QObject::connect(this, SIGNAL(specularColorChanged()),
                   this, SLOT(updateView()),
                   Qt::QueuedConnection);
  QObject::connect(this->Internal->SpecularPower, SIGNAL(valueChanged(int)),
                   this, SLOT(updateView()),
                   Qt::QueuedConnection);
  
  // setup for interpolation
  this->Internal->StyleInterpolation->clear();
  vtkSMProperty* Property = displayProxy->GetProperty("Interpolation");
  Property->UpdateDependentDomains();
  QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(
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

  this->Internal->ColorBy->setDisplay(display);
  QObject::connect(this->Internal->ColorBy,
    SIGNAL(variableChanged(pqVariableType, const QString&)),
    this, SLOT(updateEnableState()));

  this->Internal->StyleRepresentation->setDisplay(display);
  QObject::connect(this->Internal->StyleRepresentation,
    SIGNAL(currentTextChanged(const QString&)),
    this->Internal->ColorBy, SLOT(reloadGUI()));
  
  // material
  this->Internal->StyleMaterial->blockSignals(true);
  this->Internal->StyleMaterial->clear();
  if(vtkMaterialLibrary::GetNumberOfMaterials() > 0)
    {
    this->Internal->StyleMaterial->addItem("None");
    this->Internal->StyleMaterial->addItem("Browse...");
    this->Internal->StyleMaterial->addItems(this->Internal->MaterialMap.keys());
    const char* mat = this->Internal->Display->getDisplayProxy()->GetMaterialCM();
    if(mat)
      {
      QString filename = mat;
      QMap<QString, QString>::iterator iter;
      for(iter = this->Internal->MaterialMap.begin();
          iter != this->Internal->MaterialMap.end();
          ++iter)
        {
        if(filename == iter.value())
          {
          int foundidx = this->Internal->StyleMaterial->findText(iter.key());
          this->Internal->StyleMaterial->setCurrentIndex(foundidx);
          return;
          }
        }
      }
    }
  else
    {
    this->Internal->StyleMaterial->addItem("Unavailable");
    this->Internal->StyleMaterial->setEnabled(false);
    }
  this->Internal->StyleMaterial->blockSignals(false);

  this->DisableSlots = 0;

  this->updateEnableState();
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
    this->Internal->EditColorMapButton, SIGNAL(clicked()),
    this, SLOT(openColorMapEditor()));
  
  // Create an connect signal adaptors.
  if (!QMetaType::isRegistered(QMetaType::type("QVariant")))
    {
    qRegisterMetaType<QVariant>("QVariant");
    }

  this->Internal->InterpolationAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->StyleInterpolation);
  this->Internal->InterpolationAdaptor->setObjectName(
    "StyleInterpolationAdapator");
  QObject::connect(this->Internal->InterpolationAdaptor, 
    SIGNAL(currentTextChanged(const QString&)), this, SLOT(updateView()),
    Qt::QueuedConnection);
    
  this->Internal->ColorAdaptor = new pqSignalAdaptorColor(
                            this->Internal->ColorActorColor,
                            "chosenColor",
                            SIGNAL(chosenColorChanged(const QColor&)), false);
  QObject::connect(
    this->Internal->ColorActorColor, SIGNAL(chosenColorChanged(const QColor&)),
    this, SLOT(updateView()));
  
  QObject::connect(this->Internal->StyleMaterial, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(updateMaterial(int)));

}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::updateEnableState()
{
  QString val = this->Internal->Display->getColorField(); 

  if(val == "Solid Color")
    {
    this->Internal->ColorActorColor->setEnabled(true);
    this->Internal->ColorInterpolateColors->setEnabled(false);
    this->Internal->EditColorMapButton->setEnabled(false);
    }
  else
    {
    this->Internal->ColorActorColor->setEnabled(false);
    this->Internal->ColorInterpolateColors->setEnabled(true);
    this->Internal->EditColorMapButton->setEnabled(true);
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
      this->Internal->Display->getColorField(true).toAscii().data());

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
void pqDisplayProxyEditor::openColorMapEditor()
{
  if(this->Internal->Display.isNull())
    {
    return;
    }

  // Create a color map editor and set the display.
  pqColorMapEditor colorMap(this);
  colorMap.setDisplay(this->Internal->Display);
  colorMap.exec();
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
    numRenModules = this->Internal->Display->getNumberOfViewModules();
    for(unsigned int i=0; i<numRenModules; i++)
      {
      pqRenderViewModule* renModule = qobject_cast<pqRenderViewModule*>(
        this->Internal->Display->getViewModule(i));
      if (renModule)
        {
        vtkSMRenderModuleProxy* rm = renModule->getRenderModuleProxy();
        rm->ResetCamera(bounds);
        rm->ResetCameraClippingRange();
        renModule->render();
        }
      }
    }
}

// TODO:  get rid of me !!  as soon as vtkSMDisplayProxy can tell us when new
// arrays are added.
void pqDisplayProxyEditor::reloadGUI()
{
  this->Internal->ColorBy->setDisplay(this->Internal->Display);
}


QVariant pqDisplayProxyEditor::specularColor() const
{
  if(this->Internal->SpecularWhite->isChecked())
    {
    QList<QVariant> ret;
    ret.append(1.0);
    ret.append(1.0);
    ret.append(1.0);
    return ret;
    }
  
  vtkSMProxy* proxy = this->Internal->Display->getDisplayProxy();
  return pqSMAdaptor::getMultipleElementProperty(
       proxy->GetProperty("DiffuseColor"));
}

void pqDisplayProxyEditor::setSpecularColor(QVariant specColor)
{
  QList<QVariant> whiteLight;
  whiteLight.append(1.0);
  whiteLight.append(1.0);
  whiteLight.append(1.0);

  if(specColor == whiteLight && !this->Internal->SpecularWhite->isChecked())
    {
    this->Internal->SpecularWhite->setChecked(true);
    emit this->specularColorChanged();
    }
  else if(this->Internal->SpecularWhite->isChecked())
    {
    this->Internal->SpecularWhite->setChecked(false);
    emit this->specularColorChanged();
    }
}

void pqDisplayProxyEditor::updateMaterial(int idx)
{
  if(idx == 0)
    {
    this->Internal->Display->getDisplayProxy()->SetMaterialCM(0);
    this->updateView();
    }
  else if(idx == 1)
    {
    pqFileDialog diag(NULL, this, "Open Material File", QString(), 
                      "Material Files (*.xml)");
    diag.setFileMode(pqFileDialog::ExistingFile);
    if(diag.exec() == QDialog::Accepted)
      {
      QString filename = diag.getSelectedFiles()[0];
      QMap<QString, QString>::iterator iter;
      for(iter = this->Internal->MaterialMap.begin();
          iter != this->Internal->MaterialMap.end();
          ++iter)
        {
        if(filename == iter.value())
          {
          int foundidx = this->Internal->StyleMaterial->findText(iter.key());
          this->Internal->StyleMaterial->setCurrentIndex(foundidx);
          return;
          }
        }
      QFileInfo fi(filename);
      this->Internal->MaterialMap.insert(fi.fileName(), filename);
      this->Internal->StyleMaterial->addItem(fi.fileName());
      this->Internal->StyleMaterial->setCurrentIndex(
        this->Internal->StyleMaterial->count() - 1);
      }
    }
  else
    {
    QString label = this->Internal->StyleMaterial->itemText(idx);
    this->Internal->Display->getDisplayProxy()->SetMaterialCM(
      this->Internal->MaterialMap[label].toAscii().data());
    this->updateView();
    }
}

