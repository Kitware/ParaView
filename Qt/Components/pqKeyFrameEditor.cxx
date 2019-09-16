/*=========================================================================

   Program:   ParaQ
   Module:    pqKeyFrameEditor.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.2.

   See License_v1.2.txt for the full ParaQ license.
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

#include "pqKeyFrameEditor.h"
#include "ui_pqKeyFrameEditor.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLineEdit>
#include <QPointer>
#include <QSignalMapper>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <vtkSMRenderViewProxy.h>

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqCameraKeyFrameWidget.h"
#include "pqKeyFrameTypeWidget.h"
#include "pqPropertyLinks.h"
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
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setWindowModality(Qt::WindowModal);
  this->setWindowTitle(tr("Key Frame Interpolation"));
  this->setModal(true);
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(this->Child, 0, 0);
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

  l->addStretch();
  l->addWidget(buttons, 1, 0);
  this->Child->show();
}

//-----------------------------------------------------------------------------
pqKeyFrameEditorDialog::~pqKeyFrameEditorDialog()
{
  // disconnect child
  this->Child->setParent(NULL);
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
    l->setMargin(0);
    l->addWidget(this->Child);
    this->Child->show();
  }
  ~pqKeyFrameEditorWidget() override
  {
    this->Child->setParent(NULL);
    this->Child->hide();
  }

private:
  QWidget* Child;
};

//-----------------------------------------------------------------------------
// model model for key frame values
class pqKeyFrameItem : public QObject, public QStandardItem
{
public:
  // return an editor for the item
  virtual QWidget* editorWidget() { return NULL; }
  // return an editor for a dialog
  virtual QWidget* editorDialog() { return NULL; }
};

//-----------------------------------------------------------------------------
// model item for putting a key frame interpolation widget in the model
class pqKeyFrameInterpolationItem : public pqKeyFrameItem
{
public:
  pqKeyFrameInterpolationItem()
    : Widget()
  {
  }
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
    : Widget()
    , CamWidget(&this->Widget)
  {
    QVBoxLayout* l = new QVBoxLayout(&this->Widget);
    l->setMargin(0);
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
    return NULL;
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
    QObject::connect(
      &this->CameraMapper, SIGNAL(mapped(QObject*)), editor, SLOT(useCurrentCamera(QObject*)));
  }
  pqKeyFrameEditor* const Editor;
  Ui::pqKeyFrameEditor Ui;
  QPointer<pqAnimationCue> Cue;
  QPointer<pqAnimationScene> Scene;
  QStandardItemModel Model;
  QPair<double, double> TimeRange;
  QPair<QVariant, QVariant> ValueRange;
  pqKeyFrameEditorDelegate* EditorDelegate;
  QSignalMapper CameraMapper;

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

  QList<QStandardItem*> newRow(int row)
  {
    QList<QStandardItem*> items;
    items.append(this->newTimeItem(row));
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
    pqKeyFrameInterpolationItem* item = NULL;
    int count = this->Model.rowCount();
    if (count != row || row == 0)
    {
      item = new pqKeyFrameInterpolationItem();
    }
    return item;
  }
  pqCameraKeyFrameItem* newCameraItem(int)
  {
    pqCameraKeyFrameItem* item = NULL;
    item = new pqCameraKeyFrameItem();

    QObject::connect(
      &item->CamWidget, SIGNAL(useCurrentCamera()), &this->CameraMapper, SLOT(map()));
    this->CameraMapper.setMapping(&item->CamWidget, item);
    // default to current view
    this->Editor->useCurrentCamera(item);
    item->CamWidget.setUsePathBasedMode(
      pqSMAdaptor::getEnumerationProperty(this->Cue->getProxy()->GetProperty("Mode")) ==
      "Path-based");
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
  bool cameraCue()
  {
    if (QString("CameraAnimationCue") == this->Cue->getProxy()->GetXMLName())
    {
      return true;
    }
    return false;
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
  else if (domain.size())
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

  if (label != QString())
  {
    this->Internal->Ui.label->setText(label);
  }
  else
  {
    this->Internal->Ui.label->hide();
  }

  this->readKeyFrameData();
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
      bool path_based = pqSMAdaptor::getEnumerationProperty(
                          this->Internal->Cue->getProxy()->GetProperty("Mode")) == "Path-based";
      if ((i < numberKeyFrames - 1) || !path_based)
      {
        pqCameraKeyFrameItem* item = new pqCameraKeyFrameItem();
        QObject::connect(
          &item->CamWidget, SIGNAL(useCurrentCamera()), &this->Internal->CameraMapper, SLOT(map()));
        this->Internal->CameraMapper.setMapping(&item->CamWidget, item);
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

  BEGIN_UNDO_SET("Edit Keyframes");

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

  QList<QPair<int, double> > sortedKeyFrames;
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
  }
  else
  {
    row = count != 0 ? count - 1 : 0;
  }

  this->Internal->Model.insertRow(row, this->Internal->newRow(row));

  // add one more
  if (count == 0)
  {
    this->Internal->Model.insertRow(1, this->Internal->newRow(1));
  }
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
    if (item)
    {
      delete item;
    }
  }
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::deleteAllKeyFrames()
{
  // remove all rows
  this->Internal->Model.removeRows(0, this->Internal->Model.rowCount());
}

//-----------------------------------------------------------------------------
void pqKeyFrameEditor::useCurrentCamera(QObject* o)
{
  pqCameraKeyFrameItem* item = static_cast<pqCameraKeyFrameItem*>(o);

  vtkSMProxy* pxy = this->Internal->Cue->getAnimatedProxy();

  vtkSMRenderViewProxy* ren = vtkSMRenderViewProxy::SafeDownCast(pxy);
  ren->SynchronizeCameraProperties();
  item->CamWidget.initializeUsingCamera(ren->GetActiveCamera());
}
