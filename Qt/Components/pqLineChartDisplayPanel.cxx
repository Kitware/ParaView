/*=========================================================================

   Program: ParaView
   Module:    pqLineChartDisplayPanel.cxx

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
#include "pqLineChartDisplayPanel.h"
#include "ui_pqLineChartDisplayPanel.h"

#include "vtkSMChartRepresentationProxy.h"
#include "vtkQtChartRepresentation.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"

#include <QColorDialog>
#include <QHeaderView>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QList>
#include <QPointer>
#include <QPixmap>
#include <QDebug>

#include "pqChartSeriesEditorModel.h"
#include "pqComboBoxDomain.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqView.h"
#include "pqDataRepresentation.h"

#include <assert.h>


///////////// DEPRECATED **********************************************
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


//-----------------------------------------------------------------------------
class pqLineChartDisplayPanel::pqInternal : public Ui::pqLineChartDisplayPanel
{
public:
  pqInternal()
    {
    this->XAxisArrayDomain = 0;
    this->XAxisArrayAdaptor = 0;
    this->AttributeModeAdaptor = 0;
    this->Model = 0;
    this->InChange = false;
    this->CompositeIndexAdaptor = 0;
    }

  ~pqInternal()
    {
    delete this->XAxisArrayAdaptor;
    delete this->XAxisArrayDomain;
    delete this->AttributeModeAdaptor;
    delete this->CompositeIndexAdaptor;
    }

  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* XAxisArrayAdaptor;
  pqSignalAdaptorComboBox* AttributeModeAdaptor;
  pqComboBoxDomain* XAxisArrayDomain;
  pqSignalAdaptorCompositeTreeWidget* CompositeIndexAdaptor;

  vtkWeakPointer<vtkSMChartRepresentationProxy> ChartRepresentation;

  pqChartSeriesEditorModel *Model;

  bool InChange;
};

//-----------------------------------------------------------------------------
pqLineChartDisplayPanel::pqLineChartDisplayPanel(
  pqRepresentation* display,QWidget* p)
: pqDisplayPanel(display, p)
{
  this->Internal = new pqLineChartDisplayPanel::pqInternal();
  this->Internal->setupUi(this);

  this->Internal->SeriesList->setItemDelegate(
      new pqLineSeriesEditorDelegate(this));
  this->Internal->Model = new pqChartSeriesEditorModel(this);
  this->Internal->SeriesList->setModel(this->Internal->Model);

  QObject::connect(
    this->Internal->SeriesList, SIGNAL(activated(const QModelIndex &)),
    this, SLOT(activateItem(const QModelIndex &)));

  this->Internal->XAxisArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->XAxisArray);

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

  this->setDisplay(display);
}

//-----------------------------------------------------------------------------
pqLineChartDisplayPanel::~pqLineChartDisplayPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::reloadSeries()
{
  this->Internal->Model->reload();
  this->updateOptionsWidgets();
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::setDisplay(pqRepresentation* disp)
{
  this->setEnabled(false);

  // This method can only be called once in the constructor, so I am removing
  // all "cleanup" code, since it's not applicable.
  assert(this->Internal->ChartRepresentation == 0);

  vtkSMChartRepresentationProxy* proxy =
    vtkSMChartRepresentationProxy::SafeDownCast(disp->getProxy());
  this->Internal->ChartRepresentation = proxy;
  if (!this->Internal->ChartRepresentation)
    {
    qWarning() << "pqLineChartDisplayPanel given a representation proxy "
                  "that is not a LineChartRepresentation.  Cannot edit.";
    return;
    }

  // this is essential to ensure that when you undo-redo, the representation is
  // indeed update-to-date, thus ensuring correct domains etc.
  proxy->Update();

  // Give the representation to our series editor model
  this->Internal->Model->setRepresentation(
    qobject_cast<pqDataRepresentation*>(disp));

  this->setEnabled(true);

  // Connect ViewData checkbox to the proxy's Visibility property
  this->Internal->Links.addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));

  // Attribute mode.
  this->Internal->Links.addPropertyLink(
    this->Internal->UseArrayIndex, "checked",
    SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("UseIndexForXAxis"));

  this->Internal->Links.addPropertyLink(this->Internal->AttributeModeAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("AttributeType"));

  // Set up the CompositeIndexAdaptor 
  this->Internal->CompositeIndexAdaptor = new pqSignalAdaptorCompositeTreeWidget(
    this->Internal->CompositeIndex, 
    vtkSMIntVectorProperty::SafeDownCast(
      proxy->GetProperty("CompositeDataSetIndex")),
    /*autoUpdateVisibility=*/true);

  this->Internal->Links.addPropertyLink(this->Internal->CompositeIndexAdaptor,
    "values", SIGNAL(valuesChanged()),
    proxy, proxy->GetProperty("CompositeDataSetIndex"));

  // Connect to the new properties.pqComboBoxDomain will ensure that
  // when ever the domain changes the widget is updated as well.
  this->Internal->XAxisArrayDomain = new pqComboBoxDomain(
      this->Internal->XAxisArray, proxy->GetProperty("XArrayName"));
  this->Internal->XAxisArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(this->Internal->XAxisArrayAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      proxy, proxy->GetProperty("XArrayName"));

  // Request a render when any GUI widget is changed by the user.
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()), Qt::QueuedConnection);

  this->reloadSeries();
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::activateItem(const QModelIndex &index)
{
  if(!this->Internal->ChartRepresentation
      || !index.isValid() || index.column() != 1)
    {
    // We are interested in clicks on the color swab alone.
    return;
    }

  // Get current color
  QColor color = this->Internal->Model->getSeriesColor(index.row());

  // Show color selector dialog to get a new color
  color = QColorDialog::getColor(color, this);
  if (color.isValid())
    {
    // Set the new color
    this->Internal->Model->setSeriesColor(index.row(), color);
    this->Internal->ColorButton->blockSignals(true);
    this->Internal->ColorButton->setChosenColor(color);
    this->Internal->ColorButton->blockSignals(false);
    this->updateAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::updateOptionsWidgets()
{
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
    }
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::setCurrentSeriesEnabled(int state)
{
  if(state == Qt::PartiallyChecked)
    {
    // Ignore changes to partially checked state.
    return;
    }

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
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::setCurrentSeriesColor(const QColor &color)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
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
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::setCurrentSeriesThickness(int thickness)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
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
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::setCurrentSeriesStyle(int listIndex)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
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
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::setCurrentSeriesMarkerStyle(int listIndex)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
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
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::setCurrentSeriesAxes(int listIndex)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
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
}

//-----------------------------------------------------------------------------
Qt::CheckState pqLineChartDisplayPanel::getEnabledState() const
{
  Qt::CheckState enabledState = Qt::Unchecked;
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
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

  return enabledState;
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::useArrayIndexToggled(bool toggle)
{
  this->Internal->UseDataArray->setChecked(!toggle);
}

//-----------------------------------------------------------------------------
void pqLineChartDisplayPanel::useDataArrayToggled(bool toggle)
{
  this->Internal->UseArrayIndex->setChecked(!toggle);
}
