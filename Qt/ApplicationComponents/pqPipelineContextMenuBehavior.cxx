// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPipelineContextMenuBehavior.h"

#include "pqActiveObjects.h"
#include "pqContextMenuInterface.h"
#include "pqDefaultContextMenu.h"
#include "pqInterfaceTracker.h"
#include "pqPVApplicationCore.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

#include "vtkDataAssemblyUtilities.h"
#include "vtkPVDataInformation.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtksys/SystemTools.hxx"

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>

namespace
{
//-----------------------------------------------------------------------------
enum class SelectionSourceType
{
  BlockSelectors,
  BlockIds,
  None
};

//-----------------------------------------------------------------------------
QStringList getSelectedBlocks(pqDataRepresentation* repr, unsigned int blockIndex, int rank)
{
  auto producerPort = repr ? repr->getOutputPortFromInput() : nullptr;
  auto dataInfo = producerPort ? producerPort->getDataInformation() : nullptr;
  auto rDataInfo = producerPort ? producerPort->getRankDataInformation(rank) : nullptr;
  if (producerPort == nullptr || !dataInfo->IsCompositeDataSet())
  {
    return QStringList();
  }

  const std::string assemblyName = vtkSMPropertyHelper(repr->getProxy(), "Assembly").GetAsString();
  const auto hierarchy = dataInfo->GetHierarchy();
  const auto rHierarchy = rDataInfo->GetHierarchy();
  const auto assembly = dataInfo->GetDataAssembly(assemblyName.c_str());
  const auto rAssembly = rDataInfo->GetDataAssembly(assemblyName.c_str());

  QStringList pickedSelectors;

  auto appendSelections = producerPort->getSelectionInput();
  bool blockSelectionDetected = false;
  unsigned int numInputs =
    appendSelections ? vtkSMPropertyHelper(appendSelections, "Input").GetNumberOfElements() : 0;
  for (unsigned int i = 0; i < numInputs; ++i)
  {
    auto selectionSource =
      vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(i));

    // if actively selected data a block-type of selection, then we show properties
    // for the selected set of blocks.
    SelectionSourceType selectionSourceType = SelectionSourceType::None;
    std::string blockSelectorsAssemblyName;
    std::vector<std::string> selectors;
    if (strcmp(selectionSource->GetXMLName(), "BlockSelectorsSelectionSource") == 0)
    {
      selectionSourceType = SelectionSourceType::BlockSelectors;
      // get assembly name for selectors
      blockSelectorsAssemblyName =
        vtkSMPropertyHelper(selectionSource, "BlockSelectorsAssemblyName").GetAsString();
      // get block selectors
      auto svp =
        vtkSMStringVectorProperty::SafeDownCast(selectionSource->GetProperty("BlockSelectors"));
      selectors = svp->GetElements();
    }
    else if (strcmp(selectionSource->GetXMLName(), "BlockSelectionSource") == 0)
    {
      selectionSourceType = SelectionSourceType::BlockIds;
      blockSelectorsAssemblyName = vtkDataAssemblyUtilities::HierarchyName();
      // get block ids
      auto idvp = vtkSMIdTypeVectorProperty::SafeDownCast(selectionSource->GetProperty("Blocks"));
      const auto& elements = idvp->GetElements();

      // convert ids to hierarchy selectors; should we only do this for PDCs?
      selectors = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
        std::vector<unsigned int>(elements.begin(), elements.end()), hierarchy);
    }
    // check if there was a block selection
    const bool localBlockSelectionDetected = selectionSourceType != SelectionSourceType::None;
    if (!localBlockSelectionDetected)
    {
      continue;
    }
    if (blockSelectorsAssemblyName != vtkDataAssemblyUtilities::HierarchyName() &&
      assemblyName == vtkDataAssemblyUtilities::HierarchyName())
    {
      // This can't be done because we would need access to the PDC (which we can't have here),
      // so that we can convert the assembly selectors to composite ids.
      qCritical("Can't convert a selection source from '%s' to '%s'.",
        blockSelectorsAssemblyName.c_str(), assemblyName.c_str());
      continue;
    }
    blockSelectionDetected |= localBlockSelectionDetected;
    // if we have a block selection
    if (localBlockSelectionDetected)
    {
      // if there is a need for conversion
      if (assemblyName != blockSelectorsAssemblyName)
      {
        Q_ASSERT(blockSelectorsAssemblyName == vtkDataAssemblyUtilities::HierarchyName());
        Q_ASSERT(assemblyName != vtkDataAssemblyUtilities::HierarchyName());
        // convert the hierarchy selectors to composite ids
        auto compositeIds = vtkDataAssemblyUtilities::GetSelectedCompositeIds(selectors, hierarchy);
        // convert the composite ids to assembly selectors
        selectors =
          vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(compositeIds, hierarchy, assembly);
      }
      // insert the converted selectors into the list of picked selectors.
      std::transform(selectors.begin(), selectors.end(), std::back_inserter(pickedSelectors),
        [](const std::string& str) { return QString::fromStdString(str); });
    }
  }
  // If no active selection or no block selection detected, the blocks will be simply the
  // one the user clicked on.
  if (!blockSelectionDetected)
  {
    std::vector<std::string> selectors;
    if (assemblyName == vtkDataAssemblyUtilities::HierarchyName())
    {
      // get the hierarchy selector for the composite id
      selectors = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds({ blockIndex }, rHierarchy);
    }
    else
    {
      // convert the composite ids to assembly selectors
      selectors = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
        { blockIndex }, rHierarchy, rAssembly);
    }
    std::transform(selectors.begin(), selectors.end(), std::back_inserter(pickedSelectors),
      [](const std::string& str) { return QString::fromStdString(str); });
  }
  // remove selectors that are empty
  pickedSelectors.erase(std::remove_if(pickedSelectors.begin(), pickedSelectors.end(),
                          [](const QString& str) { return str.isEmpty(); }),
    pickedSelectors.end());

  return pickedSelectors;
}
} // end of namespace {}

