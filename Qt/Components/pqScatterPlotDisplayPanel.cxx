/*=========================================================================

   Program: ParaView
   Module:    pqScatterPlotDisplayPanel.cxx

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
#include "pqScatterPlotDisplayPanel.h"
#include "ui_pqScatterPlotDisplayPanel.h"

#include "pqApplicationCore.h"
#include "pqColorScaleToolbar.h"
#include "pqComboBoxDomain.h"
#include "pqComboBoxDomain.h"
#include "pqCubeAxesEditorDialog.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqScatterPlotRepresentation.h"
#include "pqScatterPlotView.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqStandardColorButton.h"
#include "pqUndoStack.h"
#include "pqView.h"

#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMScatterPlotRepresentationProxy.h"
#include "vtkSMScatterPlotViewProxy.h"
#include "vtkScatterPlotMapper.h"
#include "vtkWeakPointer.h"

#include <QColorDialog>
#include <QDebug>
#include <QHeaderView>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QList>
#include <QPixmap>
#include <QPointer>
#include <QTimer>

#include <assert.h>

// Protect the qt class in an anonymous namespace
namespace {
  //-----------------------------------------------------------------------------
  class pqComboBoxDecoratedDomain : public pqComboBoxDomain
  {
  typedef pqComboBoxDomain Superclass;
public:
    pqComboBoxDecoratedDomain(QComboBox* comboBox, vtkSMProperty* prop, 
                              const QString& domainName = QString())
    : Superclass(comboBox, prop, domainName) 
    {
    this->CellDataIcon = new QIcon(":/pqWidgets/Icons/pqCellData16.png");
    this->PointDataIcon = new QIcon(":/pqWidgets/Icons/pqPointData16.png");
    this->domainChanged();
    }
  virtual ~pqComboBoxDecoratedDomain() 
    {
    delete this->CellDataIcon;
    delete this->PointDataIcon;
    }
  QString convertDataToText(const QString& data)const
    {
    int textPos = data.indexOf(',') + 1;
    if(textPos == 0)
      {
      return data;
      }
    else
      {
      //return data.mid(textPos, data.indexOf(',',textPos));
      QString text = data.right(data.length() - textPos);
      int lastIndexOf = text.lastIndexOf("-1");
      if( lastIndexOf >= 0)
        {
        text.replace(lastIndexOf, 2, "Magnitude");
        }
      return text;
      }
    }
  QIcon* convertDataToIcon(const QString& data)const 
    {
    int textPos = data.indexOf(',');
    if(textPos < 0)
      {
      return NULL;
      }
    if(data.left( textPos ) == "point")
      {
      return this->PointDataIcon;
      }
    else if(data.left( textPos ) == "cell")
      {
      return this->CellDataIcon;      
      }
    return NULL;
    }
protected slots:
  virtual void internalDomainChanged()
    {
    QComboBox* combo = qobject_cast<QComboBox*>(this->parent());
    Q_ASSERT(combo != NULL);
    if(!combo)
      {
      return;
      }

    QList<QString> texts;
    QList<QVariant> data;
    QList<QIcon*> icons;

    pqSMAdaptor::PropertyType type;

    type = pqSMAdaptor::getPropertyType(this->getProperty());
    if(!(type == pqSMAdaptor::ENUMERATION && 
         QString(this->getDomain()->GetXMLName()) == "array_list"))
      {
      this->pqComboBoxDomain::internalDomainChanged();
      return;
      }
    QList<QVariant> enums;
    enums = pqSMAdaptor::getEnumerationPropertyDomain(this->getProperty());
    foreach(QVariant var, enums)
      {
      texts.append(this->convertDataToText(var.toString()));
      data.append(var.toString());
      icons.append(this->convertDataToIcon(var.toString()));
      }
 
    foreach (QString userStr, this->getUserStrings())
      {
      if (!data.contains(userStr))
        {
        texts.push_front(this->convertDataToText(userStr));
        data.push_front(userStr);
        icons.push_front(this->convertDataToIcon(userStr));
        }
      }

    // texts and data must be of the same size.
    assert(texts.size() == data.size() && data.size() == icons.size());

    // check if the texts didn't change
    QList<QVariant> oldData;
    QList<QString>  oldTexts;

    for(int i = 0; i < combo->count(); i++)
      {
      oldTexts.append(combo->itemText(i));
      oldData.append(combo->itemData(i));
      }

    if (oldData != data || oldTexts != texts)
      {
      // save previous value to put back
      QVariant old = combo->itemData(combo->currentIndex());
      bool prev = combo->blockSignals(true);
      combo->clear();
      for (int cc=0; cc < data.size(); cc++)
        {
        if(icons[cc])
          {
          combo->addItem(*icons[cc],texts[cc], data[cc]);
          }
        else
          {
          combo->addItem(texts[cc], data[cc]);
          }
        }
      combo->setCurrentIndex(-1);
      combo->blockSignals(prev);
      int foundOld = combo->findData(old);
      if (foundOld >= 0)
        {
        combo->setCurrentIndex(foundOld);
        }
      else
        {
        combo->setCurrentIndex(0);
        }
      }
    this->markForUpdate(false);
    }
  protected:
    QIcon* CellDataIcon;
    QIcon* PointDataIcon;
    QIcon* SolidColorIcon;
  };
}

//-----------------------------------------------------------------------------
class pqScatterPlotDisplayPanel::pqInternal : public Ui::pqScatterPlotDisplayPanel
{
public:
  pqInternal()
    {
    this->XAxisArrayDomain = 0;
    this->XAxisArrayAdaptor = 0;
    this->YAxisArrayDomain = 0;
    this->YAxisArrayAdaptor = 0;
    this->ZAxisArrayDomain = 0;
    this->ZAxisArrayAdaptor = 0;
    this->ColorArrayDomain = 0;
    this->ColorArrayAdaptor = 0;
    this->GlyphScalingArrayDomain = 0;
    this->GlyphScalingArrayAdaptor = 0;
    this->GlyphMultiSourceArrayDomain = 0;
    this->GlyphMultiSourceArrayAdaptor = 0;
    this->GlyphOrientationArrayDomain = 0;
    this->GlyphOrientationArrayAdaptor = 0;
    this->CompositeTreeAdaptor = 0;
    }

  ~pqInternal()
    {
    delete this->XAxisArrayDomain;
    delete this->XAxisArrayAdaptor;
    delete this->YAxisArrayDomain;
    delete this->YAxisArrayAdaptor;
    delete this->ZAxisArrayDomain;
    delete this->ZAxisArrayAdaptor;
    delete this->ColorArrayDomain;
    delete this->ColorArrayAdaptor;
    delete this->GlyphScalingArrayDomain;
    delete this->GlyphScalingArrayAdaptor;
    delete this->GlyphMultiSourceArrayDomain;
    delete this->GlyphMultiSourceArrayAdaptor;
    delete this->GlyphOrientationArrayDomain;
    delete this->GlyphOrientationArrayAdaptor;
    delete this->CompositeTreeAdaptor;
    }

  pqPropertyLinks Links;

  pqSignalAdaptorComboBox* XAxisArrayAdaptor;
  pqSignalAdaptorComboBox* YAxisArrayAdaptor;
  pqSignalAdaptorComboBox* ZAxisArrayAdaptor;
  pqSignalAdaptorComboBox* ColorArrayAdaptor;
  pqSignalAdaptorComboBox* GlyphScalingArrayAdaptor;
  pqSignalAdaptorComboBox* GlyphMultiSourceArrayAdaptor;
  pqSignalAdaptorComboBox* GlyphOrientationArrayAdaptor;

  pqComboBoxDecoratedDomain* XAxisArrayDomain;
  pqComboBoxDecoratedDomain* YAxisArrayDomain;
  pqComboBoxDecoratedDomain* ZAxisArrayDomain;
  pqComboBoxDecoratedDomain* ColorArrayDomain;
  pqComboBoxDecoratedDomain* GlyphScalingArrayDomain;
  pqComboBoxDecoratedDomain* GlyphMultiSourceArrayDomain;
  pqComboBoxDecoratedDomain* GlyphOrientationArrayDomain;

  pqSignalAdaptorCompositeTreeWidget* CompositeTreeAdaptor;
  vtkWeakPointer<vtkSMScatterPlotRepresentationProxy> ScatterPlotRepresentation;
  QPointer<pqScatterPlotRepresentation> Representation;

};

//-----------------------------------------------------------------------------
pqScatterPlotDisplayPanel::pqScatterPlotDisplayPanel(
  pqRepresentation* display,QWidget* p)
  : pqDisplayPanel(display, p),DisableSlots(0)
{
  this->Internal = new pqScatterPlotDisplayPanel::pqInternal();
  this->Internal->setupUi(this);
  this->setupGUIConnections();
  this->setEnabled(false);

  QObject::connect(&this->Internal->Links, SIGNAL(smPropertyChanged()),
    this, SLOT(updateAllViews()));
  QObject::connect(this->Internal->EditCubeAxes, SIGNAL(clicked(bool)),
    this, SLOT(openCubeAxesEditor()));

  this->Internal->XAxisArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->XCoordsComboBox);
  this->Internal->YAxisArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->YCoordsComboBox);
  this->Internal->ZAxisArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->ZCoordsComboBox);
  this->Internal->ColorArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->ColorComboBox);
  this->Internal->GlyphScalingArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->GlyphScalingComboBox);
  this->Internal->GlyphMultiSourceArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->GlyphMultiSourceComboBox);
  this->Internal->GlyphOrientationArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->GlyphOrientationComboBox);

  this->setDisplay(display);  
}

//-----------------------------------------------------------------------------
pqScatterPlotDisplayPanel::~pqScatterPlotDisplayPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::setupGUIConnections()
{
  QObject::connect(
    this->Internal->ViewZoomToData, SIGNAL(clicked(bool)), 
    this, SLOT(zoomToData()));
  QObject::connect(
    this->Internal->EditColorMapButton, SIGNAL(clicked()),
    this, SLOT(openColorMapEditor()));
  QObject::connect(
    this->Internal->RescaleButton, SIGNAL(clicked()),
    this, SLOT(rescaleColorToDataRange()));

  // Create an connect signal adapters.
  if (!QMetaType::isRegistered(QMetaType::type("QVariant")))
    {
    qRegisterMetaType<QVariant>("QVariant");
    }
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::setDisplay(pqRepresentation* disp)
{
  this->setEnabled(false);

  // This method can only be called once in the constructor, so I am removing
  // all "cleanup" code, since it's not applicable.
  assert(this->Internal->ScatterPlotRepresentation == 0);

  vtkSMScatterPlotRepresentationProxy* proxy =
    vtkSMScatterPlotRepresentationProxy::SafeDownCast(disp->getProxy());
  this->Internal->ScatterPlotRepresentation = proxy;
  if (!this->Internal->ScatterPlotRepresentation)
    {
    qWarning() << "pqScatterPlotDisplayPanel given a representation proxy "
                  "that is not a ScatterPlotRepresentation.  Cannot edit.";
    return;
    }

  // this is essential to ensure that when you undo-redo, the representation is
  // indeed update-to-date, thus ensuring correct domains etc.
  proxy->Update();

  // Give the representation to our series editor model
  //this->Internal->Model->setRepresentation(
  //  qobject_cast<pqDataRepresentation*>(disp));
  this->Internal->Representation = qobject_cast<pqScatterPlotRepresentation*>(disp);

  this->setEnabled(true);

  QObject::connect(this->Internal->ShowCubeAxes, SIGNAL(toggled(bool)),
                   this, SLOT(cubeAxesVisibilityChanged()));
  
  
  QObject::connect(this->Internal->ZCoordsCheckBox,
                   SIGNAL(stateChanged(int)),
                   this, SLOT(update3DMode()), Qt::QueuedConnection);
  QObject::connect(this->Internal->ColorComboBox, 
                   SIGNAL(currentIndexChanged(int)),
                   this, SLOT(onColorChanged()), Qt::QueuedConnection);
  QObject::connect(this->Internal->GlyphGroupBox,
                   SIGNAL(toggled(bool)),
                   this, SLOT(updateGlyphMode()), Qt::QueuedConnection);
  QObject::connect(this->Internal->GlyphScalingCheckBox,
                   SIGNAL(stateChanged(int)),
                   this, SLOT(updateGlyphMode()), Qt::QueuedConnection);
  QObject::connect(this->Internal->GlyphMultiSourceCheckBox,
                   SIGNAL(stateChanged(int)),
                   this, SLOT(updateGlyphMode()), Qt::QueuedConnection);
  QObject::connect(this->Internal->GlyphOrientationCheckBox,
                   SIGNAL(stateChanged(int)),
                   this, SLOT(updateGlyphMode()), Qt::QueuedConnection);


  // The slots are already connected but we do not want them to execute
  // while we are initializing the GUI
  this->DisableSlots = 1;

  // Connect ViewData checkbox to the proxy's Visibility property
  this->Internal->Links.addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));

  this->Internal->Links.addPropertyLink(this->Internal->Selectable,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Pickable"));
  this->Internal->Selectable->hide();

  vtkSMProperty* prop = 0;
  // setup cube axes visibility.
  if ((prop = proxy->GetProperty("CubeAxesVisibility")) != 0)
    {
    this->Internal->ShowCubeAxes->setChecked( 
      pqSMAdaptor::getElementProperty(prop).toInt());
    this->Internal->AnnotationGroup->show();
    }
  else
    {
    this->Internal->AnnotationGroup->hide();
    }

  // setup for point size
  if ((prop = proxy->GetProperty("PointSize")) !=0)
    {
    this->Internal->Links.addPropertyLink(this->Internal->StylePointSize,
      "value", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("PointSize"));
    this->Internal->StylePointSize->setEnabled(true);
    }
  else
    {
    this->Internal->StylePointSize->setEnabled(false);
    }
  // setup for line width
  if ((prop = proxy->GetProperty("LineWidth")) != 0)
    {
    this->Internal->Links.addPropertyLink(this->Internal->StyleLineWidth,
      "value", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("LineWidth"));
    this->Internal->StyleLineWidth->setEnabled(true);
    }
  else
    {
    this->Internal->StyleLineWidth->setEnabled(false);
    }
  // setup for opacity  
  if ((prop = proxy->GetProperty("Opacity")) != 0)
    {
    this->Internal->Links.addPropertyLink(this->Internal->Opacity,
      "value", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("Opacity"));
    this->Internal->Opacity->setEnabled(true);
    }
  else
    {
    this->Internal->Opacity->setEnabled(false);
    }

  // setup for map scalars

  this->Internal->Links.addPropertyLink(
    this->Internal->ColorMapScalars, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("MapScalars"));

  // setup for InterpolateScalarsBeforeMapping
  if (proxy->GetProperty("InterpolateScalarsBeforeMapping"))
    {
    this->Internal->Links.addPropertyLink(
      this->Internal->ColorInterpolateScalars, "checked", SIGNAL(stateChanged(int)),
      proxy, proxy->GetProperty("InterpolateScalarsBeforeMapping"));
    }

  if (proxy->GetProperty("ExtractedBlockIndex"))
    {
    this->Internal->CompositeTreeAdaptor = 
      new pqSignalAdaptorCompositeTreeWidget(
        this->Internal->compositeTree,
        this->Internal->Representation->getOutputPortFromInput()->getOutputPortProxy(),
        vtkSMCompositeTreeDomain::NONE,
        pqSignalAdaptorCompositeTreeWidget::INDEX_MODE_FLAT, false, true);
    this->Internal->compositeTree->hide();
    }
  else
    {
    this->Internal->compositeTree->hide();
    }
  
  // Connect to the new properties.pqComboBoxDomain will ensure that
  // when ever the domain changes the widget is updated as well.
  this->Internal->XAxisArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->XCoordsComboBox, proxy->GetProperty("XArrayName"));
  //this->Internal->XAxisArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->XAxisArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("XArrayName"));
  
  this->Internal->YAxisArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->YCoordsComboBox, proxy->GetProperty("YArrayName"));
  //this->Internal->YAxisArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->YAxisArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("YArrayName"));

  this->Internal->ZAxisArrayDomain = new pqComboBoxDecoratedDomain(
     this->Internal->ZCoordsComboBox, proxy->GetProperty("ZArrayName"));
  //this->Internal->ZAxisArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->ZAxisArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("ZArrayName"));

  this->Internal->ColorArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->ColorComboBox, proxy->GetProperty("ColorArrayName"));
  //this->Internal->ColorArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->ColorArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("ColorArrayName"));

  this->Internal->GlyphScalingArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->GlyphScalingComboBox, proxy->GetProperty("GlyphScalingArrayName"));
  //this->Internal->GlyphScalingArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->GlyphScalingArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("GlyphScalingArrayName"));

  this->Internal->GlyphMultiSourceArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->GlyphMultiSourceComboBox, proxy->GetProperty("GlyphMultiSourceArrayName"));
  //this->Internal->GlyphMultiSourceArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->GlyphMultiSourceArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("GlyphMultiSourceArrayName"));

  this->Internal->GlyphOrientationArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->GlyphOrientationComboBox, proxy->GetProperty("GlyphOrientationArrayName"));
  //this->Internal->GlyphOrientationArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->GlyphOrientationArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("GlyphOrientationArrayName"));
  
  // setup for ThreeDMode
  this->Internal->Links.addPropertyLink(
    this->Internal->ZCoordsCheckBox, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("ThreeDMode"));
  // setup for Colorize
  this->Internal->Links.addPropertyLink(
    this->Internal->ColorCheckBox, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Colorize"));
  
  vtkSMProperty* colorizeProperty = proxy->GetProperty("Colorize");
  this->Internal->ColorCheckBox->setChecked(
    pqSMAdaptor::getElementProperty(colorizeProperty).toInt());

  // setup for scale factor
  if ((prop = proxy->GetProperty("ScaleFactor")) != 0)
    {
    this->Internal->Links.addPropertyLink(this->Internal->GlyphScaleFactorSpinBox,
      "value", SIGNAL(valueChanged(double)),
      proxy, proxy->GetProperty("ScaleFactor"));
    this->Internal->GlyphScaleFactorSpinBox->setEnabled(true);
    }
  else
    {
    this->Internal->GlyphScaleFactorSpinBox->setEnabled(false);
    }
  
  vtkSMProperty* glyphModeProperty = proxy->GetProperty("GlyphMode");

  int glyphMode = pqSMAdaptor::getElementProperty(glyphModeProperty).toInt();
  this->Internal->GlyphGroupBox->setChecked(glyphMode);
  this->Internal->GlyphScalingCheckBox->setChecked(glyphMode & vtkScatterPlotMapper::ScaledGlyph);
  this->Internal->GlyphMultiSourceCheckBox->setChecked(glyphMode & vtkScatterPlotMapper::UseMultiGlyph);
  this->Internal->GlyphOrientationCheckBox->setChecked(glyphMode & vtkScatterPlotMapper::OrientedGlyph);

  this->DisableSlots = 0;

  // Request a render when any GUI widget is changed by the user.
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()), Qt::QueuedConnection);

  this->update3DMode();

  // The defaults glyphs are used here. As they are 2D glyphs, we force them to
  // be parallel to the camera at all time.
  vtkSMIntVectorProperty* parallelProperty = 
    vtkSMIntVectorProperty::SafeDownCast(
      this->Internal->ScatterPlotRepresentation->GetProperty("ParallelToCamera"));
  parallelProperty->SetElement(0, 1);
  this->Internal->ScatterPlotRepresentation->UpdateProperty("ParallelToCamera");
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::zoomToData()
{
  if (this->DisableSlots)
    {
    return;
    }

  double bounds[6];
  this->Internal->ScatterPlotRepresentation->GetBounds(bounds);
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
    pqScatterPlotView* scatterPlotModule = qobject_cast<pqScatterPlotView*>(
      this->Internal->Representation->getView());
    if (scatterPlotModule)
      {
      vtkSMScatterPlotViewProxy* rm = 
        scatterPlotModule->getScatterPlotViewProxy();
      rm->GetRenderView()->ResetCamera(bounds);
      scatterPlotModule->render();
      }
    }
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::update3DMode()
{  
  if (this->DisableSlots)
    {
    return;
    }
  
  pqScatterPlotView* renModule = qobject_cast<pqScatterPlotView*>(
    this->Internal->Representation->getView());
  if(!renModule)
    {
    return;
    }
  renModule->getScatterPlotViewProxy()->GetRenderView()->GetActiveCamera()
    ->SetPosition(0., 0., 1.);
  renModule->getScatterPlotViewProxy()->GetRenderView()->GetActiveCamera()
    ->SetFocalPoint(0., 0., 0.);
  renModule->getScatterPlotViewProxy()->GetRenderView()->GetActiveCamera()
    ->SetViewUp(0., 1., 0.);
    
  renModule->set3DMode(this->Internal->ZCoordsCheckBox->isChecked());
  this->zoomToData();
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::openColorMapEditor()
{
  // Get the color scale editor from the application core's registry.
  pqColorScaleToolbar *colorScale = qobject_cast<pqColorScaleToolbar *>(
      pqApplicationCore::instance()->manager("COLOR_SCALE_EDITOR"));
  if(colorScale)
    {
    colorScale->editColorMap(this->Internal->Representation);
    }
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::rescaleColorToDataRange()
{
  if(this->Internal->Representation.isNull())
    {
    return;
    }
  this->Internal->Representation->resetLookupTableScalarRange();
  this->Internal->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::updateGlyphMode()
{  
  if (this->DisableSlots)
    {
    return;
    }
  int glyphMode = 0;
  if(this->Internal->GlyphGroupBox->isChecked())
    {
    glyphMode |= vtkScatterPlotMapper::UseGlyph;
    }
  if(this->Internal->GlyphScalingCheckBox->isChecked())
    {
    glyphMode |= vtkScatterPlotMapper::ScaledGlyph;
    }
  if(this->Internal->GlyphMultiSourceCheckBox->isChecked())
    {
    glyphMode |= vtkScatterPlotMapper::UseMultiGlyph;
    }
  if(this->Internal->GlyphOrientationCheckBox->isChecked())
    {
    glyphMode |= vtkScatterPlotMapper::OrientedGlyph;
    }
  vtkSMIntVectorProperty* glyphModeProperty = 
    vtkSMIntVectorProperty::SafeDownCast(
      this->Internal->ScatterPlotRepresentation->GetProperty("GlyphMode"));
  glyphModeProperty->SetElement(0, glyphMode);
  this->Internal->ScatterPlotRepresentation->UpdateProperty("GlyphMode");
  this->Internal->ScatterPlotRepresentation->UpdateVTKObjects();
  this->updateAllViews();
}


//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::onColorChanged()
{ 
  if (this->DisableSlots)
    {
    return;
    }
  this->Internal->Representation->resetLookupTableScalarRange();
  this->Internal->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::cubeAxesVisibilityChanged()
{
  if (this->DisableSlots)
    {
    return;
    }
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
void pqScatterPlotDisplayPanel::openCubeAxesEditor()
{
  pqCubeAxesEditorDialog dialog(this);
  dialog.setRepresentationProxy(this->Internal->Representation->getProxy());
  dialog.exec();
}
