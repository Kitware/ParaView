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

//#include "vtkSMChartRepresentationProxy.h"
//#include "vtkQtChartRepresentation.h"
#include "vtkSMScatterPlotRepresentationProxy.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkWeakPointer.h"
#include "vtkSMScatterPlotViewProxy.h"

#include <QColorDialog>
#include <QHeaderView>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QList>
#include <QPointer>
#include <QPixmap>
#include <QDebug>
#include <QTimer>

#include "pqApplicationCore.h"
#include "pqStandardColorButton.h"
#include "pqUndoStack.h"
#include "pqRenderView.h"
#include "pqScatterPlotView.h"
#include "pqColorScaleToolbar.h"
//#include "pqChartSeriesEditorModel.h"
#include "pqComboBoxDomain.h"
#include "pqOutputPort.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqView.h"
#include "pqDataRepresentation.h"
#include "vtkScatterPlotMapper.h"
#include "pqPipelineRepresentation.h"
#include "pqScatterPlotRepresentation.h"
#include "vtkCamera.h"

#include <assert.h>

// Protect the qt model classes in an anonymous namespace
namespace {
  //-----------------------------------------------------------------------------
  class pqLineSeriesEditorDelegate : public QItemDelegate
  {
  typedef QItemDelegate Superclass;
public:
  pqLineSeriesEditorDelegate(QObject *parentObject=0)
    : Superclass(parentObject) { }
  virtual ~pqLineSeriesEditorDelegate() {}

  virtual bool eventFilter(QObject *object, QEvent *e)
    {
    // When the user presses the tab key, Qt tries to edit the next
    // item. If the item is not editable, Qt pops up a warning
    // "edit: editing failed". According to the tree view, the next
    // item is always in column zero, which is never editable. This
    // workaround avoids the edit next hint to prevent the message.
    if(e->type() == QEvent::KeyPress)
      {
      QKeyEvent *ke = static_cast<QKeyEvent *>(e);
      if(ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab)
        {
        QWidget *editor = qobject_cast<QWidget *>(object);
        if(!editor)
          {
          return false;
          }

        emit this->commitData(editor);
        emit this->closeEditor(editor, QAbstractItemDelegate::NoHint);
        return true;
        }
      }

    return this->Superclass::eventFilter(object, e);
    }
protected:
  virtual void drawDecoration(QPainter *painter,
    const QStyleOptionViewItem &options, const QRect &area,
    const QPixmap &pixmap) const
    {
    // Remove the selected flag from the state to make sure the pixmap
    // color is not modified.
    QStyleOptionViewItem newOptions = options;
    newOptions.state = options.state & ~QStyle::State_Selected;
    QItemDelegate::drawDecoration(painter, newOptions, area, pixmap);
    }
  };
} // End anonymous namespace


// Protect the qt model classes in an anonymous namespace
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
      return data.right(data.length() - textPos);
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
    
    this->AttributeModeAdaptor = 0;
    //this->Model = 0;
    this->InChange = false;
    this->CompositeIndexAdaptor = 0;
    this->AmbientColorAdaptor = 0;
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
    
    delete this->AttributeModeAdaptor;
    delete this->CompositeIndexAdaptor;
    delete this->AmbientColorAdaptor;
    }

  pqPropertyLinks Links;

  pqSignalAdaptorComboBox* XAxisArrayAdaptor;
  pqSignalAdaptorComboBox* YAxisArrayAdaptor;
  pqSignalAdaptorComboBox* ZAxisArrayAdaptor;
  pqSignalAdaptorComboBox* ColorArrayAdaptor;
  pqSignalAdaptorComboBox* GlyphScalingArrayAdaptor;
  pqSignalAdaptorComboBox* GlyphMultiSourceArrayAdaptor;
  pqSignalAdaptorComboBox* GlyphOrientationArrayAdaptor;
  pqSignalAdaptorComboBox* AttributeModeAdaptor;

  pqComboBoxDecoratedDomain* XAxisArrayDomain;
  pqComboBoxDecoratedDomain* YAxisArrayDomain;
  pqComboBoxDecoratedDomain* ZAxisArrayDomain;
  pqComboBoxDecoratedDomain* ColorArrayDomain;
  pqComboBoxDecoratedDomain* GlyphScalingArrayDomain;
  pqComboBoxDecoratedDomain* GlyphMultiSourceArrayDomain;
  pqComboBoxDecoratedDomain* GlyphOrientationArrayDomain;

  pqSignalAdaptorCompositeTreeWidget* CompositeIndexAdaptor;
  pqSignalAdaptorColor*    AmbientColorAdaptor;
  pqSignalAdaptorCompositeTreeWidget* CompositeTreeAdaptor;
  //vtkSMScatterPlotRepresentationProxy* ScatterPlotRepresentation;
  vtkWeakPointer<vtkSMScatterPlotRepresentationProxy> ScatterPlotRepresentation;
  QPointer<pqScatterPlotRepresentation> Representation;

  //pqChartSeriesEditorModel *Model;

  bool InChange;
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
  //QObject::connect(this->Internal->EditCubeAxes, SIGNAL(clicked(bool)),
  //  this, SLOT(editCubeAxes()));
  //QObject::connect(this->Internal->compositeTree, SIGNAL(itemSelectionChanged()),
  //  this, SLOT(volumeBlockSelected()));

