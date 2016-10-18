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
#include "pqColorAnnotationsPropertyWidget.h"
#include "ui_pqColorAnnotationsPropertyWidget.h"
#include "ui_pqSavePresetOptions.h"

#include "pqActiveObjects.h"
#include "pqChooseColorPresetReaction.h"
#include "pqDataRepresentation.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqUndoStack.h"
#include "vtkAbstractArray.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"
#include "vtkTuple.h"
#include "vtkVariant.h"

#include <vtk_jsoncpp.h>

#include <QAbstractTableModel>
#include <QColorDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QSet>
#include <algorithm>

namespace
{

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Decorator used to hide the widget when using IndexedLookup.
class pqColorAnnotationsPropertyWidgetDecorator : public pqPropertyWidgetDecorator
{
  typedef pqPropertyWidgetDecorator Superclass;
  bool IsAdvanced;

public:
  pqColorAnnotationsPropertyWidgetDecorator(vtkPVXMLElement* xmlArg, pqPropertyWidget* parentArg)
    : Superclass(xmlArg, parentArg)
    , IsAdvanced(false)
  {
  }
  virtual ~pqColorAnnotationsPropertyWidgetDecorator() {}

  void setIsAdvanced(bool val)
  {
    if (val != this->IsAdvanced)
    {
      this->IsAdvanced = val;
      emit this->visibilityChanged();
    }
  }
  virtual bool canShowWidget(bool show_advanced) const
  {
    return this->IsAdvanced ? show_advanced : true;
  }

private:
  Q_DISABLE_COPY(pqColorAnnotationsPropertyWidgetDecorator)
};

//-----------------------------------------------------------------------------
// QAbstractTableModel subclass for keeping track of the annotations.
// First column is IndexedColors and the 2nd and 3rd columns are the
// annotation value and text.
class pqAnnotationsModel : public QAbstractTableModel
{
  typedef QAbstractTableModel Superclass;

  struct ItemType
  {
    QColor Color;
    QPixmap Swatch;
    QString Value;
    QString Annotation;
    bool setData(int index, const QVariant& value)
    {
      if (index == 0 && value.canConvert(QVariant::Color))
      {
        this->Color = value.value<QColor>();
        this->Swatch = createSwatch();
        return true;
      }
      else if (index == 1)
      {
        this->Value = value.toString();
        return true;
      }
      else if (index == 2)
      {
        this->Annotation = value.toString();
        return true;
      }
      return false;
    }
    QVariant data(int index) const
    {
      if (index == 0 && this->Color.isValid())
      {
        return this->Swatch;
      }
      else if (index == 1)
      {
        return this->Value;
      }
      else if (index == 2)
      {
        return this->Annotation;
      }
      else if (index == 3 && this->Color.isValid())
      {
        return this->Color;
      }
      return QVariant();
    }
    QPixmap createSwatch()
    {
      int radius = 17;

      QPixmap pix(radius, radius);
      pix.fill(QColor(0, 0, 0, 0));

      QPainter painter(&pix);
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setBrush(QBrush(this->Color));
      painter.drawEllipse(1, 1, radius - 2, radius - 2);
      painter.end();
      return pix;
    }
  };

  QIcon MissingColorIcon;
  QVector<ItemType> Items;

public:
  pqAnnotationsModel(QObject* parentObject = 0)
    : Superclass(parentObject)
    , MissingColorIcon(":/pqWidgets/Icons/pqUnknownData16.png")
  {
  }
  virtual ~pqAnnotationsModel() {}

  /// Columns 1,2 are editable. 0 is not (since we show color swatch in 0). We
  /// hookup double-click event on the view to allow the user to edit the color.
  virtual Qt::ItemFlags flags(const QModelIndex& idx) const
  {
    return idx.column() > 0 ? this->Superclass::flags(idx) | Qt::ItemIsEditable
                            : this->Superclass::flags(idx);
  }

  virtual int rowCount(const QModelIndex& prnt = QModelIndex()) const
  {
    Q_UNUSED(prnt);
    return this->Items.size();
  }

  virtual int columnCount(const QModelIndex& /*parent*/) const { return 3; }

  virtual bool setData(const QModelIndex& idx, const QVariant& value, int role = Qt::EditRole)
  {
    Q_UNUSED(role);
    Q_ASSERT(idx.row() < this->rowCount());
    Q_ASSERT(idx.column() >= 0 && idx.column() < 3);
    if (this->Items[idx.row()].setData(idx.column(), value))
    {
      emit this->dataChanged(idx, idx);
      return true;
    }
    return false;
  }

