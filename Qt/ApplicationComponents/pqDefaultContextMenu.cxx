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
#include "pqDefaultContextMenu.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqDoubleRangeDialog.h"
#include "pqEditColorMapReaction.h"
#include "pqInterfaceTracker.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqScalarsToColors.h"
#include "pqSelectionManager.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqUndoStack.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMDoubleMapProperty.h"
#include "vtkSMDoubleMapPropertyIterator.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"
#include "vtksys/SystemTools.hxx"

#include <QAction>
#include <QApplication>
#include <QColorDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QPair>
#include <QWidget>

#include <cassert>

namespace
{
// converts array association/name pair to QVariant.
QVariant convert(const QPair<int, QString>& array)
{
  if (!array.second.isEmpty())
  {
    QStringList val;
    val << QString::number(array.first) << array.second;
    return val;
  }
  return QVariant();
}

// converts QVariant to array association/name pair.
QPair<int, QString> convert(const QVariant& val)
{
  QPair<int, QString> result;
  if (val.canConvert<QStringList>())
  {
    QStringList list = val.toStringList();
    assert(list.size() == 2);
    if (list.size() == 2)
    {
      result.first = list[0].toInt();
      result.second = list[1];
    }
  }
  return result;
}
}

//-----------------------------------------------------------------------------
pqDefaultContextMenu::pqDefaultContextMenu(QObject* parentObject)
  : QObject(parentObject)
{
}

//-----------------------------------------------------------------------------
pqDefaultContextMenu::~pqDefaultContextMenu() = default;

