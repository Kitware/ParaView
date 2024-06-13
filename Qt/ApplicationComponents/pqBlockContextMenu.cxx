// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBlockContextMenu.h"

#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqDataRepresentation.h"
#include "pqDoubleRangeDialog.h"
#include "pqUndoStack.h"
#include "vtkDataAssembly.h"
#include "vtkPVDataInformation.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"

#include <QColorDialog>
#include <QMenu>
#include <QtDebug>

#include <algorithm>
#include <array>
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

  const auto assemblyName = vtkSMPropertyHelper(repr->getProxy(), "Assembly").GetAsString();

  const auto blockNames = dataInfo->GetBlockNames(dataBlockContext, assemblyName);
  if (blockNames.empty())
  {
    return false;
  }

  // add header for block name.
  const QString title = blockNames.size() == 1 ? tr("Block '%1'").arg(blockNames[0].c_str())
                                               : tr("%1 Blocks").arg(blockNames.size());
  menu->addAction(title);
  menu->addSeparator();

  auto visibilityProperty =
    vtkSMStringVectorProperty::SafeDownCast(reprProxy->GetProperty("BlockVisibilities"));
  visibilityProperty = visibilityProperty
    ? visibilityProperty
    : vtkSMStringVectorProperty::SafeDownCast(reprProxy->GetProperty("BlockSelectors"));
  if (auto smProperty = visibilityProperty)
  {
    menu->addAction(QIcon(":/pqWidgets/Icons/pqEyeballClosed.svg"), tr("Hide Block"), [=]() {
      // use pqDataAssemblyTreeModel to avoid having to handle the complexity of
      // updating properties respecting hierarchical relationships.
      const auto assembly = dataInfo->GetDataAssembly(assemblyName);
      auto model = new pqDataAssemblyTreeModel();
      model->setUserCheckable(true);
      model->setDataAssembly(assembly);
      model->setCheckedNodes({ "/" });
      const auto nodes = assembly->SelectNodes(dataBlockContext);
      QList<int> iListNodes;
      std::copy(nodes.begin(), nodes.end(), std::back_inserter(iListNodes));
      auto indexes = model->index(iListNodes);
      for (auto& idx : indexes)
      {
        model->setData(idx, Qt::Unchecked, Qt::CheckStateRole);
      }

      const auto checkedNodes = model->checkedNodes();
      std::vector<std::string> checkedValues(checkedNodes.size());
      std::transform(checkedNodes.begin(), checkedNodes.end(), checkedValues.begin(),
        [](const QString& qstring) { return qstring.toStdString(); });

      const std::vector<std::string>& prevValues = smProperty->GetElements();

      std::vector<std::string> finalValues;
      const auto rootName = std::string("/") + assembly->GetRootNodeName();
      if (!prevValues.empty() && prevValues[0] != "/" && prevValues[0] != rootName)
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

        for (const auto& val : checkedValues)
        {
          const auto valNodeId = assembly->GetFirstNodeByPath(val.c_str());
          const auto parentValNodeId = assembly->GetParent(valNodeId);
          // check if val exists in the dictionary
          const bool foundPath = prevValuesDictionary.find(val) != prevValuesDictionary.end();
          // check if a parent path of val exists in the dictionary
          const auto parentPath = std::find_if(
            prevValuesDictionary.begin(), prevValuesDictionary.end(), [&](const std::string& str) {
              return assembly->GetFirstNodeByPath(str.c_str()) == parentValNodeId;
            });
          const bool foundParentPath = parentPath != prevValuesDictionary.end();
          // check if a child path of val exists in the dictionary
          const auto childPath = std::find_if(
            prevValuesDictionary.begin(), prevValuesDictionary.end(), [&](const std::string& str) {
              return assembly->GetParent(assembly->GetFirstNodeByPath(str.c_str())) == valNodeId;
            });
          const bool foundChildPath = childPath != prevValuesDictionary.end();
          if (foundPath || foundParentPath)
          {
            finalValues.push_back(val);
          }
          else if (foundChildPath)
          {
            finalValues.push_back(*childPath);
          }
        }
      }
      else
      {
        finalValues = checkedValues;
      }

      SM_SCOPED_TRACE(PropertiesModified).arg("proxy", reprProxy);
      BEGIN_UNDO_SET(tr("Hide Block"));
      smProperty->SetElements(finalValues);
      reprProxy->UpdateVTKObjects();
      END_UNDO_SET();
      repr->renderViewEventually();
      delete model;
    });

    menu->addAction(QIcon(":/pqWidgets/Icons/pqEyeball.svg"), tr("Show Only Block"),
      [dataBlockContext, smProperty, reprProxy, repr]() {
        SM_SCOPED_TRACE(PropertiesModified).arg("proxy", reprProxy);
        BEGIN_UNDO_SET(tr("Show Only Block"));
        smProperty->SetElements(dataBlockContext);
        reprProxy->UpdateVTKObjects();
        END_UNDO_SET();
        repr->renderViewEventually();
      });

    menu->addAction(QIcon(":/pqWidgets/Icons/pqEyeball.svg"), tr("Show All Blocks"),
      [smProperty, reprProxy, repr]() {
        SM_SCOPED_TRACE(PropertiesModified).arg("proxy", reprProxy);
        BEGIN_UNDO_SET(tr("Show All Blocks"));
        smProperty->SetElements(std::vector<std::string>({ "/" }));
        reprProxy->UpdateVTKObjects();
        END_UNDO_SET();
        repr->renderViewEventually();
      });

    menu->addSeparator();
  }
  if (auto smProperty = reprProxy->GetProperty("BlockColors"))
  {
    menu->addAction(QIcon(":/pqWidgets/Icons/explicit_color.png"), tr("Set Block Color"), [=]() {
      std::array<double, 3> rgb = vtkSMColorMapEditorHelper::GetColor(reprProxy);
      if (!dataBlockContext.empty())
      {
        auto blockRGB = vtkSMColorMapEditorHelper::GetBlockColor(reprProxy, dataBlockContext[0]);
        if (vtkSMColorMapEditorHelper::IsColorValid(blockRGB))
        {
          rgb = blockRGB;
        }
      }
      // it's potentially expensive to get the current color and hence I am just
      // using QColor here as the default color for the QColorDialog.
      const QColor color = QColorDialog::getColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]),
        pqCoreUtilities::mainWidget(), tr("Set Block Color"), QColorDialog::DontUseNativeDialog);
      if (color.isValid())
      {
        double colorF[3] = { color.redF(), color.greenF(), color.blueF() };
        vtkSMPropertyHelper helper(smProperty);
        SM_SCOPED_TRACE(PropertiesModified).arg("proxy", reprProxy);
        BEGIN_UNDO_SET(tr("Set Block Color"));
        for (const auto& selector : dataBlockContext)
        {
          helper.SetStatus(selector.c_str(), colorF, 3);
        }
        reprProxy->UpdateVTKObjects();
        END_UNDO_SET();
        repr->renderViewEventually();
      }
    });
    menu->addAction(QIcon(":/pqWidgets/Icons/inherited_color.png"), tr("Unset Block Color"), [=]() {
      SM_SCOPED_TRACE(PropertiesModified).arg("proxy", reprProxy);
      BEGIN_UNDO_SET(tr("Unset Block Color"));
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
    menu->addAction(
      QIcon(":/pqWidgets/Icons/explicit_opacity.png"), tr("Set Block Opacity"), [=]() {
        pqDoubleRangeDialog dialog(tr("Opacity:"), 0.0, 1.0, pqCoreUtilities::mainWidget());
        dialog.setObjectName("OpacityDialog");
        dialog.setWindowTitle(tr("Set Block Opacity"));
        dialog.setValue(1.0);
        if (dialog.exec() == QDialog::Accepted)
        {
          double opacity = qBound(0.0, dialog.value(), 1.0);
          vtkSMPropertyHelper helper(smProperty);
          SM_SCOPED_TRACE(PropertiesModified).arg("proxy", reprProxy);
          BEGIN_UNDO_SET(tr("Set Block Opacity"));
          for (const auto& selector : dataBlockContext)
          {
            helper.SetStatus(selector.c_str(), &opacity, 1);
          }
          reprProxy->UpdateVTKObjects();
          END_UNDO_SET();
          repr->renderViewEventually();
        }
      });

    menu->addAction(
      QIcon(":/pqWidgets/Icons/inherited_opacity.png"), tr("Unset Block Opacity"), [=]() {
        SM_SCOPED_TRACE(PropertiesModified).arg("proxy", reprProxy);
        BEGIN_UNDO_SET(tr("Unset Block Opacity"));
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