  virtual QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const
  {
    if (role == Qt::DecorationRole || role == Qt::DisplayRole)
    {
      return this->Items[idx.row()].data(idx.column());
    }
    else if (role == Qt::EditRole)
    {
      return idx.column() == 0 ? this->Items[idx.row()].data(3)
                               : this->Items[idx.row()].data(idx.column());
    }
    else if (role == Qt::ToolTipRole || role == Qt::StatusTipRole)
    {
      switch (idx.column())
      {
        case 0:
          return "Color";
        case 1:
          return "Data Value";
        case 2:
          return "Annotation Text";
      }
    }
    return QVariant();
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const
  {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
      switch (section)
      {
        case 0:
          return "";
        case 1:
          return "Value";
        case 2:
          return "Annotation";
      }
    }
    return this->Superclass::headerData(section, orientation, role);
  }

  // Add a new annotation text-pair after the given index. Returns the inserted
  // index.
  QModelIndex addAnnotation(const QModelIndex& after = QModelIndex())
  {
    int row = after.isValid() ? after.row() : (this->rowCount(QModelIndex()) - 1);
    // insert after the current one.
    row++;

    emit this->beginInsertRows(QModelIndex(), row, row);
    this->Items.insert(row, ItemType());
    emit this->endInsertRows();
    return this->index(row, 0);
  }

  // Remove the given annotation indexes. Returns item before or after the removed
  // item, if any.
  QModelIndex removeAnnotations(const QModelIndexList& toRemove = QModelIndexList())
  {
    QSet<int> rowsToRemove;
    foreach (const QModelIndex& idx, toRemove)
    {
      rowsToRemove.insert(idx.row());
    }
    QList<int> rowsList = rowsToRemove.toList();
    for (int cc = (rowsList.size() - 1); cc >= 0; --cc)
    {
      emit this->beginRemoveRows(QModelIndex(), rowsList[cc], rowsList[cc]);
      this->Items.remove(rowsList[cc]);
      emit this->endRemoveRows();
    }
    if (rowsList.size() > 0 && rowsList.front() > this->Items.size())
    {
      return this->index(rowsList.front(), 0);
    }
    if (this->Items.size() > 0)
    {
      return this->index(this->Items.size() - 1, 0);
    }
    return QModelIndex();
  }

  void removeAllAnnotations()
  {
    emit this->beginResetModel();
    this->Items.clear();
    emit this->endResetModel();
  }

  void setAnnotations(const QVector<vtkTuple<QString, 2> >& new_annotations)
  {
    int old_size = this->Items.size();
    int new_size = new_annotations.size();
    if (old_size > new_size)
    {
      // rows are removed.
      emit this->beginRemoveRows(QModelIndex(), new_size, old_size - 1);
      this->Items.resize(new_size);
      emit this->endRemoveRows();
    }
    else if (new_size > old_size)
    {
      // rows are added.
      emit this->beginInsertRows(QModelIndex(), old_size, new_size - 1);
      this->Items.resize(new_size);
      emit this->endInsertRows();
    }
    Q_ASSERT(this->Items.size() == new_annotations.size());

    // now check for data changes.
    for (int cc = 0; cc < this->Items.size(); cc++)
    {
      if (this->Items[cc].Value != new_annotations[cc][0])
      {
        this->Items[cc].Value = new_annotations[cc][0];
        emit this->dataChanged(this->index(cc, 1), this->index(cc, 1));
      }
      if (this->Items[cc].Annotation != new_annotations[cc][1])
      {
        this->Items[cc].Annotation = new_annotations[cc][1];
        emit this->dataChanged(this->index(cc, 2), this->index(cc, 2));
      }
    }
  }
  QVector<vtkTuple<QString, 2> > annotations() const
  {
    QVector<vtkTuple<QString, 2> > theAnnotations(this->Items.size());
    int cc = 0;
    foreach (const ItemType& item, this->Items)
    {
      theAnnotations[cc].GetData()[0] = item.Value;
      theAnnotations[cc].GetData()[1] = item.Annotation;
      cc++;
    }
    return theAnnotations;
  }

