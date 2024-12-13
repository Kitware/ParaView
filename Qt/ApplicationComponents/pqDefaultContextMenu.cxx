// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDefaultContextMenu.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEditColorMapReaction.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqUndoStack.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"
#include "vtksys/SystemTools.hxx"

#include <QAction>
#include <QApplication>
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
  pqRepresentation* dataContext, const QList<unsigned int>&) const
{
  this->PickedRepresentation = nullptr;
  this->Position = viewPoint;
  auto* repr = dynamic_cast<pqDataRepresentation*>(dataContext);
  if (repr)
  {
    this->PickedRepresentation = repr;

    QAction* action;
    action = menu->addAction(QIcon(":/pqWidgets/Icons/pqEyeballClosed.svg"), tr("Hide"));
    QObject::connect(action, &QAction::triggered, this, &pqDefaultContextMenu::hide);

    QMenu* reprMenu = menu->addMenu(tr("Representation")) << pqSetName("Representation");

    // populate the representation types menu.
    QList<QVariant> rTypes =
      pqSMAdaptor::getEnumerationPropertyDomain(repr->getProxy()->GetProperty("Representation"));
    QVariant curRType =
      pqSMAdaptor::getEnumerationProperty(repr->getProxy()->GetProperty("Representation"));
    Q_FOREACH (QVariant rtype, rTypes)
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
      QMenu* colorFieldsMenu =
        menu->addMenu(QIcon(":/pqWidgets/Icons/explicit_color.png"), tr("Color By"))
        << pqSetName("ColorBy");
      this->buildColorFieldsMenu(pipelineRepr, colorFieldsMenu);
    }

    action = menu->addAction(QIcon(":/pqWidgets/Icons/pqEditColor.svg"), tr("Edit Color"));

    // Don't edit colors on the active representation, edit them on the selected representation.
    // Fixes #22391.
    auto editColorReaction =
      new pqEditColorMapReaction(action, false /* don't track active objects */);
    editColorReaction->setRepresentation(repr);

    menu->addSeparator();
  }
  else
  {
    QAction* showAllBlocksAction = menu->addAction(tr("Show All Blocks"));
    this->connect(showAllBlocksAction, SIGNAL(triggered()), this, SLOT(showAllBlocks()));
  }

  // Even when nothing was picked, show the "link camera" and
  // possibly the frame decoration menu items.
  menu->addAction(tr("Link Camera..."), viewContext, SLOT(linkToOtherView()));
  menu->addAction(tr("Unlink Camera"), viewContext, SLOT(removeViewLinks()));

  if (auto tmvwidget = qobject_cast<pqTabbedMultiViewWidget*>(
        pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET")))
  {
    auto actn = menu->addAction(tr("Show Frame Decorations"));
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

  menu->addAction(solidColorIcon, tr("Solid Color"))->setData(convert(QPair<int, QString>()));
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
void pqDefaultContextMenu::showAllBlocks() const
{
  QList<pqRepresentation*> representations =
    pqActiveObjects::instance().activeView()->getRepresentations();
  for (pqRepresentation* repr : representations)
  {
    if (!repr)
    {
      continue;
    }
    if (repr->isVisible())
    {
      vtkSMProxy* proxy = repr->getProxy();
      if (!proxy)
      {
        continue;
      }
      vtkSMProperty* property = proxy->GetProperty("BlockSelectors");

      auto smProperty = vtkSMStringVectorProperty::SafeDownCast(property);
      if (!smProperty)
      {
        continue;
      }

      SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy);
      BEGIN_UNDO_SET(tr("Show All Blocks"));
      smProperty->SetElements(std::vector<std::string>({ "/" }));
      proxy->UpdateVTKObjects();
      END_UNDO_SET();
      repr->renderViewEventually();
    }
  }
}

//-----------------------------------------------------------------------------
void pqDefaultContextMenu::colorMenuTriggered(QAction* action)
{
  QPair<int, QString> array = convert(action->data());
  if (this->PickedRepresentation)
  {
    BEGIN_UNDO_SET(tr("Change coloring"));
    vtkSMViewProxy* view = pqActiveObjects::instance().activeView()->getViewProxy();
    vtkSMProxy* reprProxy = this->PickedRepresentation->getProxy();

    vtkSMProxy* oldLutProxy = vtkSMPropertyHelper(reprProxy, "LookupTable", true).GetAsProxy();

    vtkSMColorMapEditorHelper::SetScalarColoring(
      reprProxy, array.second.toUtf8().data(), array.first);

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
      vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(reprProxy, true);

      /// BUG #0011858. Users often do silly things!
      bool reprVisibility =
        vtkSMPropertyHelper(reprProxy, "Visibility", /*quiet*/ true).GetAsInt() == 1;

      // now show used scalar bars if applicable.
      if (reprVisibility &&
        gsettings->GetScalarBarMode() ==
          vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
      {
        vtkSMColorMapEditorHelper::SetScalarBarVisibility(reprProxy, view, true);
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
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", repr->getProxy());
    BEGIN_UNDO_SET(tr("Representation Type Changed"));
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
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", repr->getProxy());
    BEGIN_UNDO_SET(tr("Visibility Changed"));
    repr->setVisible(false);
    repr->renderViewEventually();
    END_UNDO_SET();
  }
}