/*
  this->Internal->SeriesList->setItemDelegate(
      new pqLineSeriesEditorDelegate(this));
  this->Internal->Model = new pqChartSeriesEditorModel(this);
  this->Internal->SeriesList->setModel(this->Internal->Model);

  QObject::connect(
    this->Internal->SeriesList, SIGNAL(activated(const QModelIndex &)),
    this, SLOT(activateItem(const QModelIndex &)));
*/
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
/*
  this->Internal->AttributeModeAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->AttributeMode);
  
  QObject::connect(this->Internal->UseArrayIndex, SIGNAL(toggled(bool)), 
    this, SLOT(useArrayIndexToggled(bool)));
  QObject::connect(this->Internal->UseDataArray, SIGNAL(toggled(bool)), 
    this, SLOT(useDataArrayToggled(bool)));

  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  QObject::connect(model,
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    this, SLOT(updateOptionsWidgets()));
  QObject::connect(model,
    SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    this, SLOT(updateOptionsWidgets()));
  QObject::connect(this->Internal->Model, SIGNAL(modelReset()),
    this, SLOT(updateOptionsWidgets()));

  QObject::connect(this->Internal->SeriesEnabled, SIGNAL(stateChanged(int)),
    this, SLOT(setCurrentSeriesEnabled(int)));
  QObject::connect(
    this->Internal->ColorButton, SIGNAL(chosenColorChanged(const QColor &)),
    this, SLOT(setCurrentSeriesColor(const QColor &)));
  QObject::connect(this->Internal->Thickness, SIGNAL(valueChanged(int)),
    this, SLOT(setCurrentSeriesThickness(int)));
  QObject::connect(this->Internal->StyleList, SIGNAL(currentIndexChanged(int)),
    this, SLOT(setCurrentSeriesStyle(int)));
  QObject::connect(this->Internal->AxisList, SIGNAL(currentIndexChanged(int)),
    this, SLOT(setCurrentSeriesAxes(int)));
  QObject::connect(this->Internal->MarkerStyleList, SIGNAL(currentIndexChanged(int)),
    this, SLOT(setCurrentSeriesMarkerStyle(int)));
*/
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
    this, SLOT(rescaleToDataRange()));

  // Create an connect signal adapters.
  if (!QMetaType::isRegistered(QMetaType::type("QVariant")))
    {
    qRegisterMetaType<QVariant>("QVariant");
    }

  //this->Internal->InterpolationAdaptor = new pqSignalAdaptorComboBox(
  //  this->Internal->StyleInterpolation);
  //this->Internal->InterpolationAdaptor->setObjectName(
  //  "StyleInterpolationAdapator");
  /*
  QObject::connect(this->Internal->ColorActorColor,
    SIGNAL(chosenColorChanged(const QColor&)),
    this, SLOT(setSolidColor(const QColor&)));
  */
  /// Set up signal-slot connections to create a single undo-set for all the
  /// changes that happen when the solid color is changed.
  /// We need to do this for both solid and edge color since we want to make
  /// sure that the undo-element for setting up of the "global property" link
  /// gets added in the same set in which the solid/edge color is changed.
  //this->Internal->ColorActorColor->setUndoLabel("Change Solid Color");
  /*
  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (stack)
    {
    QObject::connect(this->Internal->ColorActorColor,
      SIGNAL(beginUndo(const QString&)),
      stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this->Internal->ColorActorColor,
      SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    }
  */