  void setIndexedColors(const QVector<QColor>& new_colors)
  {
    int old_size = this->Items.size();
    int new_size = new_colors.size();
    if (old_size > new_size)
    {
      // rows are removed.
      emit this->beginRemoveRows(QModelIndex(), new_size, old_size - 1);
      this->Items.resize(new_size);
      emit this->endRemoveRows();
    }
    else if (new_size > old_size)
    {
      // rows are added.
      emit this->beginInsertRows(QModelIndex(), old_size, new_size - 1);
      this->Items.resize(new_size);
      emit this->endInsertRows();
    }
    Q_ASSERT(this->Items.size() == new_colors.size());

    // now check for data changes.
    for (int cc = 0; cc < this->Items.size(); cc++)
    {
      if (this->Items[cc].Color != new_colors[cc])
      {
        this->Items[cc].setData(0, new_colors[cc]);
        emit this->dataChanged(this->index(cc, 0), this->index(cc, 0));
      }
    }
  }
  QVector<QColor> indexedColors() const
  {
    QVector<QColor> icolors(this->Items.size());
    int cc = 0;
    foreach (const ItemType& item, this->Items)
    {
      icolors[cc] = item.Color;
      cc++;
    }
    return icolors;
  }

private:
  Q_DISABLE_COPY(pqAnnotationsModel)
};
}

//-----------------------------------------------------------------------------
class pqColorAnnotationsPropertyWidget::pqInternals
{
public:
  Ui::ColorAnnotationsPropertyWidget Ui;
  pqAnnotationsModel Model;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  QPointer<pqColorAnnotationsPropertyWidgetDecorator> Decorator;

  pqInternals(pqColorAnnotationsPropertyWidget* self)
  {
    this->Ui.setupUi(self);
    this->Ui.gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.verticalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.verticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

    this->Ui.AnnotationsTable->setModel(&this->Model);
    this->Ui.AnnotationsTable->horizontalHeader()->setHighlightSections(false);
#if QT_VERSION >= 0x050000
    this->Ui.AnnotationsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
#else
    this->Ui.AnnotationsTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
    this->Ui.AnnotationsTable->horizontalHeader()->setStretchLastSection(true);

    this->Decorator = new pqColorAnnotationsPropertyWidgetDecorator(NULL, self);
  }
};

