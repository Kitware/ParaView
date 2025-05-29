// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqKeyFrameEditor.h"
#include "ui_pqKeyFrameEditor.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLineEdit>
#include <QPointer>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QVector3D>

#include <vtkCamera.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMTrace.h>
#include <vtk_jsoncpp.h>
#include <vtksys/FStream.hxx>

#include "pqActiveObjects.h"
#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqCameraKeyFrameWidget.h"
#include "pqFileDialog.h"
#include "pqKeyFrameTypeWidget.h"
#include "pqOrbitCreatorDialog.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptorKeyFrameType.h"
#include "pqUndoStack.h"

#include <algorithm>

//-----------------------------------------------------------------------------
// editor dialog that comes and goes for editing a single key frame
// interpolation type
pqKeyFrameEditorDialog::pqKeyFrameEditorDialog(QWidget* p, QWidget* child)
  : QDialog(p)
{
  this->Child = child;
  QObject::connect(this, &QWidget::close, this, &QObject::deleteLater);
  this->setWindowModality(Qt::WindowModal);
  this->setWindowTitle(tr("Key Frame Interpolation"));
  this->setModal(false);
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(this->Child, 0);
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

  l->addStretch();
  l->addWidget(buttons);
  this->Child->show();
}

//-----------------------------------------------------------------------------
pqKeyFrameEditorDialog::~pqKeyFrameEditorDialog()
{
  // disconnect child
  this->Child->setParent(nullptr);
  this->Child->hide();
}

class pqKeyFrameEditorWidget : public QWidget
{
public:
  pqKeyFrameEditorWidget(QWidget* p, QWidget* child)
    : QWidget(p)
    , Child(child)
  {
    QVBoxLayout* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(this->Child);
    this->Child->show();
  }
  ~pqKeyFrameEditorWidget() override
  {
    this->Child->setParent(nullptr);
    this->Child->hide();
  }

private:
  QWidget* Child;
};

//-----------------------------------------------------------------------------
// model model for key frame values
class pqKeyFrameItem
  : public QObject
  , public QStandardItem
{
public:
  // return an editor for the item
  virtual QWidget* editorWidget() { return nullptr; }
  // return an editor for a dialog
  virtual QWidget* editorDialog() { return nullptr; }
};

//-----------------------------------------------------------------------------
// model item for putting a key frame interpolation widget in the model
class pqKeyFrameInterpolationItem : public pqKeyFrameItem
{
public:
  pqKeyFrameInterpolationItem() = default;
  // get data from combo box on key frame editor
  QVariant data(int role) const override
  {
    int idx = this->Widget.typeComboBox()->currentIndex();
    QAbstractItemModel* comboModel = this->Widget.typeComboBox()->model();
    return comboModel->data(comboModel->index(idx, 0), role);
  }
  QWidget* editorDialog() override { return &Widget; }
  pqKeyFrameTypeWidget Widget;
};

//-----------------------------------------------------------------------------
// model item for putting a key frame interpolation widget in the model
class pqCameraKeyFrameItem : public pqKeyFrameItem
{
public:
  pqCameraKeyFrameItem()
    : CamWidget(&this->Widget)
  {
    QVBoxLayout* l = new QVBoxLayout(&this->Widget);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(&this->CamWidget);
  }

  // get data from widget
  QVariant data(int role) const override
  {
    if (role == Qt::DisplayRole)
    {
      if (this->CamWidget.usePathBasedMode())
      {
        return "Path ...";
      }
      else
      {
        return "Position ...";
        // QList<QVariant> pos = this->CamWidget.position();
        // return QString("Position(%1,%2,%3), ...").
        //  arg(pos[0].toString()).arg(pos[1].toString()).arg(pos[2].toString());
      }
    }
    return QVariant();
  }
  QWidget* editorDialog() override { return &Widget; }
  QWidget Widget;
  pqCameraKeyFrameWidget CamWidget;
};