//   this->Internal->EdgeColorAdaptor = new pqSignalAdaptorColor(
//     this->Internal->EdgeColor, "chosenColor",
//     SIGNAL(chosenColorChanged(const QColor&)), false);
//   this->Internal->EdgeColor->setUndoLabel("Change Edge Color");
//   if (stack)
//     {
//     QObject::connect(this->Internal->EdgeColor,
//       SIGNAL(beginUndo(const QString&)),
//       stack, SLOT(beginUndoSet(const QString&)));
//     QObject::connect(this->Internal->EdgeColor,
//       SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
//     }
  
  // this->Internal->AmbientColorAdaptor = new pqSignalAdaptorColor(
//     this->Internal->AmbientColor, "chosenColor",
//     SIGNAL(chosenColorChanged(const QColor&)), false);
//   this->Internal->AmbientColor->setUndoLabel("Change Ambient Color");
//   if (stack)
//     {
//     QObject::connect(this->Internal->AmbientColor,
//       SIGNAL(beginUndo(const QString&)),
//       stack, SLOT(beginUndoSet(const QString&)));
//     QObject::connect(this->Internal->AmbientColor,
//       SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
//     }

  //QObject::connect(this->Internal->StyleMaterial, SIGNAL(currentIndexChanged(int)),
  //                 this, SLOT(updateMaterial(int)));
/*
  this->Internal->SliceDirectionAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->SliceDirection);
  QObject::connect(this->Internal->SliceDirectionAdaptor,
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(sliceDirectionChanged()), Qt::QueuedConnection);
*/
/*
  this->Internal->SelectedMapperAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->SelectedMapperIndex);
  QObject::connect(this->Internal->SelectedMapperAdaptor,
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(selectedMapperChanged()), Qt::QueuedConnection);
*/
/*
  this->Internal->BackfaceRepresentationAdaptor = new pqSignalAdaptorComboBox(
                                   this->Internal->BackfaceStyleRepresentation);
  this->Internal->BackfaceRepresentationAdaptor->setObjectName(
                                         "BackfaceStyleRepresentationAdapator");

  QObject::connect(this->Internal->BackfaceActorColor,
                   SIGNAL(chosenColorChanged(const QColor&)),
                   this, SLOT(setBackfaceSolidColor(const QColor&)));

  this->Internal->BackfaceActorColor->setUndoLabel("Change Backface Solid Color");
  stack = pqApplicationCore::instance()->getUndoStack();
  if (stack)
    {
    QObject::connect(this->Internal->BackfaceActorColor,
                     SIGNAL(beginUndo(const QString&)),
                     stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this->Internal->BackfaceActorColor,
                     SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    }
*/
}


