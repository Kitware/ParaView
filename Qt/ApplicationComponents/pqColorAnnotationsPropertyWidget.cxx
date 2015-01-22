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

#include "pqActiveObjects.h"
#include "pqColorMapModel.h"
#include "pqColorPresetManager.h"
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
#include "vtkSmartPointer.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkTuple.h"
#include "vtkVariant.h"

#include <QAbstractTableModel>
#include <QColorDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointer>
#include <QPointer>
#include <QSet>


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
  pqColorAnnotationsPropertyWidgetDecorator(
    vtkPVXMLElement* xmlArg, pqPropertyWidget* parentArg)
    : Superclass(xmlArg, parentArg), IsAdvanced(false)
    {
    }
  virtual ~pqColorAnnotationsPropertyWidgetDecorator()
    {
    }

  void setIsAdvanced(bool val)
    {
    if (val != this->IsAdvanced) { this->IsAdvanced = val; emit this->visibilityChanged(); }
    }
  virtual bool canShowWidget(bool show_advanced) const
    {
    return this->IsAdvanced? show_advanced : true;
    }

private:
  Q_DISABLE_COPY(pqColorAnnotationsPropertyWidgetDecorator);
};

//-----------------------------------------------------------------------------
// QAbstractTableModel subclass for keeping track of the annotations.
// First column is IndexedColors and the 2nd and 3rd columns are the
// annotation value and text.
class pqAnnotationsModel : public QAbstractTableModel
  {
  typedef QAbstractTableModel Superclass;

  QIcon MissingColorIcon;
  QVector<vtkTuple<QString, 2> > Annotations;
  QVector<QColor> IndexedColors;
public:
  pqAnnotationsModel(QObject* parentObject = 0):
    Superclass(parentObject),
    MissingColorIcon(":/pqWidgets/Icons/pqUnknownData16.png")
  {
  vtkTuple<QString, 2> item;
  item.GetData()[0] = QString("12.0");
  item.GetData()[1] = QString("Label 1");
  this->Annotations.push_back(item);

  item.GetData()[0] = QString("13.0");
  item.GetData()[1] = QString("Label 2");
  this->Annotations.push_back(item);
  }
  virtual ~pqAnnotationsModel()
    {
    }

  /// Columns 1,2 are editable. 0 is not (since we show color swatch in 0). We
  /// hookup double-click event on the view to allow the user to edit the color.
  virtual Qt::ItemFlags flags(const QModelIndex &idx) const
    {
    return idx.column() > 0?
      this->Superclass::flags(idx) | Qt::ItemIsEditable:
      this->Superclass::flags(idx);
    }

  virtual bool setData(const QModelIndex &idx, const QVariant &value,
    int role=Qt::EditRole)
    {
    Q_UNUSED(role);

    if (idx.column() == 0)
      {
      if (value.canConvert(QVariant::Color))
        {
        if (this->IndexedColors.size() <= idx.row())
          {
          this->IndexedColors.resize(idx.row()+1);
          }
        this->IndexedColors[idx.row()] = value.value<QColor>();
        emit this->dataChanged(idx, idx);
        return true;
        }
      }
    else
      {
      if (!value.toString().isEmpty())
        {
        this->Annotations[idx.row()].GetData()[idx.column()-1] = value.toString();
        emit this->dataChanged(idx, idx);
        return true;
        }
      }
    return false;
    }

  virtual int rowCount(const QModelIndex &prnt=QModelIndex()) const
    {
    Q_UNUSED(prnt);
    return this->Annotations.size();
    }

  virtual int columnCount(const QModelIndex &/*parent*/) const
    {
    return 3;
    }

  virtual QVariant data(const QModelIndex& idx, int role=Qt::DisplayRole) const
    {
    if (role == Qt::DecorationRole)
      {
      if (idx.column()==0)
        {
        if (idx.row() < this->IndexedColors.size())
          {
          return this->IndexedColors[idx.row()];
          }
        else
          {
          return this->MissingColorIcon;
          }
        }
      }
    else if (role == Qt::DisplayRole || role == Qt::EditRole)
      {
      switch (idx.column())
        {
      case 1:
        return this->Annotations[idx.row()].GetData()[0];
      case 2:
        return this->Annotations[idx.row()].GetData()[1];
        }
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
        return ""; // Color
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
  QModelIndex addAnnotation(const QModelIndex& after=QModelIndex())
    {
    int row = after.isValid()? after.row() : (this->rowCount(QModelIndex())-1);

    vtkTuple<QString, 2> new_annotation;
    if (row >= 0  && row < this->Annotations.size())
      {
      new_annotation = this->Annotations[row];
      }
    else
      {
      new_annotation.GetData()[0] = "Value";
      new_annotation.GetData()[1] = "Text";
      }

    // insert after the current one.
    row++;

    emit this->beginInsertRows(QModelIndex(), row, row);
    this->Annotations.insert(row, new_annotation);
    emit this->endInsertRows();

    return this->index(row, 0);
    }

  // Given a list of modelindexes, return a vector containing multiple sorted
  // vectors of rows, split by their discontinuity
  void splitSelectedIndexesToRowRanges(
    const QModelIndexList& indexList,
    QVector<QVector<QVariant> > &result)
    {
    if (indexList.empty())
      {
      return;
      }
    result.clear();
    QVector <int> rows;
    QModelIndexList::const_iterator iter = indexList.begin();
    for ( ; iter != indexList.end(); ++iter)
      {
      if ((*iter).isValid())
        {
        rows.push_back((*iter).row());
        }
      }
    qSort(rows.begin(), rows.end());
    result.resize(1);
    result[0].push_back(rows[0]);
    for (int i = 1; i < rows.size(); ++i)
      {
      if (rows[i] == rows[i-1])
        {
        // avoid duplicate
        continue;
        }
      if (rows[i] != rows[i-1] + 1)
        {
        result.push_back(QVector<QVariant>());
        }
      result.back().push_back(QVariant(rows[i]));
      }
    }

  // Remove the given annotation indexes. Returns item before or after the removed
  // item, if any.
  QModelIndex removeAnnotations(const QModelIndexList& toRemove=QModelIndexList())
    {
    QVector< QVector<QVariant> > rowRanges;
    this->splitSelectedIndexesToRowRanges(toRemove, rowRanges);
    int numGroups = static_cast<int>(rowRanges.size());
    for (int g = numGroups-1; g > -1; --g)
      {
      int numRows = rowRanges.at(g).size();
      int beginRow = rowRanges.at(g).at(0).toInt();
      int endRow = rowRanges.at(g).at(numRows-1).toInt();
      emit this->beginRemoveRows(QModelIndex(), beginRow, endRow);
      for (int r = endRow; r >= beginRow; --r)
        {
        this->Annotations.remove(r);
        }
      emit this->endRemoveRows();
      }

    int firstRow = rowRanges.at(0).at(0).toInt();
    int rowsCount = this->rowCount();
    if (firstRow < rowsCount)
      {
      // since firstRow is still a valid row.
      return this->index(firstRow, toRemove.at(0).column());
      }
    else if (rowsCount > 0 && (firstRow > (rowsCount-1)))
      {
      // just return the index for last row.
      return this->index(rowsCount-1, toRemove.at(0).column());
      }

    return QModelIndex();
    }

  void removeAllAnnotations()
    {
    emit this->beginResetModel();
    this->Annotations.clear();
    emit this->endResetModel();
    }

  void setAnnotations(const QVector<vtkTuple<QString, 2> >& new_annotations)
    {
    int old_size = this->Annotations.size();
    int new_size = new_annotations.size();

    if (old_size > new_size)
      {
      // rows are removed.
      emit this->beginRemoveRows(QModelIndex(), new_size, old_size-1);
      this->Annotations.resize(new_size);
      emit this->endRemoveRows();
      }
    else if (new_size > old_size)
      {
      // rows are added.
      emit this->beginInsertRows(QModelIndex(), old_size, new_size-1);
      this->Annotations.resize(new_size);
      for (int cc=old_size; cc < new_size; cc++)
        {
        this->Annotations[cc] = new_annotations[cc];
        }
      emit this->endInsertRows();
      }

    Q_ASSERT(this->Annotations.size() == new_annotations.size());
    // now check for data changes.
    for (int cc=0; cc < this->Annotations.size(); cc++)
      {
      if (this->Annotations[cc] != new_annotations[cc])
        {
        this->Annotations[cc] = new_annotations[cc];
        emit this->dataChanged(this->index(cc, 1), this->index(cc, 2));
        }
      }
    }
  const QVector<vtkTuple<QString, 2> >& annotations() const
    {
    return this->Annotations;
    }

  void setIndexedColors(const QVector<QColor>& new_colors)
    {
    if (this->IndexedColors != new_colors)
      {
      this->IndexedColors = new_colors;
      emit this->dataChanged(
        this->index(0, 0), this->index(this->rowCount()-1, 0));
      }
    }
  const QVector<QColor>& indexedColors() const
    {
    return this->IndexedColors;
    }

private:
  Q_DISABLE_COPY(pqAnnotationsModel);
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
    this->Ui.AnnotationsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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
  : Superclass(smproxy, parentObject),
  Internals(new pqInternals(this))
{
  Q_UNUSED(smgroup);

  this->addPropertyLink(
    this, "annotations", SIGNAL(annotationsChanged()),
    smproxy->GetProperty("Annotations"));

  this->addPropertyLink(
    this, "indexedColors", SIGNAL(indexedColorsChanged()),
    smproxy->GetProperty("IndexedColors"));

  // if proxy has a property named IndexedLookup, "Color" can be controlled only
  // when IndexedLookup is ON.
  if (smproxy->GetProperty("IndexedLookup"))
    {
    // we are not controlling the IndexedLookup property, we are merely
    // observing it to ensure the UI is updated correctly. Hence we don't fire
    // any signal to update the smproperty.
    this->Internals->VTKConnector->Connect(
      smproxy->GetProperty("IndexedLookup"), vtkCommand::ModifiedEvent,
      this, SLOT(updateIndexedLookupState()));
    this->updateIndexedLookupState();

    // Add decorator so the widget can be marked as advanced when IndexedLookup
    // is OFF.
    this->addDecorator(this->Internals->Decorator);
    }

  // Hookup UI buttons.
  Ui::ColorAnnotationsPropertyWidget &ui = this->Internals->Ui;
  QObject::connect(ui.Add, SIGNAL(clicked()), this, SLOT(addAnnotation()));
  QObject::connect(ui.AddActive, SIGNAL(clicked()), this, SLOT(addActiveAnnotations()));
  QObject::connect(ui.AddActiveFromVisible, SIGNAL(clicked()),
                   this, SLOT(addActiveAnnotationsFromVisibleSources()));
  QObject::connect(ui.Remove, SIGNAL(clicked()), this, SLOT(removeAnnotation()));
  QObject::connect(ui.DeleteAll, SIGNAL(clicked()), this, SLOT(removeAllAnnotations()));
  QObject::connect(ui.ChoosePreset, SIGNAL(clicked()), this, SLOT(choosePreset()));
  QObject::connect(ui.SaveAsPreset, SIGNAL(clicked()), this, SLOT(saveAsPreset()));

  QObject::connect(&this->Internals->Model,
    SIGNAL(dataChanged(const QModelIndex &, const QModelIndex&)),
    this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
  QObject::connect(
    ui.AnnotationsTable, SIGNAL(doubleClicked(const QModelIndex&)),
    this, SLOT(onDoubleClicked(const QModelIndex&)));
  QObject::connect(
    ui.AnnotationsTable, SIGNAL(editPastLastRow()),
    this, SLOT(editPastLastRow()));
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
    this->Internals->Ui.AnnotationsTable->horizontalHeader()->setSectionHidden(0,
      !val);
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
    QColor color = this->Internals->Model.data(idx,
      Qt::DecorationRole).value<QColor>();
    color = QColorDialog::getColor(color, this, "Choose Annotation Color",
      QColorDialog::DontUseNativeDialog);
    if (color.isValid())
      {
      this->Internals->Model.setData(idx, color);
      }
    }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::annotations() const
{
  const QVector<vtkTuple<QString, 2> > &value =
    this->Internals->Model.annotations();

  QList<QVariant> reply;
  for (int cc=0; cc < value.size(); cc++)
    {
    reply.push_back(value[cc].GetData()[0]);
    reply.push_back(value[cc].GetData()[1]);
    }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setAnnotations(
  const QList<QVariant>& value)
{
  QVector<vtkTuple<QString, 2> > annotationsData;
  annotationsData.resize(value.size()/2);

  for (int cc=0; (cc+1) < value.size(); cc+=2)
    {
    annotationsData[cc/2].GetData()[0] = value[cc].toString();
    annotationsData[cc/2].GetData()[1] = value[cc+1].toString();
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
void pqColorAnnotationsPropertyWidget::setIndexedColors(
  const QList<QVariant>& value)
{
  QVector<QColor> colors;
  colors.resize(value.size()/3);

  for (int cc=0; (cc+2) < value.size(); cc+=3)
    {
    QColor color;
    color.setRgbF(value[cc].toDouble(),
                  value[cc+1].toDouble(),
                  value[cc+2].toDouble());
    colors[cc/3] = color;
    }

  this->Internals->Model.setIndexedColors(colors);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::addAnnotation()
{
  QModelIndex idx = this->Internals->Model.addAnnotation(
    this->Internals->Ui.AnnotationsTable->currentIndex());
  this->Internals->Ui.AnnotationsTable->setCurrentIndex(idx);
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::editPastLastRow()
{
  this->Internals->Model.addAnnotation(
    this->Internals->Ui.AnnotationsTable->currentIndex());
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::removeAnnotation()
{
  QModelIndexList indexes =
    this->Internals->Ui.AnnotationsTable->selectionModel()->selectedIndexes();
  if( indexes.size() == 0 )
    {
    // Nothing selected. Nothing to remove
    return;
    }
  QModelIndex idx = this->Internals->Model.removeAnnotations(indexes);
  this->Internals->Ui.AnnotationsTable->setCurrentIndex(idx);
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::addActiveAnnotations()
{
  try
    {
    // obtain prominent values from the server and add them
    pqDataRepresentation* repr =
      pqActiveObjects::instance().activeRepresentation();
    if (!repr)
      {
      throw 0;
      }

    vtkPVProminentValuesInformation* info =
      vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(
        repr->getProxy());
    if (!info)
      {
      throw 0;
      }

    int component_no = -1;
    if (QString("Component") ==
      vtkSMPropertyHelper(this->proxy(), "VectorMode", true).GetAsString())
      {
      component_no = vtkSMPropertyHelper(this->proxy(),
        "VectorComponent").GetAsInt();
      }
    if (component_no == -1 && info->GetNumberOfComponents() == 1)
      {
      component_no = 0;
      }

    QList<QVariant> annotationList;
    vtkSmartPointer<vtkAbstractArray> unique_values;
    unique_values.TakeReference(
      info->GetProminentComponentValues(component_no));
    if (unique_values == NULL)
      {
      throw 0;
      }
    for (vtkIdType idx=0; idx < unique_values->GetNumberOfTuples(); idx++)
      {
      annotationList.push_back(unique_values->GetVariantValue(idx).ToString().c_str());
      annotationList.push_back(unique_values->GetVariantValue(idx).ToString().c_str());
      }
    this->setAnnotations(annotationList);
    }

  catch (int)
    {
    QMessageBox::warning(
      this, "Couldn't determine discrete values",
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
    pqServer* server =
      pqActiveObjects::instance().activeServer();
    if (!server)
      {
      throw 0;
      }

    // obtain prominent values from all visible sources colored by the same
    // array name as the name of color array for the active representation.
    pqDataRepresentation* repr =
      pqActiveObjects::instance().activeRepresentation();
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
      vtkSMProxy* representationProxy =
        vtkSMProxy::SafeDownCast(collection->GetItemAsObject(i));
      if (!representationProxy || !vtkSMPropertyHelper(representationProxy, "Visibility").GetAsInt())
        {
        continue;
        }

      vtkPVArrayInformation* currentArrayInfo =
        vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(representationProxy);
      if (!activeArrayInfo || !activeArrayInfo->GetName() ||
          !currentArrayInfo || !currentArrayInfo->GetName() ||
          strcmp(activeArrayInfo->GetName(), currentArrayInfo->GetName()))
        {
        continue;
        }

      vtkPVProminentValuesInformation* info =
        vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(
          representationProxy);
      if (!info)
        {
        continue;
        }

      int component_no = -1;
      if (QString("Component") ==
          vtkSMPropertyHelper(this->proxy(), "VectorMode", true).GetAsString())
        {
        component_no = vtkSMPropertyHelper(this->proxy(),
          "VectorComponent").GetAsInt();
        }
      if (component_no == -1 && info->GetNumberOfComponents() == 1)
        {
        component_no = 0;
        }

      vtkSmartPointer<vtkAbstractArray> unique_values;
      unique_values.TakeReference(
        info->GetProminentComponentValues(component_no));
      if (unique_values == NULL)
        {
        continue;
        }
      for (vtkIdType idx=0; idx < unique_values->GetNumberOfTuples(); idx++)
        {
        uniqueAnnotations.insert(QString(unique_values->GetVariantValue(idx).ToString().c_str()));
        }
      }

    QList<QString> uniqueList = uniqueAnnotations.values();
    qSort(uniqueList);

    QList<QVariant> annotationList;
    for (int idx = 0; idx < uniqueList.size(); ++idx)
      {
      annotationList.push_back(uniqueList[idx]);
      annotationList.push_back(uniqueList[idx]);
      }

    this->setAnnotations(annotationList);
    }

  catch (int)
    {
    QMessageBox::warning(
      this, "Couldn't determine discrete values",
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
void pqColorAnnotationsPropertyWidget::choosePreset(const pqColorMapModel* add_new/*=NULL*/)
{
  pqColorPresetManager preset(this,
    pqColorPresetManager::SHOW_INDEXED_COLORS_ONLY);
  preset.setUsingCloseButton(true);
  preset.loadBuiltinColorPresets();
  preset.restoreSettings();
  if (add_new)
    {
    preset.addColorMap(*add_new, "New Color Preset");
    }

  QObject::connect(&preset, SIGNAL(currentChanged(const pqColorMapModel*)),
    this, SLOT(applyPreset(const pqColorMapModel*)));
  preset.exec();
  preset.saveSettings(); 
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::applyPreset(const pqColorMapModel* preset)
{
  vtkNew<vtkPVXMLElement> xml;
  xml->SetName("ColorMap");
  if (pqColorPresetManager::saveColorMapToXML(preset, xml.GetPointer()))
    {
    BEGIN_UNDO_SET("Apply color preset");
    vtkSMTransferFunctionProxy::ApplyColorMap(this->proxy(), xml.GetPointer());
    emit this->changeFinished();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::saveAsPreset()
{
  vtkNew<vtkPVXMLElement> xml;
  if (vtkSMTransferFunctionProxy::SaveColorMap(this->proxy(), xml.GetPointer()))
    {
    pqColorMapModel colorMap =
      pqColorPresetManager::createColorMapFromXML(xml.GetPointer());
    this->choosePreset(&colorMap);
    }
}
