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
#include "pqApplicationCore.h"
#include "pqContextMenuInterface.h"
#include "pqCoreUtilities.h"
#include "pqDefaultContextMenu.h"
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
    result.first = list[0].toInt();
    result.second = list[1];
  }
  return result;
}
}

//-----------------------------------------------------------------------------
pqPipelineContextMenuBehavior::pqPipelineContextMenuBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(viewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
  this->Menu = new QMenu();
  this->Menu << pqSetName("PipelineContextMenu");

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
          QPointer<pqDataRepresentation> pickedRepresentation;
          pickedRepresentation = view->pickBlock(pos, blockIndex);
          this->buildMenu(pickedRepresentation, blockIndex);
          this->Menu->popup(senderWidget->mapToGlobal(newPos));
        }
      }
      this->Position = QPoint();
    }
  }

  return Superclass::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::buildMenu(pqDataRepresentation* repr, unsigned int blockIndex)
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());

  QList<unsigned int> pickedBlocks;

  bool picked_block_in_selected_blocks = false;
  pqSelectionManager* selectionManager = pqPVApplicationCore::instance()->selectionManager();
  if (selectionManager)
  {
    pqOutputPort* port = selectionManager->getSelectedPort();
    if (port)
    {
      vtkSMSourceProxy* activeSelection = port->getSelectionInput();
      if (activeSelection && strcmp(activeSelection->GetXMLName(), "BlockSelectionSource") == 0)
      {
        vtkSMPropertyHelper blocksProp(activeSelection, "Blocks");
        QVector<vtkIdType> vblocks;
        vblocks.resize(blocksProp.GetNumberOfElements());
        blocksProp.Get(&vblocks[0], blocksProp.GetNumberOfElements());
        foreach (const vtkIdType& index, vblocks)
        {
          if (index >= 0)
          {
            if (static_cast<unsigned int>(index) == blockIndex)
            {
              picked_block_in_selected_blocks = true;
            }
            pickedBlocks.push_back(static_cast<unsigned int>(index));
          }
        }
      }
    }
  }

  if (!picked_block_in_selected_blocks)
  {
    // the block that was clicked on is not one of the currently selected
    // block so actions should only affect that block
    pickedBlocks.clear();
    pickedBlocks.append(static_cast<unsigned int>(blockIndex));
  }

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
    if (mbldr && mbldr->contextMenu(this->Menu, view, this->Position, repr, pickedBlocks))
    {
      break;
    }
  }
}