//-----------------------------------------------------------------------------
pqPipelineContextMenuBehavior::pqPipelineContextMenuBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(viewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
  this->Menu = new QMenu();
  this->Menu->setObjectName("PipelineContextMenu");

  auto ifaceTracker = pqApplicationCore::instance()->interfaceTracker();
  ifaceTracker->addInterface(new pqDefaultContextMenu());
}

//-----------------------------------------------------------------------------
pqPipelineContextMenuBehavior::~pqPipelineContextMenuBehavior()
{
  delete this->Menu;
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::onViewAdded(pqView* view)
{
  if (view && view->getProxy()->IsA("vtkSMRenderViewProxy"))
  {
    // add a link view menu
    view->widget()->installEventFilter(this);
  }
}

//-----------------------------------------------------------------------------
bool pqPipelineContextMenuBehavior::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton)
    {
      this->Position = me->pos();
    }
  }
  else if (e->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton && !this->Position.isNull())
    {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Position;
      QWidget* senderWidget = qobject_cast<QWidget*>(caller);
      if (delta.manhattanLength() < 3 && senderWidget != nullptr)
      {
        pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
        if (view)
        {
          int pos[2] = { newPos.x(), newPos.y() };
          // we need to flip Y.
          int height = senderWidget->size().height();
          pos[1] = height - pos[1];
          // Account for HiDPI displays
          qreal devicePixelRatioF = senderWidget->devicePixelRatioF();
          if (!vtksys::SystemTools::HasEnv("DASHBOARD_TEST_FROM_CTEST"))
          {
            pos[0] = pos[0] * devicePixelRatioF;
            pos[1] = pos[1] * devicePixelRatioF;
          }
          unsigned int blockIndex = 0;
          int rank = 0;
          QPointer<pqDataRepresentation> pickedRepresentation;
          pickedRepresentation = view->pickBlock(pos, blockIndex, rank);
          this->buildMenu(pickedRepresentation, blockIndex, rank);
          this->Menu->popup(senderWidget->mapToGlobal(newPos));
        }
      }
      this->Position = QPoint();
    }
  }

  return Superclass::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::buildMenu(
  pqDataRepresentation* repr, unsigned int blockIndex, int rank)
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());

  const QStringList pickedSelectors = ::getSelectedBlocks(repr, blockIndex, rank);

  this->Menu->clear();

  auto interfaces =
    pqApplicationCore::instance()->interfaceTracker()->interfaces<pqContextMenuInterface*>();
  // Sort the list by priority:
  std::sort(interfaces.begin(), interfaces.end(),
    [](const pqContextMenuInterface* a, const pqContextMenuInterface* b) -> bool {
      return a->priority() > b->priority();
    });
  for (auto mbldr : interfaces)
  {
    if (mbldr &&
      (mbldr->contextMenu(
         this->Menu, view, this->Position, repr, QList<unsigned int>() /*not used*/) ||
        mbldr->contextMenu(this->Menu, view, this->Position, repr, pickedSelectors)))
    {
      break;
    }
  }
}
