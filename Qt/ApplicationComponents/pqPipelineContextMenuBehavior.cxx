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
#include "pqPipelineContextMenuBehavior.h"

#include "pqActiveObjects.h"
#include "pqContextMenuInterface.h"
#include "pqDefaultContextMenu.h"
#include "pqEditColorMapReaction.h"
#include "pqInterfaceTracker.h"
#include "pqPVApplicationCore.h"
#include "pqRenderView.h"
#include "pqSelectionManager.h"
#include "pqServerManagerModel.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkPVDataInformation.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtksys/SystemTools.hxx"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QtDebug>

#include <tuple>

namespace
{
/**
 * Returns true if the selection source is a block selection producer.
 */
bool isBlockSelection(vtkSMSourceProxy* source)
{
  return (source &&
    (strcmp(source->GetXMLName(), "BlockSelectorsSelectionSource") == 0 ||
      strcmp(source->GetXMLName(), "BlockSelectionSource") == 0));
}

std::pair<QList<unsigned int>, QStringList> getSelectedBlocks(
  pqDataRepresentation* repr, unsigned int blockIndex, int rank)
{
  auto producerPort = repr ? repr->getOutputPortFromInput() : nullptr;
  auto dataInfo = producerPort ? producerPort->getDataInformation() : nullptr;
  if (producerPort == nullptr || !dataInfo->IsCompositeDataSet())
  {
    return std::pair<QList<unsigned int>, QStringList>();
  }

  QList<unsigned int> legacyPickedBlocks;
  QStringList pickedSelectors;
  if (!isBlockSelection(producerPort->getSelectionInput()))
  {
    // If active selection is not block selection, the blocks will be simply the
    // one the user clicked on.
    legacyPickedBlocks.push_back(blockIndex);
    auto rankDataInfo = producerPort->getRankDataInformation(rank);
    auto selector =
      vtkDataAssemblyUtilities::GetSelectorForCompositeId(blockIndex, rankDataInfo->GetHierarchy());
    if (!selector.empty())
    {
      pickedSelectors.push_back(QString::fromStdString(selector));
    }
    return std::make_pair(legacyPickedBlocks, pickedSelectors);
  }

  // if actively selected data a block-type of selection, then we show properties
  // for the selected set of blocks.
  auto selectionProducer = producerPort->getSelectionInput();
  if (strcmp(selectionProducer->GetXMLName(), "BlockSelectorsSelectionSource") == 0)
  {
    auto svp =
      vtkSMStringVectorProperty::SafeDownCast(selectionProducer->GetProperty("BlockSelectors"));
    const auto& elements = svp->GetElements();
    std::transform(elements.begin(), elements.end(), std::back_inserter(pickedSelectors),
      [](const std::string& str) { return QString::fromStdString(str); });

    // convert selectors to ids (should we do this only for MBs, since it's
    // going to be incorrect for PDCs or PDCs in parallel?
    auto ids =
      vtkDataAssemblyUtilities::GetSelectedCompositeIds(elements, dataInfo->GetHierarchy());
    std::copy(ids.begin(), ids.end(), std::back_inserter(legacyPickedBlocks));
  }
  else if (strcmp(selectionProducer->GetXMLName(), "BlockSelectionSource") == 0)
  {
    auto idvp = vtkSMIdTypeVectorProperty::SafeDownCast(selectionProducer->GetProperty("Blocks"));
    const auto& elements = idvp->GetElements();
    std::copy(elements.begin(), elements.end(), std::back_inserter(legacyPickedBlocks));

    // convert ids to selectors; should we only do this for PDCs?
    const auto selectors = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
      std::vector<unsigned int>(elements.begin(), elements.end()), dataInfo->GetHierarchy());
    std::transform(selectors.begin(), selectors.end(), std::back_inserter(pickedSelectors),
      [](const std::string& str) { return QString::fromStdString(str); });
  }
  else
  {
    // should never happen!
    qCritical() << "Unexpected selectionProducer: " << selectionProducer->GetXMLName();
  }

  return std::make_pair(legacyPickedBlocks, pickedSelectors);
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

  QList<unsigned int> legacyPickedBlocks;
  QStringList pickedSelectors;
  std::tie(legacyPickedBlocks, pickedSelectors) = ::getSelectedBlocks(repr, blockIndex, rank);

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
      (mbldr->contextMenu(this->Menu, view, this->Position, repr, legacyPickedBlocks) ||
        mbldr->contextMenu(this->Menu, view, this->Position, repr, pickedSelectors)))
    {
      break;
    }
  }
}
