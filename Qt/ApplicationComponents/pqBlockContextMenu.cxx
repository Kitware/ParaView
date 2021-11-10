/*=========================================================================

   Program: ParaView
   Module:  pqBlockContextMenu.cxx

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
#include "pqBlockContextMenu.h"

#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqDataRepresentation.h"
#include "pqDoubleRangeDialog.h"
#include "pqUndoStack.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <QColorDialog>
#include <QMenu>
#include <QtDebug>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

//-----------------------------------------------------------------------------
pqBlockContextMenu::pqBlockContextMenu(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqBlockContextMenu::~pqBlockContextMenu() = default;

//-----------------------------------------------------------------------------
bool pqBlockContextMenu::contextMenu(QMenu* menu, pqView*, const QPoint&,
  pqRepresentation* dataContext, const QStringList& context) const
{
  std::vector<std::string> dataBlockContext(context.size());
  std::transform(context.begin(), context.end(), dataBlockContext.begin(),
    [](const QString& str) { return str.toStdString(); });

  if (dataBlockContext.empty())
  {
    return false;
  }

  auto reprProxy = dataContext->getProxy();
  auto repr = qobject_cast<pqDataRepresentation*>(dataContext);
  auto dataInfo = repr ? repr->getInputDataInformation() : nullptr;
  if (!dataInfo)
  {
    return false;
  }

  const auto blockNames =
    dataInfo->GetBlockNames(dataBlockContext, vtkDataAssemblyUtilities::HierarchyName());
  if (blockNames.empty())
  {
    return false;
  }

  // add header for block name.
  const QString title = blockNames.size() == 1 ? QString("Block '%1'").arg(blockNames[0].c_str())
                                               : QString("%1 Blocks").arg(blockNames.size());
  menu->addAction(title);
  menu->addSeparator();

  auto visibilityProperty =
    vtkSMStringVectorProperty::SafeDownCast(reprProxy->GetProperty("BlockVisibilities"));
  visibilityProperty = visibilityProperty
    ? visibilityProperty
    : vtkSMStringVectorProperty::SafeDownCast(reprProxy->GetProperty("BlockSelectors"));
  if (auto smProperty = visibilityProperty)
  {
    menu->addAction(QIcon(":/pqWidgets/Icons/pqEyeballClosed.svg"), "Hide Block", [=]() {
      // use pqDataAssemblyTreeModel to avoid having to handle the complexity of
      // updating properties respecting hierarchical relationships.
      const auto hierarchy = dataInfo->GetHierarchy();
      auto model = new pqDataAssemblyTreeModel();
      model->setUserCheckable(true);
      model->setDataAssembly(hierarchy);
      model->setCheckedNodes({ "/" });
      const auto nodes = hierarchy->SelectNodes(dataBlockContext);
      QList<int> iListNodes;
      std::copy(nodes.begin(), nodes.end(), std::back_inserter(iListNodes));
      auto indexes = model->index(iListNodes);
      for (auto& idx : indexes)
      {
        model->setData(idx, Qt::Unchecked, Qt::CheckStateRole);
      }

      const auto checkedNodes = model->checkedNodes();
      std::vector<std::string> value(checkedNodes.size());
      std::transform(checkedNodes.begin(), checkedNodes.end(), value.begin(),
        [](const QString& qstring) { return qstring.toStdString(); });

      const std::vector<std::string>& prevValues = smProperty->GetElements();

      if (!prevValues.empty() && prevValues[0] != "/" && prevValues[0] != "/Root")
      {
        // If there were blocks that were already hidden in the past, we want to keep them hidden.
        // We don't have the information in the parameters of the method about the previous state,
        // all we have is the current state of smProperty.
        // So what we do is we put all strings from smProperty in a std::unordered_set<std::string>.
        // We use this dictionary to do fast queries from the list of blocks that should be
        // displayed (which are all in the `value` container).
        // If a string is not in smProperty but is in value, then it was hidden in the past, so we
        // delete this from value.

        std::unordered_set<std::string> prevValuesDictionary;

        for (const std::string& val : prevValues)
        {
          prevValuesDictionary.insert(val);
        }

        for (auto it = value.begin(); it != value.end();)
        {
          if (!prevValuesDictionary.count(*it))
          {
            it = value.erase(it);
          }
          else
          {
            ++it;
          }
        }
      }

      BEGIN_UNDO_SET("Hide Block");
      smProperty->SetElements(value);
      reprProxy->UpdateVTKObjects();
      END_UNDO_SET();
      repr->renderViewEventually();
      delete model;
    });

    menu->addAction(QIcon(":/pqWidgets/Icons/pqEyeball.svg"), "Show Only Block",
      [dataBlockContext, smProperty, reprProxy, repr]() {
        BEGIN_UNDO_SET("Show Only Block");
        smProperty->SetElements(dataBlockContext);
        reprProxy->UpdateVTKObjects();
        END_UNDO_SET();
        repr->renderViewEventually();
      });

    menu->addAction(
      QIcon(":/pqWidgets/Icons/pqEyeball.svg"), "Show All Blocks", [smProperty, reprProxy, repr]() {
        BEGIN_UNDO_SET("Show All Blocks");
        smProperty->SetElements(std::vector<std::string>({ "/" }));
        reprProxy->UpdateVTKObjects();
        END_UNDO_SET();
        repr->renderViewEventually();
      });

    menu->addSeparator();
  }
  if (auto smProperty = reprProxy->GetProperty("BlockColors"))
  {
    menu->addAction(QIcon(":/pqWidgets/Icons/explicit_color.png"), "Set Block Color", [=]() {
      // it's potentially expensive to get the current color and hence I am just
      // using QColor here as the default color for the QColorDialog.
      QColor color = QColorDialog::getColor(QColor(), pqCoreUtilities::mainWidget(),
        "Set Block Color", QColorDialog::DontUseNativeDialog);
      if (color.isValid())
      {
        double colorF[3] = { color.redF(), color.greenF(), color.blueF() };
        vtkSMPropertyHelper helper(smProperty);
        BEGIN_UNDO_SET("Set Block Color");
        for (const auto& selector : dataBlockContext)
        {
          helper.SetStatus(selector.c_str(), colorF, 3);
        }
        reprProxy->UpdateVTKObjects();
        END_UNDO_SET();
        repr->renderViewEventually();
      }
    });
    menu->addAction(QIcon(":/pqWidgets/Icons/inherited_color.png"), "Unset Block Color", [=]() {
      BEGIN_UNDO_SET("Unset Block Color");
      vtkSMPropertyHelper helper(smProperty);
      for (const auto& selector : dataBlockContext)
      {
        helper.RemoveStatus(selector.c_str());
      }
      reprProxy->UpdateVTKObjects();
      END_UNDO_SET();
      repr->renderViewEventually();
    });
    menu->addSeparator();
  }
  if (auto smProperty = reprProxy->GetProperty("BlockOpacities"))
  {
    menu->addAction(QIcon(":/pqWidgets/Icons/explicit_opacity.png"), "Set Block Opacity", [=]() {
      pqDoubleRangeDialog dialog("Opacity:", 0.0, 1.0, pqCoreUtilities::mainWidget());
      dialog.setObjectName("OpacityDialog");
      dialog.setWindowTitle("Set Block Opacity");
      dialog.setValue(1.0);
      if (dialog.exec() == QDialog::Accepted)
      {
        double opacity = qBound(0.0, dialog.value(), 1.0);
        vtkSMPropertyHelper helper(smProperty);
        BEGIN_UNDO_SET("Set Block Opacity");
        for (const auto& selector : dataBlockContext)
        {
          helper.SetStatus(selector.c_str(), &opacity, 1);
        }
        reprProxy->UpdateVTKObjects();
        END_UNDO_SET();
        repr->renderViewEventually();
      }
    });

    menu->addAction(QIcon(":/pqWidgets/Icons/inherited_opacity.png"), "Unset Block Opacity", [=]() {
      BEGIN_UNDO_SET("Unset Block Color");
      vtkSMPropertyHelper helper(smProperty);
      for (const auto& selector : dataBlockContext)
      {
        helper.RemoveStatus(selector.c_str());
      }
      reprProxy->UpdateVTKObjects();
      END_UNDO_SET();
      repr->renderViewEventually();
    });
    menu->addSeparator();
  }
  return false;
}