//-----------------------------------------------------------------------------
// delegate for creating editors for model items
class pqKeyFrameEditorDelegate : public QItemDelegate
{
public:
  pqKeyFrameEditorDelegate(QObject* p)
    : QItemDelegate(p)
    , CameraMode(false)
  {
  }
  QWidget* createEditor(
    QWidget* p, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(index.model());

    if (index.column() == 0)
    {
      return new QLineEdit(p);
    }

    QStandardItem* item = model->item(index.row(), index.column());
    pqKeyFrameItem* kfitem = static_cast<pqKeyFrameItem*>(item);
    if (item && kfitem->editorWidget())
    {
      return new pqKeyFrameEditorWidget(p, kfitem->editorWidget());
    }
    else if (item && kfitem->editorDialog())
    {
      return new pqKeyFrameEditorDialog(p, kfitem->editorDialog());
    }
    else if (item)
    {
      return new QLineEdit(p);
    }
    return nullptr;
  }
  void updateEditorGeometry(
    QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    if (qobject_cast<pqKeyFrameEditorDialog*>(editor))
    {
      int w = 300;
      int h = 100;
      QWidget* p = editor->parentWidget();
      QRect prect = p->geometry();
      QRect newRect(p->mapToGlobal(prect.center()), QSize(w, h));
      newRect.adjust(-w / 2, -h, -w / 2, -h);
      editor->setGeometry(newRect);
    }
    else
    {
      QItemDelegate::updateEditorGeometry(editor, option, index);
    }
  }

  bool CameraMode;
};

//-----------------------------------------------------------------------------
class pqKeyFrameEditor::pqInternal
{
public:
  pqInternal(pqKeyFrameEditor* editor)
    : Editor(editor)
  {
  }
  pqKeyFrameEditor* const Editor;
  Ui::pqKeyFrameEditor Ui;
  QPointer<pqAnimationCue> Cue;
  QPointer<pqAnimationScene> Scene;
  QStandardItemModel Model;
  QPair<double, double> TimeRange;
  QPair<QVariant, QVariant> ValueRange;
  pqKeyFrameEditorDelegate* EditorDelegate;

  double normalizedTime(double t)
  {
    // zero span?
    double span = this->TimeRange.second - this->TimeRange.first;
    return (t - this->TimeRange.first) / span;
  }
  double realTime(double t)
  {
    double span = this->TimeRange.second - this->TimeRange.first;
    return t * span + this->TimeRange.first;
  }

  QList<QStandardItem*> newRow(int row, bool lastItem = false)
  {
    QList<QStandardItem*> items;
    items.append(this->newTimeItem(row));
    if (this->cameraPathCue() && lastItem)
    {
      return items;
    }

    if (this->cameraCue())
    {
      items.append(this->newCameraItem(row));
    }
    else
    {
      items.append(this->newInterpolationItem(row));
      items.append(this->newValueItem(row));
    }
    return items;
  }

  QStandardItem* newTimeItem(int row)
  {
    QStandardItem* item = new QStandardItem();
    int count = this->Model.rowCount();

    QVariant time = this->TimeRange.first;

    if (count == row && row != 0)
    {
      time = this->TimeRange.second;
    }
    else if (row > 0)
    {
      // average time between rows
      QModelIndex tidx = this->Model.index(row, 0);
      time = this->Model.data(tidx).toDouble();
      tidx = this->Model.index(row - 1, 0);
      time = time.toDouble() + this->Model.data(tidx).toDouble();
      time = time.toDouble() / 2.0;
    }
    item->setData(time, Qt::DisplayRole);
    return item;
  }
  pqKeyFrameInterpolationItem* newInterpolationItem(int row)
  {
    pqKeyFrameInterpolationItem* item = nullptr;
    int count = this->Model.rowCount();
    if (count != row || row == 0)
    {
      item = new pqKeyFrameInterpolationItem();
    }
    return item;
  }
  pqCameraKeyFrameItem* newCameraItem(int)
  {
    pqCameraKeyFrameItem* item = nullptr;
    item = new pqCameraKeyFrameItem();

    QObject::connect(&item->CamWidget, &pqCameraKeyFrameWidget::updateCurrentCamera, this->Editor,
      [=]() { this->Editor->updateCurrentCamera(item); });
    QObject::connect(&item->CamWidget, &pqCameraKeyFrameWidget::useCurrentCamera, this->Editor,
      [=]() { this->Editor->useCurrentCamera(item); });
    // default to current view
    this->Editor->useCurrentCamera(item);
    item->CamWidget.setUsePathBasedMode(this->cameraPathCue());
    return item;
  }
  QStandardItem* newValueItem(int row)
  {
    QStandardItem* item = new pqKeyFrameItem();
    int count = this->Model.rowCount();

    QVariant value = this->ValueRange.first;
    if (count == row && row != 0)
    {
      value = this->ValueRange.second;
    }
    else if (row > 0)
    {
      // average value between rows
      QModelIndex tidx = this->Model.index(row, 2);
      value = this->Model.data(tidx).toDouble();
      tidx = this->Model.index(row - 1, 2);
      value = value.toDouble() + this->Model.data(tidx).toDouble();
      value = value.toDouble() / 2.0;
    }
    item->setData(value, Qt::DisplayRole);
    return item;
  }

