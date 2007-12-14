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
#include <QFileInfo>
#include <QIcon>
#include <QMetaType>
#include <QPointer>
#include <QtDebug>
#include <QTimer>

// ParaView Server Manager includes
#include "vtkEventQtSlotConnect.h"
#include "vtkMaterialLibrary.h"
#include "vtkLabeledDataMapper.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"

// ParaView widget includes
#include "pqSignalAdaptors.h"

// ParaView client includes
#include "pqApplicationCore.h"
#include "pqColorScaleEditor.h"
#include "pqCubeAxesEditorDialog.h"
#include "pqFileDialog.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqSMAdaptor.h"

class pqDisplayProxyEditorInternal : public Ui::pqDisplayProxyEditor
{
public:
  pqDisplayProxyEditorInternal()
    {
    this->Links = new pqPropertyLinks;
    this->InterpolationAdaptor = 0;
    this->ColorAdaptor = 0;
    this->EdgeColorAdaptor = 0;
    }

  ~pqDisplayProxyEditorInternal()
    {
    delete this->Links;
    delete this->InterpolationAdaptor;
    }

  pqPropertyLinks* Links;

  // The representation whose properties are being edited.
  QPointer<pqPipelineRepresentation> Representation;
  pqSignalAdaptorComboBox* InterpolationAdaptor;
  pqSignalAdaptorColor*    ColorAdaptor;
  pqSignalAdaptorColor*    EdgeColorAdaptor;

  // map of <material labels, material files>
  static QMap<QString, QString> MaterialMap;
 };

QMap<QString, QString> pqDisplayProxyEditorInternal::MaterialMap;

//-----------------------------------------------------------------------------
/// constructor
pqDisplayProxyEditor::pqDisplayProxyEditor(pqPipelineRepresentation* repr, QWidget* p)
  : pqDisplayPanel(repr, p), DisableSlots(0)
{
  this->Internal = new pqDisplayProxyEditorInternal;
  this->Internal->setupUi(this);
  this->setupGUIConnections();

  // setting a repr proxy will enable this
  this->setEnabled(false);

  this->setRepresentation(repr);

  QObject::connect(this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()));
  QObject::connect(this->Internal->EditCubeAxes, SIGNAL(clicked(bool)),
    this, SLOT(editCubeAxes()));
}