//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::reloadSeries()
{
  //this->Internal->Model->reload();
  this->updateOptionsWidgets();
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

  vtkSMProperty* prop = 0;
  // setup cube axes visibility.
  if ((prop = proxy->GetProperty("CubeAxesVisibility")) != 0)
    {
     QObject::connect(this->Internal->ShowCubeAxes, SIGNAL(toggled(bool)),
       this, SLOT(cubeAxesVisibilityChanged()));
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

  // this->Internal->ColorBy->setRepresentation(this->Internal->Representation);
//   QObject::connect(this->Internal->ColorBy,
//     SIGNAL(modified()),
//     this, SLOT(updateEnableState()), Qt::QueuedConnection);

//   this->Internal->StyleRepresentation->setRepresentation(repr);
//   QObject::connect(this->Internal->StyleRepresentation,
//     SIGNAL(currentTextChanged(const QString&)),
//     this->Internal->ColorBy, SLOT(reloadGUI()));

//   QObject::connect(this->Internal->StyleRepresentation,
//     SIGNAL(currentTextChanged(const QString&)),
//     this, SLOT(updateEnableState()), Qt::QueuedConnection);

  //this->Internal->Texture->setRepresentation(this->Internal->Representation);


  if (proxy->GetProperty("ExtractedBlockIndex"))
    {
    this->Internal->CompositeTreeAdaptor = 
      new pqSignalAdaptorCompositeTreeWidget(
        this->Internal->compositeTree,
        this->Internal->Representation->getOutputPortFromInput()->getOutputPortProxy(),
        vtkSMCompositeTreeDomain::NONE,
        pqSignalAdaptorCompositeTreeWidget::INDEX_MODE_FLAT, false, true);
    }

//   if (reprProxy->GetProperty("SelectedMapperIndex"))
//     {
//     QList<QVariant> mapperNames = 
//       pqSMAdaptor::getEnumerationPropertyDomain(
//         reprProxy->GetProperty("SelectedMapperIndex"));
//     foreach(QVariant item, mapperNames)
//       {
//       this->Internal->SelectedMapperIndex->addItem(item.toString());
//       }
//     this->Internal->Links->addPropertyLink(
//       this->Internal->SelectedMapperAdaptor,
//       "currentText", SIGNAL(currentTextChanged(const QString&)),
//       reprProxy, reprProxy->GetProperty("SelectedMapperIndex"));
//     }

  this->DisableSlots = 0;
  QTimer::singleShot(0, this, SLOT(updateEnableState()));
  // Attribute mode.
/*
  this->Internal->Links.addPropertyLink(
    this->Internal->UseArrayIndex, "checked",
    SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("UseIndexForXAxis"));

  this->Internal->Links.addPropertyLink(this->Internal->AttributeModeAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("AttributeType"));
*/
  // Set up the CompositeIndexAdaptor 
/*  this->Internal->CompositeIndexAdaptor = new pqSignalAdaptorCompositeTreeWidget(
    this->Internal->CompositeIndex, 
    vtkSMIntVectorProperty::SafeDownCast(
      proxy->GetProperty("CompositeDataSetIndex")),true);


  this->Internal->Links.addPropertyLink(this->Internal->CompositeIndexAdaptor,
    "values", SIGNAL(valuesChanged()),
    proxy, proxy->GetProperty("CompositeDataSetIndex"));
*/
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
  this->Internal->ZAxisArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->ZAxisArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("ZArrayName"));

  this->Internal->ColorArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->ColorComboBox, proxy->GetProperty("ColorArrayName"));
  this->Internal->ColorArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->ColorArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("ColorArrayName"));

  this->Internal->GlyphScalingArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->GlyphScalingComboBox, proxy->GetProperty("GlyphScalingArrayName"));
  this->Internal->GlyphScalingArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->GlyphScalingArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("GlyphScalingArrayName"));

  this->Internal->GlyphMultiSourceArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->GlyphMultiSourceComboBox, proxy->GetProperty("GlyphMultiSourceArrayName"));
  this->Internal->GlyphMultiSourceArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->GlyphMultiSourceArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("GlyphMultiSourceArrayName"));

  this->Internal->GlyphOrientationArrayDomain = new pqComboBoxDecoratedDomain(
    this->Internal->GlyphOrientationComboBox, proxy->GetProperty("GlyphOrientationArrayName"));
  this->Internal->GlyphOrientationArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->GlyphOrientationArrayAdaptor,
    "currentData", SIGNAL(currentIndexChanged(int)),
    proxy, proxy->GetProperty("GlyphOrientationArrayName"));
  
  // setup for ThreeDMode
  this->Internal->Links.addPropertyLink(
    this->Internal->ZCoordsCheckBox, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("ThreeDMode"));

  this->Internal->Links.addPropertyLink(
    this->Internal->ColorCheckBox, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Colorize"));

/*  this->Internal->Links.addPropertyLink(
    this->Internal->GlyphGroupBox, "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("GlyphMode"));  
*/
  vtkSMProperty* threeDModeProperty = proxy->GetProperty("ThreeDMode");
  this->Internal->ZCoordsCheckBox->setChecked(
    pqSMAdaptor::getElementProperty(threeDModeProperty).toInt());

  vtkSMProperty* colorizeProperty = proxy->GetProperty("Colorize");
  this->Internal->ColorCheckBox->setChecked(
    pqSMAdaptor::getElementProperty(colorizeProperty).toInt());

  vtkSMProperty* glyphModeProperty = proxy->GetProperty("GlyphMode");
  int glyphMode = pqSMAdaptor::getElementProperty(glyphModeProperty).toInt();
  this->Internal->GlyphGroupBox->setChecked(glyphMode);
  this->Internal->GlyphScalingCheckBox->setChecked(glyphMode & vtkScatterPlotMapper::GLYPH_X_SCALE);
  this->Internal->GlyphMultiSourceCheckBox->setChecked(glyphMode & vtkScatterPlotMapper::GLYPH_SOURCE);
  this->Internal->GlyphOrientationCheckBox->setChecked(glyphMode & vtkScatterPlotMapper::GLYPH_X_ORIENTATION);

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
  
  QObject::connect(this->Internal->ZCoordsCheckBox,
                   SIGNAL(stateChanged(int)),
                   this, SLOT(update3DMode()), Qt::QueuedConnection);

  // Request a render when any GUI widget is changed by the user.
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()), Qt::QueuedConnection);

  this->reloadSeries();
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