  bool cameraCue() { return QString("CameraAnimationCue") == this->Cue->getProxy()->GetXMLName(); }

  bool cameraPathCue()
  {
    return this->cameraCue() &&
      (pqSMAdaptor::getEnumerationProperty(this->Cue->getProxy()->GetProperty("Mode")) ==
        "Path-based");
  }
};

//-----------------------------------------------------------------------------
pqKeyFrameEditor::pqKeyFrameEditor(
  pqAnimationScene* scene, pqAnimationCue* cue, const QString& label, QWidget* p)
  : QWidget(p)
{
  this->Internal = new pqInternal(this);
  this->Internal->Ui.setupUi(this);
  this->Internal->Scene = scene;
  this->Internal->Cue = cue;
  this->Internal->TimeRange = scene ? scene->getClockTimeRange() : QPair<double, double>(0, 1);
  QList<QVariant> domain = pqSMAdaptor::getMultipleElementPropertyDomain(
    cue->getAnimatedProperty(), cue->getAnimatedPropertyIndex());
  if (this->Internal->Cue->getProxy()->GetXMLName() &&
    strcmp(this->Internal->Cue->getProxy()->GetXMLName(), "TimeAnimationCue") == 0)
  {
    this->Internal->ValueRange.first = this->Internal->TimeRange.first;
    this->Internal->ValueRange.second = this->Internal->TimeRange.second;
  }
  else if (!domain.empty())
  {
    this->Internal->ValueRange.first = domain[0];
    this->Internal->ValueRange.second = domain[1];
  }
  else
  {
    this->Internal->ValueRange.first = 0;
    this->Internal->ValueRange.second = 0;
  }

  this->Internal->Ui.tableView->setModel(&this->Internal->Model);
  this->Internal->Ui.tableView->horizontalHeader()->setStretchLastSection(true);

  this->Internal->EditorDelegate = new pqKeyFrameEditorDelegate(this->Internal->Ui.tableView);
  this->Internal->Ui.tableView->setItemDelegate(this->Internal->EditorDelegate);

  connect(this->Internal->Ui.pbNew, SIGNAL(clicked(bool)), this, SLOT(newKeyFrame()));
  connect(this->Internal->Ui.pbDelete, SIGNAL(clicked(bool)), this, SLOT(deleteKeyFrame()));
  connect(this->Internal->Ui.pbDeleteAll, SIGNAL(clicked(bool)), this, SLOT(deleteAllKeyFrames()));
  connect(
    this->Internal->Ui.pbCreateOrbit, SIGNAL(clicked(bool)), this, SLOT(createOrbitalKeyFrame()));
  connect(
    this->Internal->Ui.pbImportKeyFrames, SIGNAL(clicked(bool)), this, SLOT(importTrajectory()));
  connect(
    this->Internal->Ui.pbExportKeyFrames, SIGNAL(clicked(bool)), this, SLOT(exportTrajectory()));
  connect(this->Internal->Ui.pbUseCurrentCamera, SIGNAL(clicked(bool)), this,
    SLOT(useCurrentCameraForSelected()));
  connect(this->Internal->Ui.pbApplyToCamera, SIGNAL(clicked(bool)), this,
    SLOT(updateCurrentCameraWithSelected()));
  connect(this->Internal->Ui.pbUseSpline, SIGNAL(clicked(bool)), this, SLOT(updateSplineMode()));

  connect(this->Internal->Ui.tableView->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(updateButtons()));

  if (label != QString())
  {
    this->Internal->Ui.label->setText(label);
  }
  else
  {
    this->Internal->Ui.label->hide();
  }

  this->readKeyFrameData();

  bool pathBased = this->Internal->cameraPathCue();
  this->Internal->Ui.pbCameraModeStackedWidget->setVisible(this->Internal->cameraCue());
  this->Internal->Ui.pbCameraModeStackedWidget->setCurrentIndex(pathBased ? 1 : 0);

  this->updateButtons();

  QObject::connect(
    &this->Internal->Model, &QStandardItemModel::itemChanged, this, &pqKeyFrameEditor::modified);
}