//-----------------------------------------------------------------------------
/// destructor
pqDisplayProxyEditor::~pqDisplayProxyEditor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
/// set the proxy to repr display properties for
void pqDisplayProxyEditor::setRepresentation(pqPipelineRepresentation* repr) 
{
  if(this->Internal->Representation == repr)
    {
    return;
    }

  vtkSMProxy* reprProxy = (repr)? repr->getProxy() : NULL;
  if(this->Internal->Representation)
    {
    // break all old links.
    this->Internal->Links->removeAllPropertyLinks();
    }

  this->Internal->Representation = repr;
  if (!repr )
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
    reprProxy, reprProxy->GetProperty("Visibility"));

  // setup cube axes visibility.
  this->Internal->Links->addPropertyLink(this->Internal->ShowCubeAxes,
    "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("CubeAxesVisibility"));

  // setup for choosing color
  this->Internal->Links->addPropertyLink(this->Internal->ColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("AmbientColor"));
  this->Internal->Links->addPropertyLink(this->Internal->ColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("DiffuseColor"));

  // setup for specular lighting
  QObject::connect(this->Internal->SpecularWhite, SIGNAL(toggled(bool)),
                   this, SIGNAL(specularColorChanged()));
  QObject::connect(this->Internal->ColorAdaptor,
                   SIGNAL(colorChanged(const QVariant&)),
                   this, SIGNAL(specularColorChanged()));
  this->Internal->Links->addPropertyLink(this->Internal->SpecularIntensity,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Specular"));
  this->Internal->Links->addPropertyLink(this,
    "specularColor", SIGNAL(specularColorChanged()),
    reprProxy, reprProxy->GetProperty("SpecularColor"));
  this->Internal->Links->addPropertyLink(this->Internal->SpecularPower,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("SpecularPower"));
  QObject::connect(this->Internal->SpecularIntensity, SIGNAL(editingFinished()),
                   this, SLOT(updateAllViews()));
  QObject::connect(this, SIGNAL(specularColorChanged()),
                   this, SLOT(updateAllViews()));
  QObject::connect(this->Internal->SpecularPower, SIGNAL(editingFinished()),
                   this, SLOT(updateAllViews()));
  
  // setup for interpolation
  this->Internal->StyleInterpolation->clear();
  vtkSMProperty* Property = reprProxy->GetProperty("Interpolation");
  Property->UpdateDependentDomains();
  QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(
    Property);
  foreach(QVariant item, items)
    {
    this->Internal->StyleInterpolation->addItem(item.toString());
    }
  this->Internal->Links->addPropertyLink(this->Internal->InterpolationAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("Interpolation"));

  // setup for point size
  this->Internal->Links->addPropertyLink(this->Internal->StylePointSize,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("PointSize"));

  // setup for line width
  this->Internal->Links->addPropertyLink(this->Internal->StyleLineWidth,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("LineWidth"));

  // setup for translate
  this->Internal->Links->addPropertyLink(this->Internal->TranslateX,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->TranslateY,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 1);
  
  this->Internal->Links->addPropertyLink(this->Internal->TranslateZ,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 2);

  // setup for scale
  this->Internal->Links->addPropertyLink(this->Internal->ScaleX,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleY,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleZ,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 2);

  // setup for orientation
  this->Internal->Links->addPropertyLink(this->Internal->OrientationX,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationY,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationZ,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 2);

  // setup for origin
  this->Internal->Links->addPropertyLink(this->Internal->OriginX,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->OriginY,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->OriginZ,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 2);

  // setup for opacity
  this->Internal->Links->addPropertyLink(this->Internal->Opacity,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Opacity"));

  // setup for map scalars
  this->Internal->Links->addPropertyLink(
    this->Internal->ColorMapScalars, "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("MapScalars"));

  // setup for InterpolateScalarsBeforeMapping
  this->Internal->Links->addPropertyLink(
    this->Internal->ColorInterpolateColors, "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("InterpolateScalarsBeforeMapping"));

  this->Internal->ColorBy->setRepresentation(repr);
  QObject::connect(this->Internal->ColorBy,
    SIGNAL(modified()),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);

  this->Internal->StyleRepresentation->setRepresentation(repr);
  QObject::connect(this->Internal->StyleRepresentation,
    SIGNAL(currentTextChanged(const QString&)),
    this->Internal->ColorBy, SLOT(reloadGUI()));

  QObject::connect(this->Internal->StyleRepresentation,
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);

  this->Internal->Texture->setRepresentation(repr);

  /*
  if (reprProxy->GetProperty("ScalarOpacityUnitDistance"))
    {
    this->Internal->Links->addPropertyLink(
      this->Internal->ScalarOpacityUnitDistance, "value",
      SIGNAL(editingFinished()),
      reprProxy, reprProxy->GetProperty("ScalarOpacityUnitDistance"));
    }
    */

  this->Internal->Links->addPropertyLink(this->Internal->EdgeColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("EdgeColor"));

#if 0                                       //FIXME 
  // material
  this->Internal->StyleMaterial->blockSignals(true);
  this->Internal->StyleMaterial->clear();
  if(vtkMaterialLibrary::GetNumberOfMaterials() > 0)
    {
    this->Internal->StyleMaterial->addItem("None");
    this->Internal->StyleMaterial->addItem("Browse...");
    this->Internal->StyleMaterial->addItems(this->Internal->MaterialMap.keys());
    const char* mat = this->Internal->Representation->getDisplayProxy()->GetMaterialCM();
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
#endif

  this->DisableSlots = 0;
  
  QTimer::singleShot(0, this, SLOT(updateEnableState()));
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::setupGUIConnections()
{
  // We are usinging Queues slot execution where ever possible,
  // This ensures that the updateAllViews() slot is called 
  // only after the vtkSMProperty has been changed by the pqPropertyLinks.
  QObject::connect(
    this->Internal->ViewData, SIGNAL(stateChanged(int)),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->ColorInterpolateColors, SIGNAL(stateChanged(int)),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->ColorMapScalars, SIGNAL(stateChanged(int)),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->StylePointSize,  SIGNAL(editingFinished()), 
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->StyleLineWidth, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->TranslateX, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->TranslateY, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->TranslateZ, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->ScaleX, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->ScaleY, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->ScaleZ, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->OrientationX, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->OrientationY, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->OrientationZ, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->OriginX, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->OriginY, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->OriginZ, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->Opacity, SIGNAL(editingFinished()),
    this, SLOT(updateAllViews()));
  QObject::connect(
    this->Internal->ViewZoomToData, SIGNAL(clicked(bool)), 
    this, SLOT(zoomToData()));
  QObject::connect(
    this->Internal->EditColorMapButton, SIGNAL(clicked()),
    this, SLOT(openColorMapEditor()));
  QObject::connect(
    this->Internal->RescaleButton, SIGNAL(clicked()),
    this, SLOT(rescaleToDataRange()));

  // Create an connect signal adapters.
  if (!QMetaType::isRegistered(QMetaType::type("QVariant")))
    {
    qRegisterMetaType<QVariant>("QVariant");
    }

  this->Internal->InterpolationAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->StyleInterpolation);
  this->Internal->InterpolationAdaptor->setObjectName(
    "StyleInterpolationAdapator");
  QObject::connect(this->Internal->InterpolationAdaptor, 
    SIGNAL(currentTextChanged(const QString&)), this, SLOT(updateAllViews()));
    
  this->Internal->ColorAdaptor = new pqSignalAdaptorColor(
                            this->Internal->ColorActorColor,
                            "chosenColor",
                            SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Internal->EdgeColorAdaptor = new pqSignalAdaptorColor(
    this->Internal->EdgeColor, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);

  QObject::connect(
    this->Internal->ColorActorColor, SIGNAL(chosenColorChanged(const QColor&)),
    this, SLOT(updateAllViews()));
  
  QObject::connect(this->Internal->StyleMaterial, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(updateMaterial(int)));

}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::updateEnableState()
{
  if (this->Internal->ColorBy->getCurrentText() == "Solid Color")
    {
    this->Internal->ColorInterpolateColors->setEnabled(false);
    this->Internal->ColorButtonStack->setCurrentWidget(
        this->Internal->SolidColorPage);
    this->Internal->LightingGroup->setEnabled(true);
    }
  else
    {
    this->Internal->LightingGroup->setEnabled(false);
    this->Internal->ColorInterpolateColors->setEnabled(true);
    this->Internal->ColorButtonStack->setCurrentWidget(
        this->Internal->ColorMapPage);
    }

  int reprType = this->Internal->Representation->getRepresentationType();
  
  //this->Internal->ScalarOpacityUnitDistance->setEnabled(
  //  reprType == vtkSMPVRepresentationProxy::VOLUME);
  this->Internal->EdgeStyleGroup->setEnabled(
    reprType == vtkSMPVRepresentationProxy::SURFACE_WITH_EDGES);

  vtkSMDataRepresentationProxy* display = 
    this->Internal->Representation->getRepresentationProxy();
  if (display)
    {
    QVariant scalarMode = pqSMAdaptor::getEnumerationProperty(
      display->GetProperty("ColorAttributeType"));
    vtkPVDataInformation* geomInfo = 
      display->GetRepresentedDataInformation(/*update=*/false);
    vtkPVDataSetAttributesInformation* attrInfo;
    if (scalarMode == "POINT_DATA")
      {
      attrInfo = geomInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = geomInfo->GetCellDataInformation();
      }
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(
      this->Internal->Representation->getColorField(true).toAscii().data());

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
  if(this->Internal->Representation.isNull())
    {
    return;
    }

  // Create a color map editor and set the display.
  pqColorScaleEditor colorScale(this);
  colorScale.setRepresentation(this->Internal->Representation);
  colorScale.exec();
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::rescaleToDataRange()
{
  if(this->Internal->Representation.isNull())
    {
    return;
    }

  this->Internal->Representation->resetLookupTableScalarRange();
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::zoomToData()
{
  if (this->DisableSlots)
    {
    return;
    }

  double bounds[6];
  this->Internal->Representation->getDataBounds(bounds);
  if (bounds[0]<=bounds[1] && bounds[2]<=bounds[3] && bounds[4]<=bounds[5])
    {
    pqRenderView* renModule = qobject_cast<pqRenderView*>(
      this->Internal->Representation->getView());
    if (renModule)
      {
      vtkSMRenderViewProxy* rm = renModule->getRenderViewProxy();
      rm->ResetCamera(bounds);
      renModule->render();
      }
    }
}

//-----------------------------------------------------------------------------
// TODO:  get rid of me !!  as soon as vtkSMDisplayProxy can tell us when new
// arrays are added.
void pqDisplayProxyEditor::reloadGUI()
{
  this->Internal->ColorBy->setRepresentation(this->Internal->Representation);
}


//-----------------------------------------------------------------------------
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
  
  vtkSMProxy* proxy = this->Internal->Representation->getProxy();
  return pqSMAdaptor::getMultipleElementProperty(
       proxy->GetProperty("DiffuseColor"));
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::updateMaterial(int vtkNotUsed(idx))
{
  // FIXME: when we enable materials.
#if 0   
  if(idx == 0)
    {
    this->Internal->Representation->getDisplayProxy()->SetMaterialCM(0);
    this->updateAllViews();
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
    this->Internal->Representation->getDisplayProxy()->SetMaterialCM(
      this->Internal->MaterialMap[label].toAscii().data());
    this->updateAllViews();
    }
#endif
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::editCubeAxes()
{
  pqCubeAxesEditorDialog dialog(this);
  dialog.setRepresentationProxy(this->Internal->Representation->getProxy());
  dialog.exec();
}
