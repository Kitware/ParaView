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
#include "pqEditColorMapReaction.h"
#include "pqPipelineRepresentation.h"
#include "pqMultiBlockInspectorPanel.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
#include "pqSelectionManager.h"
#include "pqPVApplicationCore.h"
#include "vtkSMProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

#include <QAction>
#include <QApplication>
#include <QColorDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QRegExp>
#include <QWidget>

namespace
{
  const QStringList colorFieldData(const QString& field, int component)
    {
    QStringList result;
    result << field << QString::number(component);
    return result;
    }

  bool extractColorFieldData(const QVariant& variant, QString& field, int& component)
    {
    QStringList list = variant.toStringList();
    if (list.size() == 2)
      {
      field = list[0];
      component = list[1].toInt();
      return true;
      }
    return false;
    }

  pqMultiBlockInspectorPanel* getMultiBlockInspectorPanel()
  {
    // get multi-block inspector panel
    pqMultiBlockInspectorPanel *panel = 0;
    foreach(QWidget *widget, qApp->topLevelWidgets())
      {
      panel = widget->findChild<pqMultiBlockInspectorPanel *>();

      if(panel)
        {
        break;
        }
      }
    return panel;
  }
}

//-----------------------------------------------------------------------------
pqPipelineContextMenuBehavior::pqPipelineContextMenuBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(viewAdded(pqView*)),
    this, SLOT(onViewAdded(pqView*)));
  this->Menu = new QMenu();
  this->Menu << pqSetName("PipelineContextMenu");
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
    view->getWidget()->installEventFilter(this);
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
      if (delta.manhattanLength() < 3 && senderWidget != NULL)
        {
        pqRenderView* view = qobject_cast<pqRenderView*>(
          pqActiveObjects::instance().activeView());
        if (view)
          {
          int pos[2] = { newPos.x(), newPos.y() } ;
          // we need to flip Y.
          int height = senderWidget->size().height();
          pos[1] = height - pos[1];
          unsigned int blockIndex = 0;
          this->PickedRepresentation = view->pickBlock(pos, blockIndex);
          this->buildMenu(this->PickedRepresentation, blockIndex);
          this->Menu->popup(senderWidget->mapToGlobal(newPos));
          }
        }
      this->Position = QPoint();
      }
    }

  return Superclass::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::buildMenu(pqDataRepresentation* repr,
                                              unsigned int blockIndex)
{
  pqRenderView* view = qobject_cast<pqRenderView*>(
    pqActiveObjects::instance().activeView());  

  // get currently selected block ids
  this->PickedBlocks.clear();

  bool picked_block_in_selected_blocks = false;
  pqSelectionManager *selectionManager =
    pqPVApplicationCore::instance()->selectionManager();
  if(selectionManager)
    {
    pqOutputPort *port = selectionManager->getSelectedPort();
    if(port)
      {
      vtkSMSourceProxy *activeSelection = port->getSelectionInput();
      if(activeSelection &&
         strcmp(activeSelection->GetXMLName(), "BlockSelectionSource") == 0)
        {
        vtkSMPropertyHelper blocksProp(activeSelection, "Blocks");
        QVector <vtkIdType> vblocks;
        vblocks.resize(blocksProp.GetNumberOfElements());
        blocksProp.Get(&vblocks[0], blocksProp.GetNumberOfElements());
        foreach (const vtkIdType &index, vblocks)
          {
          if (index >= 0)
            {
            if (static_cast<unsigned int>(index) == blockIndex)
              {
              picked_block_in_selected_blocks = true;
              }
            this->PickedBlocks.push_back(static_cast<unsigned int>(index));
            }
          }
        }
      }
    }

  if (!picked_block_in_selected_blocks)
    {
    // the block that was clicked on is not one of the currently selected
    // block so actions should only affect that block
    this->PickedBlocks.clear();
    this->PickedBlocks.append(static_cast<unsigned int>(blockIndex));
    }

  if (repr)
    {
    this->Menu->clear();

    vtkPVDataInformation *info = repr->getInputDataInformation();
    vtkPVCompositeDataInformation *compositeInfo = info->GetCompositeDataInformation();
    if(compositeInfo && compositeInfo->GetDataIsComposite())
      {
      bool multipleBlocks = this->PickedBlocks.size() > 1;

      if(multipleBlocks)
        {
        this->Menu->addAction(QString("%1 Blocks").arg(this->PickedBlocks.size()));
        }
      else
        {
        QString blockName = this->lookupBlockName(blockIndex);
        this->Menu->addAction(QString("Block '%1'").arg(blockName));
        }
      this->Menu->addSeparator();

      QAction *hideBlockAction =
        this->Menu->addAction(QString("Hide Block%1").arg(multipleBlocks ? "s" : ""));
      this->connect(hideBlockAction, SIGNAL(triggered()),
                    this, SLOT(hideBlock()));

      QAction *showOnlyBlockAction =
        this->Menu->addAction(QString("Show Only Block%1").arg(multipleBlocks ? "s" : ""));
      this->connect(showOnlyBlockAction, SIGNAL(triggered()),
                    this, SLOT(showOnlyBlock()));

      QAction *showAllBlocksAction =
        this->Menu->addAction("Show All Blocks");
      this->connect(showAllBlocksAction, SIGNAL(triggered()),
                    this, SLOT(showAllBlocks()));

      QAction *unsetVisibilityAction =
        this->Menu->addAction(QString("Unset Block %1")
            .arg(multipleBlocks ? "Visibilities" : "Visibility"));
      this->connect(unsetVisibilityAction, SIGNAL(triggered()),
                    this, SLOT(unsetBlockVisibility()));

      this->Menu->addSeparator();

      QAction *setBlockColorAction =
        this->Menu->addAction(QString("Set Block Color%1")
          .arg(multipleBlocks ? "s" : ""));
      this->connect(setBlockColorAction, SIGNAL(triggered()),
                    this, SLOT(setBlockColor()));

      QAction *unsetBlockColorAction =
        this->Menu->addAction(QString("Unset Block Color%1")
          .arg(multipleBlocks ? "s" : ""));
      this->connect(unsetBlockColorAction, SIGNAL(triggered()),
                    this, SLOT(unsetBlockColor()));

      this->Menu->addSeparator();

      QAction *setBlockOpacityAction =
        this->Menu->addAction(QString("Set Block %1")
          .arg(multipleBlocks ? "Opacities" : "Opacity"));
      this->connect(setBlockOpacityAction, SIGNAL(triggered()),
                    this, SLOT(setBlockOpacity()));

      QAction *unsetBlockOpacityAction =
        this->Menu->addAction(QString("Unset Block %1")
            .arg(multipleBlocks ? "Opacities" : "Opacity"));
      this->connect(unsetBlockOpacityAction, SIGNAL(triggered()),
                    this, SLOT(unsetBlockOpacity()));

      this->Menu->addSeparator();
      }

    QAction* action;
    action = this->Menu->addAction("Hide");
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(hide()));

    QMenu* reprMenu = this->Menu->addMenu("Representation")
      << pqSetName("Representation");

    // populate the representation types menu.
    QList<QVariant> rTypes = pqSMAdaptor::getEnumerationPropertyDomain(
      repr->getProxy()->GetProperty("Representation"));
    QVariant curRType = pqSMAdaptor::getEnumerationProperty(
      repr->getProxy()->GetProperty("Representation"));
    foreach (QVariant rtype, rTypes)
      {
      QAction* raction = reprMenu->addAction(rtype.toString());
      raction->setCheckable(true);
      raction->setChecked(rtype == curRType);
      }

    QObject::connect(reprMenu, SIGNAL(triggered(QAction*)),
      this, SLOT(reprTypeChanged(QAction*)));

    this->Menu->addSeparator();

    pqPipelineRepresentation* pipelineRepr =
      qobject_cast<pqPipelineRepresentation*>(repr);

    if (pipelineRepr)
      {
      QMenu* colorFieldsMenu = this->Menu->addMenu("Color By")
        << pqSetName("ColorBy");
      this->buildColorFieldsMenu(pipelineRepr, colorFieldsMenu);
      }

    action = this->Menu->addAction("Edit Color");
    new pqEditColorMapReaction(action);
    }
  else
    {
      // when nothing was picked we show the "link camera" menu.
      this->Menu->clear();
      this->Menu->addAction("Link Camera...",
        view, SLOT(linkToOtherView()));
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::buildColorFieldsMenu(
  pqPipelineRepresentation* pipelineRepr, QMenu* menu)
{
  QObject::connect(menu, SIGNAL(triggered(QAction*)),
    this, SLOT(colorMenuTriggered(QAction*)), Qt::QueuedConnection);

  QIcon cellDataIcon(":/pqWidgets/Icons/pqCellData16.png");
  QIcon pointDataIcon(":/pqWidgets/Icons/pqPointData16.png");
  QIcon solidColorIcon(":/pqWidgets/Icons/pqSolidColor16.png");

  QList<QString> available_fields = pipelineRepr->getColorFields();
  QRegExp regExpCell(" \\(cell\\)\\w*$");
  QRegExp regExpPoint(" \\(point\\)\\w*$");
  foreach (QString field, available_fields)
    {
    if (field == pqPipelineRepresentation::solidColor())
      {
      menu->addAction(solidColorIcon, field)->setData(
        colorFieldData(field, -1));
      continue;
      }

    int num_components = pipelineRepr->getColorFieldNumberOfComponents(field);
    QString arrayname = field;
    bool cell_data = false;
    if (regExpCell.indexIn(field) != -1)
      {
      cell_data = true;
      arrayname.replace(regExpCell, "");
      }
    else if (regExpPoint.indexIn(field) != -1)
      {
      arrayname.replace(regExpCell, "");
      }

    if (num_components == 1)
      {
      QAction* c_action = menu->addAction(cell_data? cellDataIcon: pointDataIcon,
        arrayname);
      c_action << pqSetName(field);
      c_action->setData(colorFieldData(field, -1));
      }
    else if (num_components > 1)
      {
      QMenu* component_menu = menu->addMenu(cell_data?
        cellDataIcon: pointDataIcon, arrayname) << pqSetName(field);
      QObject::connect(menu, SIGNAL(triggered(QAction*)),
        this, SLOT(colorMenuTriggered(QAction*)), Qt::QueuedConnection);
      QAction* c_action = component_menu->addAction("Magnitude");
      c_action->setData(colorFieldData(field, -1));
      c_action << pqSetName("-1");
      for (int cc=0; cc < num_components; cc++)
        {
        QString component_name =
          pipelineRepr->getColorFieldComponentName(field, cc);
        c_action = component_menu->addAction(component_name.isEmpty()?
          QString::number(cc): component_name);
        c_action << pqSetName(component_name);
        c_action->setData(colorFieldData(field, cc));
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::colorMenuTriggered(QAction* action)
{
  QString field;
  int component;
  if (!extractColorFieldData(action->data(), field, component))
    {
    return;
    }

  pqPipelineRepresentation* repr = qobject_cast<pqPipelineRepresentation*>(
    this->PickedRepresentation);
  if (repr)
    {
    BEGIN_UNDO_SET("Color Changed");
    repr->setColorField(field);
    pqScalarsToColors* lut = repr->getLookupTable();
    if (lut)
      {
      if (component == -1)
        {
        lut->setVectorMode(pqScalarsToColors::MAGNITUDE, -1);
        }
      else
        {
        lut->setVectorMode(pqScalarsToColors::COMPONENT, component);
        lut->updateScalarBarTitles(
          repr->getColorFieldComponentName(field, component));
        }
      repr->resetLookupTableScalarRange();
      }
    repr->renderViewEventually();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::reprTypeChanged(QAction* action)
{
  pqDataRepresentation* repr = this->PickedRepresentation;
  if (repr)
    {
    BEGIN_UNDO_SET("Representation Type Changed");
    pqSMAdaptor::setEnumerationProperty(
      repr->getProxy()->GetProperty("Representation"),
      action->text());
    repr->getProxy()->UpdateVTKObjects();
    repr->renderViewEventually();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::hide()
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

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::hideBlock()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if(!action)
    {
    return;
    }

  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if (panel)
    {
    panel->setBlockVisibility(this->PickedBlocks, false);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::showOnlyBlock()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if(!action)
    {
    return;
    }

  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if (panel)
    {
    panel->showOnlyBlocks(this->PickedBlocks);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::showAllBlocks()
{
  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if (panel)
    {
    panel->showAllBlocks();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::unsetBlockVisibility()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if(!action)
    {
    return;
    }

  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if(panel)
    {
    panel->clearBlockVisibility(this->PickedBlocks);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::setBlockColor()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if(!action)
    {
    return;
    }

  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if (panel)
    {
    QColor color = QColorDialog::getColor(QColor(), panel, "Choose Block Color",
      QColorDialog::DontUseNativeDialog);
    if(color.isValid())
      {
      panel->setBlockColor(this->PickedBlocks, color);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::unsetBlockColor()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if(!action)
    {
    return;
    }

  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if(panel)
    {
    panel->clearBlockColor(this->PickedBlocks);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::setBlockOpacity()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if(!action)
    {
    return;
    }

  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if(panel)
    {
    panel->promptAndSetBlockOpacity(this->PickedBlocks);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::unsetBlockOpacity()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if(!action)
    {
    return;
    }

  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if(panel)
    {
    panel->clearBlockOpacity(this->PickedBlocks);
    }
}

//-----------------------------------------------------------------------------
QString pqPipelineContextMenuBehavior::lookupBlockName(unsigned int flatIndex) const
{
  pqMultiBlockInspectorPanel *panel = getMultiBlockInspectorPanel();
  if(panel)
    {
    return panel->lookupBlockName(flatIndex);
    }
  else
    {
    return QString();
    }
}