//-----------------------------------------------------------------------------
pqKeyFrameEditor::~pqKeyFrameEditor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::setValuesOnly(bool vo)
{
  this->Internal->Ui.pbNew->setVisible(!vo);
  this->Internal->Ui.pbDelete->setVisible(!vo);
  this->Internal->Ui.pbDeleteAll->setVisible(!vo);
  this->Internal->Ui.tableView->setColumnHidden(0, vo);
}

//-----------------------------------------------------------------------------
bool pqKeyFrameEditor::valuesOnly() const
{
  return this->Internal->Ui.tableView->isColumnHidden(0);
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::readKeyFrameData()
{
  this->Internal->Model.clear();

  if (!this->Internal->Cue)
  {
    return;
  }

  int numberKeyFrames = this->Internal->Cue->getNumberOfKeyFrames();
  this->Internal->Model.setRowCount(numberKeyFrames);
  QStringList headerLabels;
  bool camera = this->Internal->cameraCue();
  this->Internal->EditorDelegate->CameraMode = camera;
  if (camera)
  {
    this->Internal->Model.setColumnCount(2);
    headerLabels << tr("Time");
    headerLabels << tr("Camera Values");
  }
  else
  {
    this->Internal->Model.setColumnCount(3);
    headerLabels << tr("Time");
    headerLabels << tr("Interpolation") << tr("Value");
  }

  // set the header data
  this->Internal->Model.setHorizontalHeaderLabels(headerLabels);

  for (int i = 0; i < numberKeyFrames; i++)
  {
    pqSMProxy keyFrame = this->Internal->Cue->getKeyFrame(i);

    QModelIndex idx = this->Internal->Model.index(i, 0);
    QVariant keyTime = pqSMAdaptor::getElementProperty(keyFrame->GetProperty("KeyTime"));
    double realKeyTime = this->Internal->realTime(keyTime.toDouble());
    this->Internal->Model.setData(idx, realKeyTime, Qt::DisplayRole);

    if (camera)
    {
      bool path_based = this->Internal->cameraPathCue();
      if ((i < numberKeyFrames - 1) || !path_based)
      {
        pqCameraKeyFrameItem* item = new pqCameraKeyFrameItem();
        QObject::connect(&item->CamWidget, &pqCameraKeyFrameWidget::updateCurrentCamera, this,
          [=]() { this->Internal->Editor->updateCurrentCamera(item); });
        QObject::connect(&item->CamWidget, &pqCameraKeyFrameWidget::useCurrentCamera, this,
          [=]() { this->Internal->Editor->useCurrentCamera(item); });
        item->CamWidget.setUsePathBasedMode(path_based);
        item->CamWidget.initializeUsingKeyFrame(keyFrame);
        this->Internal->Model.setItem(i, 1, item);
      }
    }
    else
    {
      if (i < numberKeyFrames - 1)
      {
        pqKeyFrameInterpolationItem* item = new pqKeyFrameInterpolationItem();
        this->Internal->Model.setItem(i, 1, item);

        // initialize gui with adaptor
        pqPropertyLinks links;
        pqSignalAdaptorKeyFrameType adaptor(&item->Widget, &links);
        adaptor.setKeyFrameProxy(keyFrame);
      }

      idx = this->Internal->Model.index(i, 2);
      pqKeyFrameItem* item = new pqKeyFrameItem();
      item->setData(
        pqSMAdaptor::getElementProperty(keyFrame->GetProperty("KeyValues")), Qt::DisplayRole);
      this->Internal->Model.setItem(i, 2, item);
    }
  }
}

static bool timeSort(const QPair<int, double>& a, const QPair<int, double>& b)
{
  return a.second < b.second;
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::writeKeyFrameData()
{
  if (!this->Internal->Cue)
  {
    return;
  }

  bool camera = this->Internal->cameraCue();

  int oldNumber = this->Internal->Cue->getNumberOfKeyFrames();
  int newNumber = this->Internal->Model.rowCount();

  BEGIN_UNDO_SET(tr("Edit Keyframes"));
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->Internal->Cue->getProxy());

  if (camera)
  {
    this->Internal->Cue->setKeyFrameType("CameraKeyFrame");
  }

  bool prev = this->Internal->Cue->blockSignals(true);
  for (int i = 0; i < oldNumber - newNumber; i++)
  {
    this->Internal->Cue->deleteKeyFrame(0);
  }
  for (int i = 0; i < newNumber - oldNumber; i++)
  {
    this->Internal->Cue->insertKeyFrame(0);
  }

  QList<QPair<int, double>> sortedKeyFrames;
  for (int i = 0; i < newNumber; i++)
  {
    QModelIndex idx = this->Internal->Model.index(i, 0);
    QVariant newData = this->Internal->Model.data(idx, Qt::DisplayRole);
    double nTime = this->Internal->normalizedTime(newData.toDouble());
    sortedKeyFrames.append(QPair<int, double>(i, nTime));
  }
  std::sort(sortedKeyFrames.begin(), sortedKeyFrames.end(), timeSort);

  for (int i = 0; i < newNumber; i++)
  {
    vtkSMProxy* keyFrame = this->Internal->Cue->getKeyFrame(i);
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", keyFrame);

    int j = sortedKeyFrames[i].first;

    QModelIndex idx = this->Internal->Model.index(j, 0);
    QVariant newData = this->Internal->Model.data(idx, Qt::DisplayRole);
    double nTime = this->Internal->normalizedTime(newData.toDouble());
    pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyTime"), nTime);

    if (camera)
    {
      pqCameraKeyFrameItem* item =
        static_cast<pqCameraKeyFrameItem*>(this->Internal->Model.item(j, 1));
      if (item)
      {
        item->CamWidget.saveToKeyFrame(keyFrame);
      }
    }
    else
    {
      pqKeyFrameInterpolationItem* item =
        static_cast<pqKeyFrameInterpolationItem*>(this->Internal->Model.item(j, 1));
      if (item)
      {
        pqSMAdaptor::setEnumerationProperty(keyFrame->GetProperty("Type"), item->Widget.type());
        pqSMAdaptor::setElementProperty(keyFrame->GetProperty("Base"), item->Widget.base());
        pqSMAdaptor::setElementProperty(
          keyFrame->GetProperty("StartPower"), item->Widget.startPower());
        pqSMAdaptor::setElementProperty(keyFrame->GetProperty("EndPower"), item->Widget.endPower());
        pqSMAdaptor::setElementProperty(keyFrame->GetProperty("Phase"), item->Widget.phase());
        pqSMAdaptor::setElementProperty(keyFrame->GetProperty("Offset"), item->Widget.offset());
        pqSMAdaptor::setElementProperty(
          keyFrame->GetProperty("Frequency"), item->Widget.frequency());
      }

      idx = this->Internal->Model.index(j, 2);
      newData = this->Internal->Model.data(idx, Qt::DisplayRole);
      pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyValues"), newData);
    }
    keyFrame->UpdateVTKObjects();
  }

  this->Internal->Cue->blockSignals(prev);
  this->Internal->Cue->triggerKeyFramesModified();
  if (auto sceneProxy = this->Internal->Scene->getProxy())
  {
    // Reload current timestep, force deleting outdated geometry cache
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("ForceDisableCaching", 1), 1);
    sceneProxy->UpdateProperty("ForceDisableCaching", 1);
    pqSMAdaptor::setElementProperty(
      sceneProxy->GetProperty("AnimationTime"), this->Internal->Scene->getAnimationTime());
    sceneProxy->UpdateProperty("AnimationTime", 1);
    sceneProxy->UpdateVTKObjects();
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("ForceDisableCaching", 1), 0);
    sceneProxy->UpdateProperty("ForceDisableCaching", 1);
  }

  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::newKeyFrame()
{
  // insert before selection, or insert 2nd to last
  int row = 0;
  int count = this->Internal->Model.rowCount();

  QModelIndex idx = this->Internal->Ui.tableView->selectionModel()->currentIndex();
  if (idx.isValid())
  {
    row = idx.row();
    // inserting at index 0 acts as insert at index 1
    if (count > 1 && row == 0)
    {
      row++;
    }
  }
  else if (count != 0)
  {
    row = count - 1;
  }

  auto newItem = this->Internal->newRow(row);
  this->Internal->Model.insertRow(row, newItem);

  // add one more
  if (count == 0)
  {
    this->Internal->Model.insertRow(1, this->Internal->newRow(1, true));
  }

  this->Internal->Ui.tableView->selectionModel()->setCurrentIndex(
    this->Internal->Model.indexFromItem(newItem.first()),
    QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
  this->updateButtons();
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::deleteKeyFrame()
{
  QModelIndex idx = this->Internal->Ui.tableView->selectionModel()->currentIndex();
  if (idx.isValid())
  {
    this->Internal->Model.removeRow(idx.row());
  }
  // if one row left, remove interpolation type
  if (this->Internal->Model.rowCount() == 1)
  {
    QStandardItem* item = this->Internal->Model.takeItem(0, 1);
    delete item;
  }

  this->updateButtons();
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::deleteAllKeyFrames()
{
  // remove all rows
  this->Internal->Model.removeRows(0, this->Internal->Model.rowCount());

  this->updateButtons();
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::updateButtons()
{
  QModelIndexList indexes = this->Internal->Ui.tableView->selectionModel()->selectedRows();
  QModelIndex index = this->Internal->Ui.tableView->selectionModel()->currentIndex();
  this->Internal->Ui.pbCreateOrbit->setEnabled(
    index.isValid() && index.row() < this->Internal->Ui.tableView->model()->rowCount() - 1);
  this->Internal->Ui.pbUseCurrentCamera->setEnabled(index.isValid());
  this->Internal->Ui.pbApplyToCamera->setEnabled(index.isValid());

  QStandardItem* item = this->Internal->Model.item(0, 1);
  pqCameraKeyFrameItem* cameraItem = dynamic_cast<pqCameraKeyFrameItem*>(item);
  if (cameraItem)
  {
    int useSpline =
      pqSMAdaptor::getElementProperty(this->Internal->Cue->getProxy()->GetProperty("Interpolation"))
        .toInt();
    this->Internal->Ui.pbUseSpline->setChecked(useSpline == 1);
  }
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::useCurrentCamera(QStandardItem* item)
{
  pqCameraKeyFrameItem* cameraItem = dynamic_cast<pqCameraKeyFrameItem*>(item);

  if (!cameraItem)
  {
    return;
  }

  vtkSMProxy* pxy = this->Internal->Cue->getAnimatedProxy();

  vtkSMRenderViewProxy* ren = vtkSMRenderViewProxy::SafeDownCast(pxy);
  ren->SynchronizeCameraProperties();
  cameraItem->CamWidget.initializeUsingCamera(ren->GetActiveCamera());
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::useCurrentCameraForSelected()
{
  QModelIndex index = this->Internal->Ui.tableView->selectionModel()->currentIndex();
  if (!index.isValid())
  {
    return;
  }

  const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(index.model());
  QStandardItem* item = model->item(index.row(), 1);

  this->useCurrentCamera(item);
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::updateCurrentCamera(QStandardItem* item)
{
  pqCameraKeyFrameItem* cameraItem = dynamic_cast<pqCameraKeyFrameItem*>(item);
  if (!cameraItem)
  {
    return;
  }

  vtkSMProxy* pxy = this->Internal->Cue->getAnimatedProxy();

  vtkSMRenderViewProxy* ren = vtkSMRenderViewProxy::SafeDownCast(pxy);
  ren->SynchronizeCameraProperties();
  cameraItem->CamWidget.applyToCamera(ren->GetActiveCamera());
  ren->StillRender();
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::updateCurrentCameraWithSelected()
{
  QModelIndex index = this->Internal->Ui.tableView->selectionModel()->currentIndex();
  if (!index.isValid())
  {
    return;
  }

  const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(index.model());
  QStandardItem* item = model->item(index.row(), 1);

  this->updateCurrentCamera(item);
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::createOrbitalKeyFrame()
{
  assert(this->Internal->cameraCue());
  QModelIndex index = this->Internal->Ui.tableView->selectionModel()->currentIndex();
  if (!index.isValid())
  {
    return;
  }

  const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(index.model());
  QStandardItem* item = model->item(index.row(), 1);
  pqCameraKeyFrameItem* camItem = dynamic_cast<pqCameraKeyFrameItem*>(item);
  if (!camItem)
  {
    qWarning("Selected row is not a camera item. Cannot create orbit.");
    return;
  }

  pqOrbitCreatorDialog creator(this);

  vtkSMProxy* pxy = this->Internal->Cue->getAnimatedProxy();
  vtkSMRenderViewProxy* ren = vtkSMRenderViewProxy::SafeDownCast(pxy);
  if (ren)
  {
    creator.setNormal(ren->GetActiveCamera()->GetViewUp());
    creator.setOrigin(ren->GetActiveCamera()->GetPosition());
    if (creator.exec() != QDialog::Accepted)
    {
      return;
    }
  }

  // default orbit has 7 points.
  QVariantList orbit = creator.orbitPoints(7);
  std::vector<double> pos(orbit.size());
  for (int i = 0; i < orbit.size(); i++)
  {
    pos[i] = orbit[i].toDouble();
  }
  camItem->CamWidget.setPositionPoints(pos);

  QVector3D normals = creator.normal();
  std::vector<double> norm{ normals.x(), normals.y(), normals.z() };
  camItem->CamWidget.setViewUp(norm.data());

  this->Internal->Cue->triggerKeyFramesModified();
  this->Internal->Cue->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::importTrajectory()
{
  QString filters =
    QString("ParaView keyframes configuration (*.pvkfc);;") + "All Files" + " (*.*)";

  pqFileDialog dialog(nullptr, this, tr("Load Custom KeyFrames Configuration"), "", filters, false);
  dialog.setFileMode(pqFileDialog::ExistingFile);

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  QString filename = dialog.getSelectedFiles()[0];
  vtksys::ifstream inputFile(filename.toStdString().c_str());
  Json::Value keyFramesConfiguration;
  Json::CharReaderBuilder builder;
  std::string errs;
  bool validJson = Json::parseFromStream(builder, inputFile, &keyFramesConfiguration, &errs);

  if (!validJson)
  {
    qWarning() << "Could not read:" << filename << "\n" << errs.c_str();
    return;
  }

  if (!keyFramesConfiguration["keyFrames"].isArray())
  {
    qWarning() << "The .pvkfc reader expect a `keyFrames` array containing all key frames.";
    return;
  }

  int keyFrameNumber = keyFramesConfiguration["keyFrames"].size();

  if (keyFrameNumber < 1)
  {
    qWarning() << "This .pvkfc file seems to be empty - a `keyFrames` array object is expected.";
    return;
  }

  const Json::Value& firstFrame = keyFramesConfiguration["keyFrames"][0];
  if (this->Internal->cameraCue())
  {
    if (!firstFrame["camera"].isObject())
    {
      qWarning() << "This .pvkfc file does not contain a `camera` object, even though a camera "
                    "animation has been selected.";
      return;
    }
    else if (this->Internal->cameraPathCue() ^ firstFrame["camera"]["positions"].isArray())
    {
      qWarning() << "This .pvkfc file is incompatible with the current camera animation mode.";
      return;
    }
  }
  else if (!firstFrame["interpolation"].isObject())
  {
    qWarning() << "This .pvkfc file does not contain an `interpolation` object, even though an "
                  "interpolation animation has been selected.";
    return;
  }

  this->deleteAllKeyFrames();
  for (int frameIdx = 0; frameIdx < keyFrameNumber - 1; frameIdx++)
  {
    this->newKeyFrame();
  }

  for (int frameIdx = 0; frameIdx < keyFrameNumber; frameIdx++)
  {
    const Json::Value& keyFrame = keyFramesConfiguration["keyFrames"][frameIdx];
    if (keyFrame["time"].isDouble())
    {
      QModelIndex idx = this->Internal->Model.index(frameIdx, 0);
      double realKeyTime = this->Internal->realTime(keyFrame["time"].asDouble());
      this->Internal->Model.setData(idx, realKeyTime, Qt::DisplayRole);
    }

    if (this->Internal->cameraCue())
    {
      pqCameraKeyFrameItem* item =
        dynamic_cast<pqCameraKeyFrameItem*>(this->Internal->Model.item(frameIdx, 1));
      if (item)
      {
        item->CamWidget.initializeUsingJSON(keyFrame["camera"]);
      }
    }
    else
    {
      pqKeyFrameInterpolationItem* item =
        static_cast<pqKeyFrameInterpolationItem*>(this->Internal->Model.item(frameIdx, 1));
      if (item && keyFrame["interpolation"].isObject())
      {
        item->Widget.initializeUsingJSON(keyFrame["interpolation"]);
      }
      if (keyFrame["value"].isDouble())
      {
        QModelIndex idx = this->Internal->Model.index(frameIdx, 2);
        this->Internal->Model.setData(idx, keyFrame["value"].asDouble(), Qt::DisplayRole);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::exportTrajectory()
{
  QString filters =
    QString("ParaView keyframes configuration (*.pvkfc);;") + "All Files" + " (*.*)";

  pqFileDialog dialog(
    nullptr, this, tr("Save Current KeyFrames Configuration"), "", filters, false);
  dialog.setFileMode(pqFileDialog::AnyFile);

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }
  Json::Value keyFramesConfiguration;

  for (int frameIdx = 0; frameIdx < this->Internal->Model.rowCount(); frameIdx++)
  {
    Json::Value keyFrame;
    QModelIndex idx = this->Internal->Model.index(frameIdx, 0);
    QVariant newData = this->Internal->Model.data(idx, Qt::DisplayRole);
    keyFrame["time"] = this->Internal->normalizedTime(newData.toDouble());

    if (this->Internal->cameraCue())
    {
      pqCameraKeyFrameItem* item =
        dynamic_cast<pqCameraKeyFrameItem*>(this->Internal->Model.item(frameIdx, 1));
      if (item)
      {
        keyFrame["camera"] = item->CamWidget.serializeToJSON();
      }
    }
    else
    {
      pqKeyFrameInterpolationItem* item =
        static_cast<pqKeyFrameInterpolationItem*>(this->Internal->Model.item(frameIdx, 1));
      if (item)
      {
        keyFrame["interpolation"] = item->Widget.serializeToJSON();
      }
      idx = this->Internal->Model.index(frameIdx, 2);
      newData = this->Internal->Model.data(idx, Qt::DisplayRole);
      keyFrame["value"] = newData.toDouble();
    }
    keyFramesConfiguration["keyFrames"].append(keyFrame);
  }

  QString filename = dialog.getSelectedFiles()[0];
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "  ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  vtksys::ofstream outputFile(filename.toStdString().c_str());
  writer->write(keyFramesConfiguration, &outputFile);
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::updateSplineMode()
{
  assert(this->Internal->cameraCue());
  bool useSpline = this->Internal->Ui.pbUseSpline->isChecked();
  pqSMAdaptor::setElementProperty(
    this->Internal->Cue->getProxy()->GetProperty("Interpolation"), useSpline ? 1 : 0);
  this->Internal->Cue->getProxy()->UpdateVTKObjects();
}