//-----------------------------------------------------------------------------
pqColorAnnotationsPropertyWidget::pqColorAnnotationsPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals(this))
{
  Q_UNUSED(smgroup);

  this->addPropertyLink(
    this, "annotations", SIGNAL(annotationsChanged()), smproxy->GetProperty("Annotations"));

  this->addPropertyLink(
    this, "indexedColors", SIGNAL(indexedColorsChanged()), smproxy->GetProperty("IndexedColors"));

  // if proxy has a property named IndexedLookup, "Color" can be controlled only
  // when IndexedLookup is ON.
  if (smproxy->GetProperty("IndexedLookup"))
  {
    // we are not controlling the IndexedLookup property, we are merely
    // observing it to ensure the UI is updated correctly. Hence we don't fire
    // any signal to update the smproperty.
    this->Internals->VTKConnector->Connect(smproxy->GetProperty("IndexedLookup"),
      vtkCommand::ModifiedEvent, this, SLOT(updateIndexedLookupState()));
    this->updateIndexedLookupState();

    // Add decorator so the widget can be marked as advanced when IndexedLookup
    // is OFF.
    this->addDecorator(this->Internals->Decorator);
  }

  // Hookup UI buttons.
  Ui::ColorAnnotationsPropertyWidget& ui = this->Internals->Ui;
  QObject::connect(ui.Add, SIGNAL(clicked()), this, SLOT(addAnnotation()));
  QObject::connect(ui.AddActive, SIGNAL(clicked()), this, SLOT(addActiveAnnotations()));
  QObject::connect(ui.AddActiveFromVisible, SIGNAL(clicked()), this,
    SLOT(addActiveAnnotationsFromVisibleSources()));
  QObject::connect(ui.Remove, SIGNAL(clicked()), this, SLOT(removeAnnotation()));
  QObject::connect(ui.DeleteAll, SIGNAL(clicked()), this, SLOT(removeAllAnnotations()));
  QObject::connect(ui.ChoosePreset, SIGNAL(clicked()), this, SLOT(choosePreset()));
  QObject::connect(ui.SaveAsPreset, SIGNAL(clicked()), this, SLOT(saveAsPreset()));

  QObject::connect(&this->Internals->Model,
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
  QObject::connect(ui.AnnotationsTable, SIGNAL(doubleClicked(const QModelIndex&)), this,
    SLOT(onDoubleClicked(const QModelIndex&)));
  QObject::connect(ui.AnnotationsTable, SIGNAL(editPastLastRow()), this, SLOT(editPastLastRow()));
}

//-----------------------------------------------------------------------------
pqColorAnnotationsPropertyWidget::~pqColorAnnotationsPropertyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::updateIndexedLookupState()
{
  if (this->proxy()->GetProperty("IndexedLookup"))
  {
    bool val = vtkSMPropertyHelper(this->proxy(), "IndexedLookup").GetAsInt() != 0;
    this->Internals->Ui.AnnotationsTable->horizontalHeader()->setSectionHidden(0, !val);
    this->Internals->Ui.ChoosePreset->setVisible(val);
    this->Internals->Ui.SaveAsPreset->setVisible(val);
    this->Internals->Decorator->setIsAdvanced(!val);
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::onDataChanged(
  const QModelIndex& topleft, const QModelIndex& btmright)
{
  if (topleft.column() == 0)
  {
    emit this->indexedColorsChanged();
  }
  if (btmright.column() >= 1)
  {
    emit this->annotationsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::onDoubleClicked(const QModelIndex& idx)
{
  if (idx.column() == 0)
  {
    QColor color = this->Internals->Model.data(idx, Qt::EditRole).value<QColor>();
    color = QColorDialog::getColor(
      color, this, "Choose Annotation Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
      this->Internals->Model.setData(idx, color);
    }
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::annotations() const
{
  const QVector<vtkTuple<QString, 2> >& value = this->Internals->Model.annotations();

  QList<QVariant> reply;
  for (int cc = 0; cc < value.size(); cc++)
  {
    reply.push_back(value[cc].GetData()[0]);
    reply.push_back(value[cc].GetData()[1]);
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setAnnotations(const QList<QVariant>& value)
{
  QVector<vtkTuple<QString, 2> > annotationsData;
  annotationsData.resize(value.size() / 2);

  for (int cc = 0; (cc + 1) < value.size(); cc += 2)
  {
    annotationsData[cc / 2].GetData()[0] = value[cc].toString();
    annotationsData[cc / 2].GetData()[1] = value[cc + 1].toString();
  }

  this->Internals->Model.setAnnotations(annotationsData);

  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::indexedColors() const
{
  QList<QVariant> reply;
  QVector<QColor> colors = this->Internals->Model.indexedColors();
  foreach (const QColor& color, colors)
  {
    reply.push_back(color.redF());
    reply.push_back(color.greenF());
    reply.push_back(color.blueF());
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setIndexedColors(const QList<QVariant>& value)
{
  QVector<QColor> colors;
  colors.resize(value.size() / 3);

  for (int cc = 0; (cc + 2) < value.size(); cc += 3)
  {
    QColor color;
    color.setRgbF(value[cc].toDouble(), value[cc + 1].toDouble(), value[cc + 2].toDouble());
    colors[cc / 3] = color;
  }

  this->Internals->Model.setIndexedColors(colors);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::addAnnotation()
{
  QModelIndex idx =
    this->Internals->Model.addAnnotation(this->Internals->Ui.AnnotationsTable->currentIndex());
  this->Internals->Ui.AnnotationsTable->setCurrentIndex(idx);
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::editPastLastRow()
{
  this->Internals->Model.addAnnotation(this->Internals->Ui.AnnotationsTable->currentIndex());
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::removeAnnotation()
{
  QModelIndexList indexes =
    this->Internals->Ui.AnnotationsTable->selectionModel()->selectedIndexes();
  if (indexes.size() == 0)
  {
    // Nothing selected. Nothing to remove
    return;
  }
  QModelIndex idx = this->Internals->Model.removeAnnotations(indexes);
  this->Internals->Ui.AnnotationsTable->setCurrentIndex(idx);
  emit this->annotationsChanged();
}

namespace
{

//-----------------------------------------------------------------------------
// Given a list of existing annotations and a list of potentially new
// annotations, merge the lists. The candidate annotations are first
// selected to fill in empty annotation values in the existing
// annotations list, then they are added to the end.
//
// mergedAnnotations - interleaved annotation array (value/label)
// existingAnnotations - interleaved annotation array (value/label)
// candidateValues - candidate array of just annotation values, possibly new
void MergeAnnotations(QList<QVariant>& mergedAnnotations,
  const QList<QVariant>& existingAnnotations, const QList<QVariant>& candidateValues)
{
  // Extract values from exisiting interleaved annotation list.
  QList<QVariant> existingValues;
  for (int idx = 0; idx < existingAnnotations.size() / 2; ++idx)
  {
    existingValues.push_back(existingAnnotations[2 * idx]);
  }

  // Subset candidate annotations to only those not in existing annotations
  // Candidate values
  QList<QVariant> newCandidateValues;
  for (int idx = 0; idx < candidateValues.size(); ++idx)
  {
    if (!existingValues.contains(candidateValues[idx]))
    {
      newCandidateValues.push_back(candidateValues[idx]);
    }
  }

  // Iterate over existing annotations, backfilling new candidates
  // in empty slots where possible.
  int candidateIdx = 0;
  for (int idx = 0; idx < existingValues.size(); ++idx)
  {
    if (existingValues[idx] == "" && candidateIdx < newCandidateValues.size())
    {
      mergedAnnotations.push_back(newCandidateValues[candidateIdx]); // value
      mergedAnnotations.push_back(newCandidateValues[candidateIdx]); // annotation
      ++candidateIdx;
    }
    else
    {
      mergedAnnotations.push_back(existingAnnotations[2 * idx + 0]); // value
      mergedAnnotations.push_back(existingAnnotations[2 * idx + 1]); // annotation
    }
  }

  // Add any left over candidates
  for (; candidateIdx < newCandidateValues.size(); ++candidateIdx)
  {
    mergedAnnotations.push_back(newCandidateValues[candidateIdx]); // value
    mergedAnnotations.push_back(newCandidateValues[candidateIdx]); // annotation
  }
}

} // end anonymous namespace

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::addActiveAnnotations()
{
  try
  {
    // obtain prominent values from the server and add them
    pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
    if (!repr)
    {
      throw 0;
    }

    vtkPVProminentValuesInformation* info =
      vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(repr->getProxy());
    if (!info)
    {
      throw 0;
    }

    int component_no = -1;
    if (QString("Component") ==
      vtkSMPropertyHelper(this->proxy(), "VectorMode", true).GetAsString())
    {
      component_no = vtkSMPropertyHelper(this->proxy(), "VectorComponent").GetAsInt();
    }
    if (component_no == -1 && info->GetNumberOfComponents() == 1)
    {
      component_no = 0;
    }

    vtkSmartPointer<vtkAbstractArray> uniqueValues;
    uniqueValues.TakeReference(info->GetProminentComponentValues(component_no));
    if (uniqueValues == NULL)
    {
      throw 0;
    }

    QList<QVariant> existingAnnotations = this->annotations();

    QList<QVariant> candidateAnnotationValues;
    for (vtkIdType idx = 0; idx < uniqueValues->GetNumberOfTuples(); idx++)
    {
      candidateAnnotationValues.push_back(uniqueValues->GetVariantValue(idx).ToString().c_str());
    }

    // Combined annotation values (old and new)
    QList<QVariant> mergedAnnotations;
    MergeAnnotations(mergedAnnotations, existingAnnotations, candidateAnnotationValues);

    // Set the merged annotations
    this->setAnnotations(mergedAnnotations);
  }
  catch (int)
  {
    QMessageBox::warning(this, "Couldn't determine discrete values",
      "Could not determine discrete values using the data produced by the "
      "current source/filter. Please add annotations manually.",
      QMessageBox::Ok);
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::addActiveAnnotationsFromVisibleSources()
{
  try
  {
    pqServer* server = pqActiveObjects::instance().activeServer();
    if (!server)
    {
      throw 0;
    }

    // obtain prominent values from all visible sources colored by the same
    // array name as the name of color array for the active representation.
    pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
    if (!repr)
    {
      throw 0;
    }

    vtkSMPVRepresentationProxy* activeRepresentationProxy =
      vtkSMPVRepresentationProxy::SafeDownCast(repr->getProxy());
    if (!activeRepresentationProxy)
    {
      throw 0;
    }

    vtkPVArrayInformation* activeArrayInfo =
      vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(activeRepresentationProxy);

    vtkSMSessionProxyManager* pxm = server->proxyManager();

    // Iterate over representations, collecting prominent values from each.
    QSet<QString> uniqueAnnotations;
    vtkSmartPointer<vtkCollection> collection = vtkSmartPointer<vtkCollection>::New();
    pxm->GetProxies("representations", collection);
    for (int i = 0; i < collection->GetNumberOfItems(); ++i)
    {
      vtkSMProxy* representationProxy = vtkSMProxy::SafeDownCast(collection->GetItemAsObject(i));
      if (!representationProxy ||
        !vtkSMPropertyHelper(representationProxy, "Visibility").GetAsInt())
      {
        continue;
      }

      vtkPVArrayInformation* currentArrayInfo =
        vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(representationProxy);
      if (!activeArrayInfo || !activeArrayInfo->GetName() || !currentArrayInfo ||
        !currentArrayInfo->GetName() ||
        strcmp(activeArrayInfo->GetName(), currentArrayInfo->GetName()))
      {
        continue;
      }

      vtkPVProminentValuesInformation* info =
        vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(representationProxy);
      if (!info)
      {
        continue;
      }

      int component_no = -1;
      if (QString("Component") ==
        vtkSMPropertyHelper(this->proxy(), "VectorMode", true).GetAsString())
      {
        component_no = vtkSMPropertyHelper(this->proxy(), "VectorComponent").GetAsInt();
      }
      if (component_no == -1 && info->GetNumberOfComponents() == 1)
      {
        component_no = 0;
      }

      vtkSmartPointer<vtkAbstractArray> uniqueValues;
      uniqueValues.TakeReference(info->GetProminentComponentValues(component_no));
      if (uniqueValues == NULL)
      {
        continue;
      }
      for (vtkIdType idx = 0; idx < uniqueValues->GetNumberOfTuples(); idx++)
      {
        QString value(uniqueValues->GetVariantValue(idx).ToString().c_str());
        uniqueAnnotations.insert(value);
      }
    }

    QList<QString> uniqueList = uniqueAnnotations.values();
    qSort(uniqueList);

    QList<QVariant> existingAnnotations = this->annotations();

    QList<QVariant> candidateAnnotationValues;
    for (int idx = 0; idx < uniqueList.size(); idx++)
    {
      candidateAnnotationValues.push_back(uniqueList[idx]);
    }

    // Combined annotation values (old and new)
    QList<QVariant> mergedAnnotations;
    MergeAnnotations(mergedAnnotations, existingAnnotations, candidateAnnotationValues);

    this->setAnnotations(mergedAnnotations);
  }
  catch (int)
  {
    QMessageBox::warning(this, "Couldn't determine discrete values",
      "Could not determine discrete values using the data produced by the "
      "current source/filter. Please add annotations manually.",
      QMessageBox::Ok);
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::removeAllAnnotations()
{
  this->Internals->Model.removeAllAnnotations();
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::choosePreset(const char* presetName)
{
  QAction* tmp = new QAction(NULL);
  pqChooseColorPresetReaction* ccpr = new pqChooseColorPresetReaction(tmp, false);
  ccpr->setTransferFunction(this->proxy());
  this->connect(ccpr, SIGNAL(presetApplied()), SIGNAL(changeFinished()));
  ccpr->choosePreset(presetName);
  delete ccpr;
  delete tmp;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::saveAsPreset()
{
  QDialog dialog(this);
  Ui::SavePresetOptions ui;
  ui.setupUi(&dialog);
  ui.saveOpacities->setVisible(false);
  ui.saveAnnotations->setEnabled(
    vtkSMPropertyHelper(this->proxy(), "Annotations", true).GetNumberOfElements() > 0);

  // For now, let's not provide an option to not save colors. We'll need to fix
  // the pqPresetToPixmap to support rendering only opacities.
  ui.saveColors->setChecked(true);
  ui.saveColors->setEnabled(false);
  ui.saveColors->hide();

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  Json::Value cpreset = vtkSMTransferFunctionProxy::GetStateAsPreset(this->proxy());
  if (!ui.saveAnnotations->isChecked())
  {
    cpreset.removeMember("Annotations");
  }
  vtkStdString presetName;
  if (!cpreset.isNull())
  {
    // This scoping is necessary to ensure that the vtkSMTransferFunctionPresets
    // saves the new preset to the "settings" before the choosePreset dialog is
    // shown.
    vtkNew<vtkSMTransferFunctionPresets> presets;
    presetName = presets->AddUniquePreset(cpreset);
  }
  this->choosePreset(presetName);
}