void pqScatterPlotDisplayPanel::update3DMode()
{
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
void pqScatterPlotDisplayPanel::rescaleToDataRange()
{
  if(this->Internal->Representation.isNull())
    {
    return;
    }
  this->Internal->Representation->resetLookupTableScalarRange();
  this->Internal->Representation->renderViewEventually();
}

/*
//-----------------------------------------------------------------------------
void vtkSMScatterPlotRepresentationProxy::resetLookupTableScalarRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  QString colorField = this->getColorField();
  if (lut && colorField != "" && 
    colorField != pqPipelineRepresentation::solidColor())
    {
    QPair<double,double> range = this->getColorFieldRange();
    lut->setScalarRange(range.first, range.second);

    // scalar opacity is treated as slave to the lookup table.
    pqScalarOpacityFunction* opacity = this->getScalarOpacityFunction();
    if(opacity)
      {
      opacity->setScalarRange(range.first, range.second);
      }
    }
}
*/


void pqScatterPlotDisplayPanel::updateGlyphMode()
{
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
// Called when the GUI selection for the solid color changes.
void pqScatterPlotDisplayPanel::setSolidColor(const QColor& color)
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
QVariant pqScatterPlotDisplayPanel::specularColor() const
{
//   if(this->Internal->SpecularWhite->isChecked())
//     {
//     QList<QVariant> ret;
//     ret.append(1.0);
//     ret.append(1.0);
//     ret.append(1.0);
//     return ret;
//     }

  vtkSMProxy* proxy = this->Internal->ScatterPlotRepresentation;
  return pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty("DiffuseColor"));
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::activateItem(const QModelIndex &index)
{
  if(!this->Internal->ScatterPlotRepresentation
      || !index.isValid() || index.column() != 1)
    {
    // We are interested in clicks on the color swab alone.
    return;
    }

  // Get current color
  QColor color; //= this->Internal->Model->getSeriesColor(index.row());

  // Show color selector dialog to get a new color
  color = QColorDialog::getColor(color, this);
  if (color.isValid())
    {
    // Set the new color
    //this->Internal->Model->setSeriesColor(index.row(), color);
    //this->Internal->ColorButton->blockSignals(true);
    //this->Internal->ColorButton->setChosenColor(color);
    //this->Internal->ColorButton->blockSignals(false);
    this->updateAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::updateOptionsWidgets()
{
/*
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    // Use the selection list to determine the tri-state of the
    // enabled and legend check boxes.
    this->Internal->SeriesEnabled->blockSignals(true);
    this->Internal->SeriesEnabled->setCheckState(this->getEnabledState());
    this->Internal->SeriesEnabled->blockSignals(false);

    // Show the options for the current item.
    QModelIndex current = model->currentIndex();
    QModelIndexList indexes = model->selectedIndexes();
    if((!current.isValid() || !model->isSelected(current)) &&
        indexes.size() > 0)
      {
      current = indexes.last();
      }

    this->Internal->ColorButton->blockSignals(true);
    this->Internal->Thickness->blockSignals(true);
    this->Internal->StyleList->blockSignals(true);
    this->Internal->MarkerStyleList->blockSignals(true);
    this->Internal->AxisList->blockSignals(true);
    if (current.isValid())
      {
      int seriesIndex = current.row();
      QColor color = this->Internal->Model->getSeriesColor(seriesIndex);
      this->Internal->ColorButton->setChosenColor(color);
      this->Internal->Thickness->setValue(
        this->Internal->Model->getSeriesThickness(seriesIndex));
      this->Internal->StyleList->setCurrentIndex(
        this->Internal->Model->getSeriesStyle(seriesIndex));
      this->Internal->MarkerStyleList->setCurrentIndex(
        this->Internal->Model->getSeriesMarkerStyle(seriesIndex));
      this->Internal->AxisList->setCurrentIndex(
        this->Internal->Model->getSeriesAxisCorner(seriesIndex));
      }
    else
      {
      this->Internal->ColorButton->setChosenColor(Qt::white);
      this->Internal->Thickness->setValue(1);
      this->Internal->StyleList->setCurrentIndex(0);
      this->Internal->MarkerStyleList->setCurrentIndex(0);
      this->Internal->AxisList->setCurrentIndex(0);
      }

    this->Internal->ColorButton->blockSignals(false);
    this->Internal->Thickness->blockSignals(false);
    this->Internal->StyleList->blockSignals(false);
    this->Internal->MarkerStyleList->blockSignals(false);
    this->Internal->AxisList->blockSignals(false);

    // Disable the widgets if nothing is selected or current.
    bool hasItems = indexes.size() > 0;
    this->Internal->SeriesEnabled->setEnabled(hasItems);
    this->Internal->ColorButton->setEnabled(hasItems);
    this->Internal->Thickness->setEnabled(hasItems);
    this->Internal->StyleList->setEnabled(hasItems);
    this->Internal->MarkerStyleList->setEnabled(hasItems);
    this->Internal->AxisList->setEnabled(hasItems);
    }*/
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::setCurrentSeriesEnabled(int state)
{
  if(state == Qt::PartiallyChecked)
    {
    // Ignore changes to partially checked state.
    return;
    }
/*
  bool enabled = state == Qt::Checked;
  this->Internal->SeriesEnabled->setTristate(false);
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Model->setSeriesEnabled(iter->row(), enabled);
      }

    this->Internal->InChange = false;
    this->updateAllViews();
    }
*/
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::setCurrentSeriesColor(const QColor &vtkNotUsed(color))
{
/*  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Model->setSeriesColor(iter->row(), color);
      }

    this->Internal->InChange = false;
    this->updateAllViews();
    }
*/
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::setCurrentSeriesThickness(int vtkNotUsed(thickness))
{
/*  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if (model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Model->setSeriesThickness(iter->row(), thickness);
      }

    this->Internal->InChange = false;
    this->updateAllViews();
    }
*/
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::setCurrentSeriesStyle(int vtkNotUsed(listIndex))
{
/*  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Model->setSeriesStyle(iter->row(), listIndex);
      }

    this->Internal->InChange = false;
    this->updateAllViews();
    }
*/
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::setCurrentSeriesMarkerStyle(int vtkNotUsed(listIndex))
{
/*  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Model->setSeriesMarkerStyle(iter->row(), listIndex);
      }

    this->Internal->InChange = false;
    this->updateAllViews();
    }
*/
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::setCurrentSeriesAxes(int vtkNotUsed(listIndex))
{
/*  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Model->setSeriesAxisCorner(iter->row(), listIndex);
      }

    this->Internal->InChange = false;
    this->updateAllViews();
    }
*/
}

//-----------------------------------------------------------------------------
Qt::CheckState pqScatterPlotDisplayPanel::getEnabledState() const
{

  Qt::CheckState enabledState = Qt::Unchecked;
/*  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    // Use the selection list to determine the tri-state of the
    // enabled check box.
    bool enabled = false;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for(int i = 0; iter != indexes.end(); ++iter, ++i)
      {
      enabled = this->Internal->Model->getSeriesEnabled(iter->row()); 
      if (i == 0)
        {
        enabledState = enabled ? Qt::Checked : Qt::Unchecked;
        }
      else if((enabled && enabledState == Qt::Unchecked) ||
          (!enabled && enabledState == Qt::Checked))
        {
        enabledState = Qt::PartiallyChecked;
        break;
        }
      }
    }
*/
  return enabledState;
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::useArrayIndexToggled(bool vtkNotUsed(toggle))
{
  //this->Internal->UseDataArray->setChecked(!toggle);
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::useDataArrayToggled(bool vtkNotUsed(toggle))
{
  //this->Internal->UseArrayIndex->setChecked(!toggle);
}

//-----------------------------------------------------------------------------
void pqScatterPlotDisplayPanel::updateEnableState()
{
  this->Internal->ColorInterpolateScalars->setEnabled(true);
  //this->Internal->ColorButtonStack->setCurrentWidget(
  //  this->Internal->ColorMapPage);
  //this->Internal->BackfaceActorColor->setEnabled(false);
  /*
  int reprType = this->Internal->Representation->getRepresentationType();

  if (this->Internal->ColorBy->getCurrentText() == "Solid Color")
    {
    this->Internal->ColorInterpolateScalars->setEnabled(false);
    if (reprType == vtkSMPVRepresentationProxy::WIREFRAME ||
      reprType == vtkSMPVRepresentationProxy::POINTS ||
      reprType == vtkSMPVRepresentationProxy::OUTLINE)
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
    reprType == vtkSMPVRepresentationProxy::SURFACE_WITH_EDGES);

  this->Internal->SliceGroup->setEnabled(
    reprType == vtkSMPVRepresentationProxy::SLICE);
  if (reprType == vtkSMPVRepresentationProxy::SLICE)
    {
    // every time the user switches to Slice mode we update the domain for the
    // slider since the domain depends on the input to the image mapper which
    // may have changed.
    QTimer::singleShot(0, this, SLOT(sliceDirectionChanged()));
    }

  this->Internal->compositeTree->setVisible(
   this->Internal->CompositeTreeAdaptor &&
   (reprType == vtkSMPVRepresentationProxy::VOLUME));

  this->Internal->SelectedMapperIndex->setEnabled(
    reprType == vtkSMPVRepresentationProxy::VOLUME
    && this->Internal->Representation->getProxy()->GetProperty("SelectedMapperIndex"));

  vtkSMProperty *backfaceRepProperty = this->Internal->Representation
    ->getRepresentationProxy()->GetProperty("BackfaceRepresentation");
  if (   !backfaceRepProperty
      || (   (reprType != vtkSMPVRepresentationProxy::POINTS)
          && (reprType != vtkSMPVRepresentationProxy::WIREFRAME)
          && (reprType != vtkSMPVRepresentationProxy::SURFACE)
          && (reprType != vtkSMPVRepresentationProxy::SURFACE_WITH_EDGES) ) )
    {
    this->Internal->BackfaceStyleGroup->setEnabled(false);
    }
  else
    {
    this->Internal->BackfaceStyleGroup->setEnabled(true);
    int backRepType
      = pqSMAdaptor::getElementProperty(backfaceRepProperty).toInt();

    bool backFollowsFront
      = (   (backRepType == vtkSMPVRepresentationProxy::FOLLOW_FRONTFACE)
         || (backRepType == vtkSMPVRepresentationProxy::CULL_BACKFACE)
         || (backRepType == vtkSMPVRepresentationProxy::CULL_FRONTFACE) );

    this->Internal->BackfaceStyleGroupOptions->setEnabled(!backFollowsFront);
    }

  vtkSMDataRepresentationProxy* display = 
    this->Internal->Representation->getRepresentationProxy();
  if (display)
    {
    QVariant scalarMode = pqSMAdaptor::getEnumerationProperty(
      display->GetProperty("ColorAttributeType"));
    vtkPVDataInformation* geomInfo = 
      display->GetRepresentedDataInformation(false);
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
    }

  this->Internal->ColorMapScalars->setCheckState(Qt::Checked);
  this->Internal->ColorMapScalars->setEnabled(false);
  */
}
