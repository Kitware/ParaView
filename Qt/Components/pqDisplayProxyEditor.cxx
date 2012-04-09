/*=========================================================================

   Program: ParaView
   Module:    pqDisplayProxyEditor.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

// this include
#include "pqDisplayProxyEditor.h"
#include "ui_pqDisplayProxyEditor.h"

// Qt includes
#include <QDoubleValidator>
#include <QFileInfo>
#include <QIcon>
#include <QIntValidator>
#include <QKeyEvent>
#include <QMetaType>
#include <QPointer>
#include <QtDebug>

// ParaView Server Manager includes
#include "vtkEventQtSlotConnect.h"
#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkLabeledDataMapper.h"
#include "vtkMaterialLibrary.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMIntVectorProperty.h"
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
#include "pqCoreUtilities.h"
#include "pqCubeAxesEditorDialog.h"
#include "pqFileDialog.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSMAdaptor.h"
#include "pqStandardColorLinkAdaptor.h"
#include "pqTriggerOnIdleHelper.h"
#include "pqUndoStack.h"
#include "pqWidgetRangeDomain.h"
#include "pqSettings.h"

class pqDisplayProxyEditorInternal : public Ui::pqDisplayProxyEditor
{
public:
  pqDisplayProxyEditorInternal()
    {
    this->Links = new pqPropertyLinks;
    this->InterpolationAdaptor = 0;
    this->EdgeColorAdaptor = 0;
    this->AmbientColorAdaptor = 0;
    this->SliceDirectionAdaptor = 0;
    this->BackfaceRepresentationAdaptor = 0;
    this->SliceDomain = 0;
    this->SelectedMapperAdaptor = 0;
    this->SelectedResamplerAdaptor = 0;
    this->CompositeTreeAdaptor = 0;
    }

  ~pqDisplayProxyEditorInternal()
    {
    delete this->Links;
    delete this->InterpolationAdaptor;
    delete this->SliceDirectionAdaptor;
    delete this->BackfaceRepresentationAdaptor;
    delete this->SliceDomain;
    delete this->AmbientColorAdaptor;
    delete this->EdgeColorAdaptor;
    }

  pqPropertyLinks* Links;

  // The representation whose properties are being edited.
  QPointer<pqPipelineRepresentation> Representation;
  pqSignalAdaptorComboBox* InterpolationAdaptor;
  pqSignalAdaptorColor*    EdgeColorAdaptor;
  pqSignalAdaptorColor*    AmbientColorAdaptor;
  pqSignalAdaptorComboBox* SliceDirectionAdaptor;
  pqSignalAdaptorComboBox* SelectedMapperAdaptor;
  pqSignalAdaptorComboBox* SelectedResamplerAdaptor;
  pqSignalAdaptorComboBox* BackfaceRepresentationAdaptor;
  pqWidgetRangeDomain* SliceDomain;
  pqSignalAdaptorCompositeTreeWidget* CompositeTreeAdaptor;
  pqTriggerOnIdleHelper TriggerUpdateEnableState;

  // map of <material labels, material files>
  static QMap<QString, QString> MaterialMap;
 };

QMap<QString, QString> pqDisplayProxyEditorInternal::MaterialMap;

//-----------------------------------------------------------------------------
/// constructor
pqDisplayProxyEditor::pqDisplayProxyEditor(pqPipelineRepresentation* repr, QWidget* p)
  : pqDisplayPanel(repr, p), DisableSlots(0)
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  bool allowSpecularHighlightingWithScalarColoring = settings->value(
    "allowSpecularHighlightingWithScalarColoring").toBool();
  this->DisableSpecularOnScalarColoring = !allowSpecularHighlightingWithScalarColoring;

  this->Internal = new pqDisplayProxyEditorInternal;
  this->Internal->setupUi(this);

  QObject::connect(&this->Internal->TriggerUpdateEnableState,
    SIGNAL(triggered()), this, SLOT(updateEnableState()));

  this->setupGUIConnections();

  // setting a repr proxy will enable this
  this->setEnabled(false);

  this->setRepresentation(repr);

  QObject::connect(this->Internal->Links, SIGNAL(smPropertyChanged()),
    this, SLOT(updateAllViews()));
  QObject::connect(this->Internal->EditCubeAxes, SIGNAL(clicked(bool)),
    this, SLOT(editCubeAxes()));
  QObject::connect(this->Internal->compositeTree, SIGNAL(itemSelectionChanged()),
    this, SLOT(volumeBlockSelected()));
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

  delete this->Internal->SliceDomain;
  this->Internal->SliceDomain = 0;
  delete this->Internal->CompositeTreeAdaptor;
  this->Internal->CompositeTreeAdaptor = 0;

  vtkSMProxy* reprProxy = (repr)? repr->getProxy() : NULL;
  if(this->Internal->Representation)
    {
    // break all old links.
    this->Internal->Links->removeAllPropertyLinks();
    }

  this->Internal->Representation = repr;
  this->Internal->TriggerUpdateEnableState.setServer(
    repr? repr->getServer() : NULL);
  if (!repr )
    {
    this->setEnabled(false);
    return;
    }

  this->setEnabled(true);

  // The slots are already connected but we do not want them to execute
  // while we are initializing the GUI
  this->DisableSlots = 1;

  // setup for visibility
  this->Internal->Links->addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("Visibility"));

  this->Internal->Links->addPropertyLink(this->Internal->Selectable,
    "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("Pickable"));

  vtkSMProperty* prop = 0;

  // setup cube axes visibility.
  if ((prop = reprProxy->GetProperty("CubeAxesVisibility")) != 0)
    {
     QObject::connect(this->Internal->ShowCubeAxes, SIGNAL(toggled(bool)),
       this, SLOT(cubeAxesVisibilityChanged()));
     
     //needed so the undo / redo properly activate the checkbox
     this->Internal->Links->addPropertyLink(this->Internal->ShowCubeAxes,
    "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("CubeAxesVisibility"));
    this->Internal->AnnotationGroup->show();
    }
  else
    {
    this->Internal->AnnotationGroup->hide();
    }

  //if ((prop = reprProxy->GetProperty("PieceBoundsVisibility")) != 0) //DDM TODO
  //  {
  //  this->Internal->Links->addPropertyLink(this->Internal->ShowPieceBounds,
  //    "checked", SIGNAL(stateChanged(int)),
  //    reprProxy, prop);
  //  }

  // setup for choosing color
  if (reprProxy->GetProperty("DiffuseColor"))
    {
    QList<QVariant> curColor = pqSMAdaptor::getMultipleElementProperty(
      reprProxy->GetProperty("DiffuseColor"));

    bool prev = this->Internal->ColorActorColor->blockSignals(true);
    this->Internal->ColorActorColor->setChosenColor(
      QColor(qRound(curColor[0].toDouble()*255),
        qRound(curColor[1].toDouble()*255),
        qRound(curColor[2].toDouble()*255), 255));
    this->Internal->ColorActorColor->blockSignals(prev);

    // setup for specular lighting
    QObject::connect(this->Internal->SpecularWhite, SIGNAL(toggled(bool)),
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
    }

  // setup for interpolation
  this->Internal->StyleInterpolation->clear();
  if ((prop = reprProxy->GetProperty("Interpolation")) != 0)
    {
    prop->UpdateDependentDomains();
    QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(prop);
    foreach(QVariant item, items)
      {
      this->Internal->StyleInterpolation->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(this->Internal->InterpolationAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, prop);
    this->Internal->StyleInterpolation->setEnabled(true);
    }
  else
    {
    this->Internal->StyleInterpolation->setEnabled(false);
    }

  // setup for point size
  if ((prop = reprProxy->GetProperty("PointSize")) !=0)
    {
    this->Internal->Links->addPropertyLink(this->Internal->StylePointSize,
      "value", SIGNAL(editingFinished()),
      reprProxy, reprProxy->GetProperty("PointSize"));
    this->Internal->StylePointSize->setEnabled(true);
    }
  else
    {
    this->Internal->StylePointSize->setEnabled(false);
    }

  // setup for line width
  if ((prop = reprProxy->GetProperty("LineWidth")) != 0)
    {
    this->Internal->Links->addPropertyLink(this->Internal->StyleLineWidth,
      "value", SIGNAL(editingFinished()),
      reprProxy, reprProxy->GetProperty("LineWidth"));
    this->Internal->StyleLineWidth->setEnabled(true);
    }
  else
    {
    this->Internal->StyleLineWidth->setEnabled(false);
    }

  // setup for translate
  this->Internal->Links->addPropertyLink(this->Internal->TranslateX,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 0);
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Internal->TranslateX->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->TranslateY,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 1);
  validator = new QDoubleValidator(this);
  this->Internal->TranslateY->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->TranslateZ,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Position"), 2);
  validator = new QDoubleValidator(this);
  this->Internal->TranslateZ->setValidator(validator);


  // setup for scale
  this->Internal->Links->addPropertyLink(this->Internal->ScaleX,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 0);
  validator = new QDoubleValidator(this);
  this->Internal->ScaleX->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleY,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 1);
  validator = new QDoubleValidator(this);
  this->Internal->ScaleY->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleZ,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Scale"), 2);
  validator = new QDoubleValidator(this);
  this->Internal->ScaleZ->setValidator(validator);

  // setup for orientation
  this->Internal->Links->addPropertyLink(this->Internal->OrientationX,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 0);
  validator = new QDoubleValidator(this);
  this->Internal->OrientationX->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationY,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 1);
  validator = new QDoubleValidator(this);
  this->Internal->OrientationY->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationZ,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Orientation"), 2);
  validator = new QDoubleValidator(this);
  this->Internal->OrientationZ->setValidator(validator);

  // setup for origin
  this->Internal->Links->addPropertyLink(this->Internal->OriginX,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 0);
  validator = new QDoubleValidator(this);
  this->Internal->OriginX->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->OriginY,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 1);
  validator = new QDoubleValidator(this);
  this->Internal->OriginY->setValidator(validator);

  this->Internal->Links->addPropertyLink(this->Internal->OriginZ,
    "text", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Origin"), 2);
  validator = new QDoubleValidator(this);
  this->Internal->OriginZ->setValidator(validator);

  // setup for opacity
  this->Internal->Links->addPropertyLink(this->Internal->Opacity,
    "value", SIGNAL(editingFinished()),
    reprProxy, reprProxy->GetProperty("Opacity"));

  // setup of nonlinear subdivision
  if (reprProxy->GetProperty("NonlinearSubdivisionLevel"))
    {
    this->Internal->Links->addPropertyLink(
                this->Internal->NonlinearSubdivisionLevel,
                "value", SIGNAL(valueChanged(int)),
                reprProxy, reprProxy->GetProperty("NonlinearSubdivisionLevel"));
    this->Internal->NonlinearSubdivisionLevel->setEnabled(true);
    }
  else
    {
    this->Internal->NonlinearSubdivisionLevel->setEnabled(false);
    }

  // setup for map scalars
  this->Internal->Links->addPropertyLink(
    this->Internal->ColorMapScalars, "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("MapScalars"));

  // setup for InterpolateScalarsBeforeMapping
  if (reprProxy->GetProperty("InterpolateScalarsBeforeMapping"))
    {
    this->Internal->Links->addPropertyLink(
      this->Internal->ColorInterpolateScalars, "checked", SIGNAL(stateChanged(int)),
      reprProxy, reprProxy->GetProperty("InterpolateScalarsBeforeMapping"));
    }

  this->Internal->ColorBy->setRepresentation(repr);
  QObject::connect(this->Internal->ColorBy,
    SIGNAL(modified()),
    &this->Internal->TriggerUpdateEnableState, SLOT(trigger()));

  this->Internal->StyleRepresentation->setRepresentation(repr);
  QObject::connect(this->Internal->StyleRepresentation,
    SIGNAL(currentTextChanged(const QString&)),
    this->Internal->ColorBy, SLOT(reloadGUI()));

  QObject::connect(this->Internal->StyleRepresentation,
    SIGNAL(currentTextChanged(const QString&)),
    &this->Internal->TriggerUpdateEnableState, SLOT(trigger()));

  this->Internal->Texture->setRepresentation(repr);

  if ( (prop = reprProxy->GetProperty("EdgeColor")) != 0)
    {
    this->Internal->Links->addPropertyLink(this->Internal->EdgeColorAdaptor,
      "color", SIGNAL(colorChanged(const QVariant&)),
      reprProxy, prop);
    }

  if ( (prop = reprProxy->GetProperty("AmbientColor")) != 0)
    {
    this->Internal->Links->addPropertyLink(this->Internal->AmbientColorAdaptor,
      "color", SIGNAL(colorChanged(const QVariant&)),
      reprProxy, prop);
    }

  if (reprProxy->GetProperty("Slice"))
    {
    this->Internal->SliceDomain = new pqWidgetRangeDomain(
      this->Internal->Slice, "minimum", "maximum",
      reprProxy->GetProperty("Slice"), 0);

    this->Internal->Links->addPropertyLink(this->Internal->Slice,
      "value", SIGNAL(valueChanged(int)),
      reprProxy, reprProxy->GetProperty("Slice"));

    QList<QVariant> sliceModes =
      pqSMAdaptor::getEnumerationPropertyDomain(
        reprProxy->GetProperty("SliceMode"));
    foreach(QVariant item, sliceModes)
      {
      this->Internal->SliceDirection->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(
      this->Internal->SliceDirectionAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("SliceMode"));
    }

  if (reprProxy->GetProperty("ExtractedBlockIndex"))
    {
    this->Internal->CompositeTreeAdaptor =
      new pqSignalAdaptorCompositeTreeWidget(
        this->Internal->compositeTree,
        this->Internal->Representation->getOutputPortFromInput()->getOutputPortProxy(),
        vtkSMCompositeTreeDomain::NONE,
        pqSignalAdaptorCompositeTreeWidget::INDEX_MODE_FLAT, false, true);
    }

  if (reprProxy->GetProperty("ResamplingMethod"))
    {
    QList<QVariant> methodNames =
      pqSMAdaptor::getEnumerationPropertyDomain(
        reprProxy->GetProperty("ResamplingMethod"));
    foreach(QVariant item, methodNames)
      {
      this->Internal->SelectResamplerMethod->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(
      this->Internal->SelectedResamplerAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("ResamplingMethod"));
    }
  if (reprProxy->GetProperty("SelectMapper"))
    {
    QList<QVariant> mapperNames =
      pqSMAdaptor::getEnumerationPropertyDomain(
        reprProxy->GetProperty("SelectMapper"));
    foreach(QVariant item, mapperNames)
      {
      this->Internal->SelectMapper->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(
      this->Internal->SelectedMapperAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("SelectMapper"));
    }
  else  if (reprProxy->GetProperty("VolumeRenderingMode"))
    {
    QList<QVariant> mapperNames =
      pqSMAdaptor::getEnumerationPropertyDomain(
        reprProxy->GetProperty("VolumeRenderingMode"));
    foreach(QVariant item, mapperNames)
      {
      this->Internal->SelectMapper->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(
      this->Internal->SelectedMapperAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("VolumeRenderingMode"));
    }

  // setup for number of samples
  if (reprProxy->GetProperty("NumberOfSamples"))
    {
    this->Internal->Links->
      addPropertyLink(this->Internal->ISamples,
                      "text", SIGNAL(editingFinished()),
                      reprProxy, reprProxy->GetProperty("NumberOfSamples"), 0);
    QIntValidator* validator = new QIntValidator(this);
    validator->setBottom(10);
    this->Internal->ISamples->setValidator(validator);

    this->Internal->Links->
      addPropertyLink(this->Internal->JSamples,
                      "text", SIGNAL(editingFinished()),
                      reprProxy, reprProxy->GetProperty("NumberOfSamples"), 1);
    this->Internal->JSamples->setValidator(validator);
    
    this->Internal->Links->
      addPropertyLink(this->Internal->KSamples,
                      "text", SIGNAL(editingFinished()),
                      reprProxy, reprProxy->GetProperty("NumberOfSamples"), 2);
    this->Internal->KSamples->setValidator(validator);
    }

  this->Internal->BackfaceStyleRepresentation->clear();
  if ((prop = reprProxy->GetProperty("BackfaceRepresentation")) != NULL)
    {
    prop->UpdateDependentDomains();
    QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(prop);
    foreach (QVariant item, items)
      {
      this->Internal->BackfaceStyleRepresentation->addItem(item.toString());
      }
    this->Internal->Links->addPropertyLink(
                      this->Internal->BackfaceRepresentationAdaptor,
                      "currentText", SIGNAL(currentTextChanged(const QString&)),
                      reprProxy, prop);
    this->Internal->BackfaceStyleGroup->setEnabled(true);
    }
  else
    {
    this->Internal->BackfaceStyleGroup->setEnabled(false);
    }

  QObject::connect(this->Internal->BackfaceStyleRepresentation,
                   SIGNAL(currentIndexChanged(const QString&)),
                   &this->Internal->TriggerUpdateEnableState, SLOT(trigger()));

  // setup for choosing backface color
  if (reprProxy->GetProperty("BackfaceDiffuseColor"))
    {
    QList<QVariant> curColor = pqSMAdaptor::getMultipleElementProperty(
                                reprProxy->GetProperty("BackfaceDiffuseColor"));

    bool prev = this->Internal->BackfaceActorColor->blockSignals(true);
    this->Internal->BackfaceActorColor->setChosenColor(
                               QColor(qRound(curColor[0].toDouble()*255),
                                      qRound(curColor[1].toDouble()*255),
                                      qRound(curColor[2].toDouble()*255), 255));
    this->Internal->BackfaceActorColor->blockSignals(prev);

    new pqStandardColorLinkAdaptor(this->Internal->BackfaceActorColor,
                                   reprProxy, "BackfaceDiffuseColor");
    }

  // setup for backface opacity
  if (reprProxy->GetProperty("BackfaceOpacity"))
    {
    this->Internal->Links->addPropertyLink(this->Internal->BackfaceOpacity,
                                         "value", SIGNAL(editingFinished()),
                                         reprProxy,
                                         reprProxy->GetProperty("BackfaceOpacity"));
    }

  new pqStandardColorLinkAdaptor(this->Internal->ColorActorColor,
    reprProxy, "DiffuseColor");
  if (reprProxy->GetProperty("EdgeColor"))
    {
    new pqStandardColorLinkAdaptor(this->Internal->EdgeColor,
      reprProxy, "EdgeColor");
    }
  if (reprProxy->GetProperty("AmbientColor"))
    {
    new pqStandardColorLinkAdaptor(this->Internal->AmbientColor,
      reprProxy, "AmbientColor");
    }

  this->DisableSlots = 0;

  if (reprProxy->GetProperty("Shade"))
    {
    this->Internal->Links->addPropertyLink(this->Internal->Shading,
      "checked", SIGNAL(toggled(bool)),
      reprProxy, reprProxy->GetProperty("Shade"));
    this->Internal->Shading->setEnabled(true);
    }
  else
    {
    this->Internal->Shading->setEnabled(false);
    }

  if (reprProxy->GetProperty("FreezeFocalPoint"))
    {
    this->Internal->Links->addPropertyLink(this->Internal->FreezeFocalPoint,
      "checked", SIGNAL(toggled(bool)),
      reprProxy, reprProxy->GetProperty("FreezeFocalPoint"));
    this->Internal->Shading->setEnabled(true);
    }
  else
    {
    this->Internal->FreezeFocalPoint->setEnabled(false);
    }

  this->updateEnableState();

}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::setupGUIConnections()
{
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

  QObject::connect(this->Internal->ColorActorColor,
    SIGNAL(chosenColorChanged(const QColor&)),
    this, SLOT(setSolidColor(const QColor&)));

  /// Set up signal-slot connections to create a single undo-set for all the
  /// changes that happen when the solid color is changed.
  /// We need to do this for both solid and edge color since we want to make
  /// sure that the undo-element for setting up of the "global property" link
  /// gets added in the same set in which the solid/edge color is changed.
  this->Internal->ColorActorColor->setUndoLabel("Change Solid Color");
  QObject::connect(this->Internal->ColorActorColor,
    SIGNAL(beginUndo(const QString&)),
    this, SLOT(beginUndoSet(const QString&)));
  QObject::connect(this->Internal->ColorActorColor,
    SIGNAL(endUndo()), this, SLOT(endUndoSet()));

  this->Internal->EdgeColorAdaptor = new pqSignalAdaptorColor(
    this->Internal->EdgeColor, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Internal->EdgeColor->setUndoLabel("Change Edge Color");
  QObject::connect(this->Internal->EdgeColor,
    SIGNAL(beginUndo(const QString&)),
    this, SLOT(beginUndoSet(const QString&)));
  QObject::connect(this->Internal->EdgeColor,
    SIGNAL(endUndo()), this, SLOT(endUndoSet()));

  this->Internal->AmbientColorAdaptor = new pqSignalAdaptorColor(
    this->Internal->AmbientColor, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Internal->AmbientColor->setUndoLabel("Change Ambient Color");
  QObject::connect(this->Internal->AmbientColor,
    SIGNAL(beginUndo(const QString&)),
    this, SLOT(beginUndoSet(const QString&)));
  QObject::connect(this->Internal->AmbientColor,
    SIGNAL(endUndo()), this, SLOT(endUndoSet()));

  this->Internal->SliceDirectionAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->SliceDirection);
  QObject::connect(this->Internal->SliceDirectionAdaptor,
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(sliceDirectionChanged()));

  this->Internal->SelectedMapperAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->SelectMapper);

  this->Internal->SelectedResamplerAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->SelectResamplerMethod);

  this->Internal->BackfaceRepresentationAdaptor = new pqSignalAdaptorComboBox(
                                   this->Internal->BackfaceStyleRepresentation);
  this->Internal->BackfaceRepresentationAdaptor->setObjectName(
                                         "BackfaceStyleRepresentationAdapator");

  QObject::connect(this->Internal->BackfaceActorColor,
                   SIGNAL(chosenColorChanged(const QColor&)),
                   this, SLOT(setBackfaceSolidColor(const QColor&)));

  this->Internal->BackfaceActorColor->setUndoLabel("Change Backface Solid Color");
  QObject::connect(this->Internal->BackfaceActorColor,
    SIGNAL(beginUndo(const QString&)),
    this, SLOT(beginUndoSet(const QString&)));
  QObject::connect(this->Internal->BackfaceActorColor,
    SIGNAL(endUndo()), this, SLOT(endUndoSet()));
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::updateEnableState()
{
  if (!this->Internal->Representation ||
    this->Internal->Representation->getServer() == NULL)
    {
    return;
    }

  Q_ASSERT(
    this->Internal->Representation->getServer()->isProgressPending() == false);

  QString reprType = this->Internal->Representation->getRepresentationType();

  if (this->Internal->ColorBy->getCurrentText() == "Solid Color")
    {
    this->Internal->ColorInterpolateScalars->setEnabled(false);
    if (reprType == "Wireframe" ||
      reprType == "Points" ||
      reprType == "Outline")
      {
      this->Internal->ColorButtonStack->setCurrentWidget(
        this->Internal->AmbientColorPage);
      this->Internal->LightingGroup->setEnabled(false);
      }
    else
      {
      this->Internal->ColorButtonStack->setCurrentWidget(
        this->Internal->SolidColorPage);
      this->Internal->LightingGroup->setEnabled(true);
      }
    this->Internal->BackfaceActorColor->setEnabled(true);
    }
  else
    {
    if (this->DisableSpecularOnScalarColoring)
      {
      this->Internal->LightingGroup->setEnabled(false);
      }
    this->Internal->ColorInterpolateScalars->setEnabled(true);
    this->Internal->ColorButtonStack->setCurrentWidget(
        this->Internal->ColorMapPage);
    this->Internal->BackfaceActorColor->setEnabled(false);
    }


  this->Internal->EdgeStyleGroup->setEnabled(
    reprType == "Surface With Edges");

  this->Internal->SliceGroup->setEnabled(
    reprType == "Slice" );
  if (reprType == "Slice")
    {
    // every time the user switches to Slice mode we update the domain for the
    // slider since the domain depends on the input to the image mapper which
    // may have changed.
    this->sliceDirectionChanged();
    }

  this->Internal->compositeTree->setVisible(
   this->Internal->CompositeTreeAdaptor &&
   (reprType == "Volume"));

  this->Internal->SelectMapper->setEnabled(
    reprType == "Volume"
    && (this->Internal->Representation->getProxy()->GetProperty("SelectMapper") ||
        this->Internal->Representation->getProxy()->GetProperty("VolumeRenderingMode")));

  bool hasNumberOfSamples = 
    this->Internal->Representation->getProxy()->GetProperty("NumberOfSamples");
  this->Internal->ISamples->setEnabled(hasNumberOfSamples);
  this->Internal->JSamples->setEnabled(hasNumberOfSamples);
  this->Internal->KSamples->setEnabled(hasNumberOfSamples);
  
  vtkSMProperty *backfaceRepProperty = this->Internal->Representation
    ->getRepresentationProxy()->GetProperty("BackfaceRepresentation");
  if (   !backfaceRepProperty
      || (   (reprType != "Points")
          && (reprType != "Wireframe")
          && (reprType != "Surface")
          && (reprType != "Surface With Edges") ) )
    {
    this->Internal->BackfaceStyleGroup->setEnabled(false);
    }
  else
    {
    this->Internal->BackfaceStyleGroup->setEnabled(true);
    int backRepType
      = pqSMAdaptor::getElementProperty(backfaceRepProperty).toInt();

    bool backFollowsFront
      = (   (backRepType == vtkGeometryRepresentationWithFaces::FOLLOW_FRONTFACE)
         || (backRepType == vtkGeometryRepresentationWithFaces::CULL_BACKFACE)
         || (backRepType == vtkGeometryRepresentationWithFaces::CULL_FRONTFACE) );

    this->Internal->BackfaceStyleGroupOptions->setEnabled(!backFollowsFront);
    }

  vtkSMRepresentationProxy* display =
    this->Internal->Representation->getRepresentationProxy();
  if (display)
    {
    QVariant scalarMode = pqSMAdaptor::getEnumerationProperty(
      display->GetProperty("ColorAttributeType"));
    vtkPVDataInformation* geomInfo =
      display->GetRepresentedDataInformation();
    if (!geomInfo)
      {
      return;
      }
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
      // Upto 4 component unsigned chars can be direcly mapped.
      if (arrayInfo->GetNumberOfComponents() <= 4)
        {
        // One component causes more trouble than it is worth.
        this->Internal->ColorMapScalars->setEnabled(true);
        return;
        }
      }
    if ( arrayInfo )
      {
      this->Internal->ColorMapScalars->setCheckState(Qt::Checked);
      }
    }

  
  this->Internal->ColorMapScalars->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::openColorMapEditor()
{
  pqColorScaleEditor editor(pqCoreUtilities::mainWidget());
  editor.setObjectName("pqColorScaleDialog");
  editor.setRepresentation(this->Internal->Representation);
  editor.exec();
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

  pqRenderView* renModule = qobject_cast<pqRenderView*>(
    this->Internal->Representation->getView());
  if (renModule)
    {
    vtkSMRenderViewProxy* rm = renModule->getRenderViewProxy();
    rm->ZoomTo(this->Internal->Representation->getProxy());
    renModule->render();
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
void pqDisplayProxyEditor::cubeAxesVisibilityChanged()
{
  vtkSMProxy* reprProxy = (this->Internal->Representation)? this->Internal->Representation->getProxy() : NULL;
  vtkSMProperty* prop = 0;

  // setup cube axes visibility.
  if ((prop = reprProxy->GetProperty("CubeAxesVisibility")) != 0)
    {
    pqSMAdaptor::setElementProperty(prop, this->Internal->ShowCubeAxes->isChecked());
    reprProxy->UpdateVTKObjects();
    }
  this->updateAllViews();
}
//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::editCubeAxes()
{
  pqCubeAxesEditorDialog dialog(this);
  dialog.setRepresentationProxy(this->Internal->Representation->getProxy());
  if (dialog.exec() == QDialog::Accepted)
    {
    this->Internal->Representation->renderViewEventually();
    }
}
//----------------------------------------------------------------------------
bool pqDisplayProxyEditor::isCubeAxesVisible()
{
  return this->Internal->ShowCubeAxes->isChecked();
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::sliceDirectionChanged()
{
  if (this->Internal->Representation)
    {
    vtkSMProxy* reprProxy = this->Internal->Representation->getProxy();
    vtkSMProperty* prop = reprProxy->GetProperty("SliceMode");
    if (prop)
      {
      prop->UpdateDependentDomains();
      }
    }
}

//-----------------------------------------------------------------------------
/// Handle user selection of block for volume rendering
void pqDisplayProxyEditor::volumeBlockSelected()
{
  if (this->Internal->CompositeTreeAdaptor
      && this->Internal->Representation)
    {
    bool valid = false;
    unsigned int selectedIndex =
      this->Internal->CompositeTreeAdaptor->getCurrentFlatIndex(&valid);
    if (valid && selectedIndex > 0)
      {
      vtkSMRepresentationProxy* repr =
        this->Internal->Representation->getRepresentationProxy();
      pqSMAdaptor::setElementProperty(
        repr->GetProperty("ExtractedBlockIndex"), selectedIndex);
      repr->UpdateVTKObjects();
      this->Internal->Representation->renderViewEventually();
      this->Internal->ColorBy->reloadGUI();
      }
    }
}

//-----------------------------------------------------------------------------
// Called when the GUI selection for the solid color changes.
void pqDisplayProxyEditor::setSolidColor(const QColor& color)
{
  QList<QVariant> val;
  val.push_back(color.red()/255.0);
  val.push_back(color.green()/255.0);
  val.push_back(color.blue()/255.0);
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->Representation->getProxy()->GetProperty("DiffuseColor"), val);

  // If specular white is off, then we want to update the specular color as
  // well.
  emit this->specularColorChanged();
}

//-----------------------------------------------------------------------------
// Called when the GUI selection for the backface solid color changes.
void pqDisplayProxyEditor::setBackfaceSolidColor(const QColor& color)
{
  QList<QVariant> val;
  val.push_back(color.red()/255.0);
  val.push_back(color.green()/255.0);
  val.push_back(color.blue()/255.0);

  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->Representation->getProxy()->GetProperty("BackfaceAmbientColor"), val);
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->Representation->getProxy()->GetProperty("BackfaceDiffuseColor"), val);

  // If specular white is off, then we want to update the specular color as
  // well.
  emit this->specularColorChanged();
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::beginUndoSet(const QString& str)
{
  BEGIN_UNDO_SET(str.toAscii().data());
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::endUndoSet()
{
  END_UNDO_SET();
}