//-----------------------------------------------------------------------------
bool pqDefaultContextMenu::contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
  pqRepresentation* dataContext, const QList<unsigned int>& dataBlockContext) const
{
  this->PickedRepresentation = nullptr;
  this->PickedBlocks = dataBlockContext;
  this->Position = viewPoint;
  // unsigned int blockIndex = dataBlockContext.empty() ? -1 : dataBlockContext[0];
  auto* repr = dynamic_cast<pqDataRepresentation*>(dataContext);
  if (repr)
  {
    this->PickedRepresentation = repr;

    vtkPVDataInformation* info = repr->getInputDataInformation();
    // Hiding the block specific parameters for now.
    // We'll need to revisit if want to preserve these or let MultiBlock Inspector be the
    // way for users to handle this.
    if (info->IsCompositeDataSet())
    {
#if 0
      bool multipleBlocks = this->PickedBlocks.size() > 1;

      if (multipleBlocks)
      {
        menu->addAction(QString("%1 Blocks").arg(this->PickedBlocks.size()));
      }
      else
      {
        const auto blockName = info->GetBlockName(static_cast<vtkTypeInt64>(blockIndex));
        menu->addAction(QString("Block '%1'").arg(blockName.c_str()));
      }
      menu->addSeparator();

      QAction* hideBlockAction =
        menu->addAction(QString("Hide Block%1").arg(multipleBlocks ? "s" : ""));
      this->connect(hideBlockAction, SIGNAL(triggered()), this, SLOT(hideBlock()));

      QAction* showOnlyBlockAction =
        menu->addAction(QString("Show Only Block%1").arg(multipleBlocks ? "s" : ""));
      this->connect(showOnlyBlockAction, SIGNAL(triggered()), this, SLOT(showOnlyBlock()));

      QAction* showAllBlocksAction = menu->addAction("Show All Blocks");
      this->connect(showAllBlocksAction, SIGNAL(triggered()), this, SLOT(showAllBlocks()));

      QAction* unsetVisibilityAction = menu->addAction(
        QString("Unset Block %1").arg(multipleBlocks ? "Visibilities" : "Visibility"));
      this->connect(unsetVisibilityAction, SIGNAL(triggered()), this, SLOT(unsetBlockVisibility()));

      menu->addSeparator();

      QAction* setBlockColorAction =
        menu->addAction(QString("Set Block Color%1").arg(multipleBlocks ? "s" : ""));
      this->connect(setBlockColorAction, SIGNAL(triggered()), this, SLOT(setBlockColor()));

      QAction* unsetBlockColorAction =
        menu->addAction(QString("Unset Block Color%1").arg(multipleBlocks ? "s" : ""));
      this->connect(unsetBlockColorAction, SIGNAL(triggered()), this, SLOT(unsetBlockColor()));

      menu->addSeparator();

      QAction* setBlockOpacityAction =
        menu->addAction(QString("Set Block %1").arg(multipleBlocks ? "Opacities" : "Opacity"));
      this->connect(setBlockOpacityAction, SIGNAL(triggered()), this, SLOT(setBlockOpacity()));

      QAction* unsetBlockOpacityAction =
        menu->addAction(QString("Unset Block %1").arg(multipleBlocks ? "Opacities" : "Opacity"));
      this->connect(unsetBlockOpacityAction, SIGNAL(triggered()), this, SLOT(unsetBlockOpacity()));

      menu->addSeparator();
#endif
    }

    QAction* action;
    action = menu->addAction("Hide");
    QObject::connect(action, &QAction::triggered, this, &pqDefaultContextMenu::hide);

    QMenu* reprMenu = menu->addMenu("Representation") << pqSetName("Representation");

    // populate the representation types menu.
    QList<QVariant> rTypes =
      pqSMAdaptor::getEnumerationPropertyDomain(repr->getProxy()->GetProperty("Representation"));
    QVariant curRType =
      pqSMAdaptor::getEnumerationProperty(repr->getProxy()->GetProperty("Representation"));
    foreach (QVariant rtype, rTypes)
    {
      QAction* raction = reprMenu->addAction(rtype.toString());
      raction->setCheckable(true);
      raction->setChecked(rtype == curRType);
    }

    QObject::connect(reprMenu, &QMenu::triggered, this, &pqDefaultContextMenu::reprTypeChanged);

    menu->addSeparator();

    pqPipelineRepresentation* pipelineRepr = qobject_cast<pqPipelineRepresentation*>(repr);

    if (pipelineRepr)
    {
      QMenu* colorFieldsMenu = menu->addMenu("Color By") << pqSetName("ColorBy");
      this->buildColorFieldsMenu(pipelineRepr, colorFieldsMenu);
    }

    action = menu->addAction("Edit Color");
    new pqEditColorMapReaction(action);

    menu->addSeparator();
  }
  else
  {
    repr = pqActiveObjects::instance().activeRepresentation();
    if (repr)
    {
      vtkPVDataInformation* info = repr->getInputDataInformation();
      if (info->IsCompositeDataSet())
      {
        QAction* showAllBlocksAction = menu->addAction("Show All Blocks");
        this->connect(showAllBlocksAction, SIGNAL(triggered()), this, SLOT(showAllBlocks()));
      }
    }
  }

  // Even when nothing was picked, show the "link camera" and
  // possibly the frame decoration menu items.
  menu->addAction("Link Camera...", viewContext, SLOT(linkToOtherView()));

  if (auto tmvwidget = qobject_cast<pqTabbedMultiViewWidget*>(
        pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET")))
  {
    auto actn = menu->addAction("Show Frame Decorations");
    actn->setCheckable(true);
    actn->setChecked(tmvwidget->decorationsVisibility());
    QObject::connect(
      actn, &QAction::triggered, tmvwidget, &pqTabbedMultiViewWidget::setDecorationsVisibility);
  }
  return false; // Allow lower-priority interfaces to modify the menu.
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::buildColorFieldsMenu(
  pqPipelineRepresentation* pipelineRepr, QMenu* menu) const
{
  QObject::connect(
    menu, &QMenu::triggered, this, &pqDefaultContextMenu::colorMenuTriggered, Qt::QueuedConnection);

  QIcon cellDataIcon(":/pqWidgets/Icons/pqCellData.svg");
  QIcon pointDataIcon(":/pqWidgets/Icons/pqPointData.svg");
  QIcon solidColorIcon(":/pqWidgets/Icons/pqSolidColor.svg");

  menu->addAction(solidColorIcon, "Solid Color")->setData(convert(QPair<int, QString>()));
  vtkSMProperty* prop = pipelineRepr->getProxy()->GetProperty("ColorArrayName");
  if (!prop)
  {
    return;
  }
  auto domain = prop->FindDomain<vtkSMArrayListDomain>();
  if (!domain)
  {
    return;
  }

  // We are only showing array names here without worrying about components since that
  // keeps the menu simple and code even simpler :).
  for (unsigned int cc = 0, max = domain->GetNumberOfStrings(); cc < max; cc++)
  {
    int association = domain->GetFieldAssociation(cc);
    int icon_association = domain->GetDomainAssociation(cc);
    QString name = domain->GetString(cc);

    QIcon& icon = (icon_association == vtkDataObject::CELL) ? cellDataIcon : pointDataIcon;

    QVariant data = convert(QPair<int, QString>(association, name));
    menu->addAction(icon, name)->setData(data);
  }
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::colorMenuTriggered(QAction* action)
{
  QPair<int, QString> array = convert(action->data());
  if (this->PickedRepresentation)
  {
    BEGIN_UNDO_SET("Change coloring");
    vtkSMViewProxy* view = pqActiveObjects::instance().activeView()->getViewProxy();
    vtkSMProxy* reprProxy = this->PickedRepresentation->getProxy();

    vtkSMProxy* oldLutProxy = vtkSMPropertyHelper(reprProxy, "LookupTable", true).GetAsProxy();

    vtkSMPVRepresentationProxy::SetScalarColoring(
      reprProxy, array.second.toLocal8Bit().data(), array.first);

    vtkNew<vtkSMTransferFunctionManager> tmgr;

    // Hide unused scalar bars, if applicable.
    vtkPVGeneralSettings* gsettings = vtkPVGeneralSettings::GetInstance();
    switch (gsettings->GetScalarBarMode())
    {
      case vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS:
      case vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS:
        tmgr->HideScalarBarIfNotNeeded(oldLutProxy, view);
        break;
    }

    if (!array.second.isEmpty())
    {
      // we could now respect some application setting to determine if the LUT is
      // to be reset.
      vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(reprProxy, true);

      /// BUG #0011858. Users often do silly things!
      bool reprVisibility =
        vtkSMPropertyHelper(reprProxy, "Visibility", /*quiet*/ true).GetAsInt() == 1;

      // now show used scalar bars if applicable.
      if (reprVisibility &&
        gsettings->GetScalarBarMode() ==
          vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
      {
        vtkSMPVRepresentationProxy::SetScalarBarVisibility(reprProxy, view, true);
      }
    }

    this->PickedRepresentation->renderViewEventually();
    END_UNDO_SET();
  }
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::reprTypeChanged(QAction* action)
{
  pqDataRepresentation* repr = this->PickedRepresentation;
  if (repr)
  {
    BEGIN_UNDO_SET("Representation Type Changed");
    pqSMAdaptor::setEnumerationProperty(
      repr->getProxy()->GetProperty("Representation"), action->text());
    repr->getProxy()->UpdateVTKObjects();
    repr->renderViewEventually();
    END_UNDO_SET();
  }
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::hide()
{
  pqDataRepresentation* repr = this->PickedRepresentation;
  if (repr)
  {
    BEGIN_UNDO_SET("Visibility Changed");
    repr->setVisible(false);
    repr->renderViewEventually();
    END_UNDO_SET();
  }
}

namespace
{
void readVisibilityMap(vtkSMIntVectorProperty* ivp, QMap<unsigned int, int>& visibilities)
{
  for (unsigned i = 0; i + 1 < ivp->GetNumberOfElements(); i += 2)
  {
    visibilities[ivp->GetElement(i)] = ivp->GetElement(i + 1);
  }
}

void setVisibilitiesFromMap(
  vtkSMIntVectorProperty* ivp, QMap<unsigned int, int>& visibilities, vtkSMProxy* proxy)
{
  std::vector<int> vector;

  for (QMap<unsigned int, int>::const_iterator i = visibilities.begin(); i != visibilities.end();
       i++)
  {
    vector.push_back(static_cast<int>(i.key()));
    vector.push_back(static_cast<int>(i.value()));
  }
  BEGIN_UNDO_SET("Change Block Visibilities");
  if (!vector.empty())
  {
    // if property changes, ModifiedEvent will be fired and
    // this->UpdateUITimer will be started.
    ivp->SetElements(&vector[0], static_cast<unsigned int>(vector.size()));
  }
  proxy->UpdateVTKObjects();
  END_UNDO_SET();
}
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::hideBlock()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
  {
    return;
  }

  vtkSMProxy* proxy = this->PickedRepresentation->getProxy();
  vtkSMProperty* property = proxy->GetProperty("BlockVisibility");
  if (property)
  {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(property);
    QMap<unsigned int, int> visibilities;
    readVisibilityMap(ivp, visibilities);
    for (int i = 0; i < this->PickedBlocks.size(); ++i)
    {
      visibilities[this->PickedBlocks[i]] = 0;
    }
    setVisibilitiesFromMap(ivp, visibilities, proxy);
  }
  this->PickedRepresentation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::showOnlyBlock()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
  {
    return;
  }

  vtkSMProxy* proxy = this->PickedRepresentation->getProxy();
  vtkSMProperty* property = proxy->GetProperty("BlockVisibility");
  if (property)
  {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(property);
    QMap<unsigned int, int> visibilities;
    visibilities[0] = 0;
    for (int i = 0; i < this->PickedBlocks.size(); ++i)
    {
      visibilities[this->PickedBlocks[i]] = 1;
    }
    setVisibilitiesFromMap(ivp, visibilities, proxy);
  }
  this->PickedRepresentation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::showAllBlocks()
{
  pqRepresentation* repr = this->PickedRepresentation;
  if (!repr)
  {
    repr = pqActiveObjects::instance().activeRepresentation();
    if (!repr)
    {
      return;
    }
  }
  vtkSMProxy* proxy = repr->getProxy();
  if (!proxy)
  {
    return;
  }
  vtkSMProperty* property = proxy->GetProperty("BlockVisibility");
  if (property)
  {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(property);
    QMap<unsigned int, int> visibilities;
    visibilities[0] = 1;
    setVisibilitiesFromMap(ivp, visibilities, proxy);
  }
  repr->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::unsetBlockVisibility()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
  {
    return;
  }

  vtkSMProxy* proxy = this->PickedRepresentation->getProxy();
  vtkSMProperty* property = proxy->GetProperty("BlockVisibility");
  if (property)
  {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(property);
    QMap<unsigned int, int> visibilities;
    readVisibilityMap(ivp, visibilities);
    for (int i = 0; i < this->PickedBlocks.size(); ++i)
    {
      visibilities.remove(this->PickedBlocks[i]);
    }
    setVisibilitiesFromMap(ivp, visibilities, proxy);
  }
  this->PickedRepresentation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::setBlockColor()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
  {
    return;
  }

  QColor qcolor = QColorDialog::getColor(QColor(), pqCoreUtilities::mainWidget(),
    "Choose Block Color", QColorDialog::DontUseNativeDialog);

  vtkSMProxy* proxy = this->PickedRepresentation->getProxy();
  vtkSMProperty* property = proxy->GetProperty("BlockColor");
  if (property)
  {
    BEGIN_UNDO_SET("Change Block Colors");
    vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(property);
    for (int i = 0; i < this->PickedBlocks.size(); ++i)
    {
      double color[] = { qcolor.redF(), qcolor.greenF(), qcolor.blueF() };
      dmp->SetElements(this->PickedBlocks[i], color);
    }
    proxy->UpdateVTKObjects();
    END_UNDO_SET();
  }
  this->PickedRepresentation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::unsetBlockColor()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
  {
    return;
  }

  vtkSMProxy* proxy = this->PickedRepresentation->getProxy();
  vtkSMProperty* property = proxy->GetProperty("BlockColor");
  if (property)
  {
    BEGIN_UNDO_SET("Change Block Colors");
    vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(property);
    QMap<unsigned int, QColor> blockColors;
    vtkSmartPointer<vtkSMDoubleMapPropertyIterator> iter;
    iter.TakeReference(dmp->NewIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      QColor color = QColor::fromRgbF(
        iter->GetElementComponent(0), iter->GetElementComponent(1), iter->GetElementComponent(2));
      blockColors.insert(iter->GetKey(), color);
    }
    for (int i = 0; i < this->PickedBlocks.size(); ++i)
    {
      blockColors.remove(this->PickedBlocks[i]);
    }

    dmp->ClearElements();
    QMap<unsigned int, QColor>::const_iterator iter2;
    for (iter2 = blockColors.begin(); iter2 != blockColors.end(); iter2++)
    {
      QColor qcolor = iter2.value();
      double color[] = { qcolor.redF(), qcolor.greenF(), qcolor.blueF() };
      dmp->SetElements(iter2.key(), color);
    }
    proxy->UpdateVTKObjects();
    END_UNDO_SET();
  }
  this->PickedRepresentation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::setBlockOpacity()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
  {
    return;
  }

  vtkSMProxy* proxy = this->PickedRepresentation->getProxy();
  vtkSMProperty* property = proxy->GetProperty("BlockOpacity");
  if (property)
  {
    vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(property);
    // Hope this works?
    double current_opacity = 1;
    if (dmp->HasElement(this->PickedBlocks[0]))
    {
      current_opacity = dmp->GetElement(this->PickedBlocks[0]);
    }

    pqDoubleRangeDialog dialog("Opacity:", 0.0, 1.0, pqCoreUtilities::mainWidget());
    dialog.setValue(current_opacity);
    bool ok = dialog.exec();
    if (!ok)
    {
      return;
    }

    BEGIN_UNDO_SET("Change Block Opacities");
    for (int i = 0; i < this->PickedBlocks.size(); ++i)
    {
      dmp->SetElement(this->PickedBlocks[i], dialog.value());
    }
    proxy->UpdateVTKObjects();
    END_UNDO_SET();
  }
  this->PickedRepresentation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::unsetBlockOpacity()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action)
  {
    return;
  }

  vtkSMProxy* proxy = this->PickedRepresentation->getProxy();
  vtkSMProperty* property = proxy->GetProperty("BlockOpacity");
  if (property)
  {
    BEGIN_UNDO_SET("Change Block Opacities");
    vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(property);

    for (int i = 0; i < this->PickedBlocks.size(); ++i)
    {
      dmp->RemoveElement(this->PickedBlocks[i]);
    }
    proxy->UpdateVTKObjects();
    END_UNDO_SET();
  }
  this->PickedRepresentation->renderViewEventually();
}

namespace
{
const char* findBlockName(
  int flatIndexTarget, int& flatIndexCurrent, vtkPVCompositeDataInformation* currentInfo)
{
  (void)flatIndexCurrent;
  (void)flatIndexTarget;
  (void)currentInfo;
// FIXME in the context of https://gitlab.kitware.com/paraview/paraview/-/issues/20081
#if 0
  // An interior block shouldn't be selected, only blocks with geometry can be
  if (flatIndexCurrent == flatIndexTarget)
  {
    return nullptr;
  }
  for (unsigned int i = 0; i < currentInfo->GetNumberOfChildren(); i++)
  {
    ++flatIndexCurrent;
    if (flatIndexCurrent == flatIndexTarget)
    {
      return currentInfo->GetName(i);
    }
    else if (flatIndexCurrent > flatIndexTarget)
    {
      return nullptr;
    }
    vtkPVDataInformation* childInfo = currentInfo->GetDataInformation(i);
    if (childInfo)
    {
      vtkPVCompositeDataInformation* compositeChildInfo = childInfo->GetCompositeDataInformation();

      // recurse down through child blocks only if the child block
      // is composite and is not a multi-piece data set
      if (compositeChildInfo->GetDataIsComposite() && !compositeChildInfo->GetDataIsMultiPiece())
      {
        const char* result = findBlockName(flatIndexTarget, flatIndexCurrent, compositeChildInfo);
        if (result)
        {
          return result;
        }
      }
      else if (compositeChildInfo && compositeChildInfo->GetDataIsMultiPiece())
      {
        flatIndexCurrent += compositeChildInfo->GetNumberOfChildren();
      }
    }
  }
#endif
  return nullptr;
}
}

//-----------------------------------------------------------------------------
QString pqDefaultContextMenu::lookupBlockName(unsigned int flatIndex) const
{
  Q_UNUSED(flatIndex);
  return QString(); // FIXME in the context of
                    // https://gitlab.kitware.com/paraview/paraview/-/issues/20081
#if 0
  vtkPVDataInformation* info = this->PickedRepresentation->getRepresentedDataInformation();
  if (!info)
  {
    return QString();
  }
  vtkPVCompositeDataInformation* compositeInfo = info->GetCompositeDataInformation();

  int myIdx = 0;
  const char* name = findBlockName(flatIndex, myIdx, compositeInfo);
  if (name)
  {
    return QString(name);
  }
  else
  {
    return QString();
  }
#endif
}
