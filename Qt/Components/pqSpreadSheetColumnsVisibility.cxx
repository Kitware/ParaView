/*=========================================================================

Program: ParaView
Module:  pqSpreadSheetColumnsVisibility.cxx

Copyright (c) Kitware Inc.
All rights reserved.

See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

========================================================================*/

#include "pqSpreadSheetColumnsVisibility.h"

#include "pqUndoStack.h"

#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"

#include <QHBoxLayout>
#include <QWidget>

#include <algorithm>
#include <memory>
#include <set>

//-----------------------------------------------------------------------------
QCheckBox* pqSpreadSheetColumnsVisibility::addCheckableAction(
  QMenu* menu, const QString& text, const bool checked)
{
  QCheckBox* cb = new QCheckBox();
  cb->setObjectName("CheckBox");
  cb->setText(text);
  cb->setChecked(checked);
  // We need a layout to set margins - there are none for Plastic theme by default
  QHBoxLayout* layout = new QHBoxLayout();
  layout->addWidget(cb);
  layout->setContentsMargins(4, 2, 4, 2);
  QWidget* widget = new QWidget(menu);
  widget->setObjectName(text);
  widget->setLayout(layout);
  QWidgetAction* action = new QWidgetAction(menu);
  action->setText(text);
  action->setDefaultWidget(widget);
  menu->addAction(action);
  return cb;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetColumnsVisibility::updateAllCheckState(
  QCheckBox* allCheckbox, const std::vector<QCheckBox*>& cboxes)
{
  size_t checked = 0;
  for (auto cb : cboxes)
  {
    checked += (cb->isChecked() ? 1 : 0);
  }

  QSignalBlocker sblocker(allCheckbox);
  if (checked == cboxes.size())
  {
    allCheckbox->setCheckState(Qt::Checked);
    allCheckbox->setTristate(false);
  }
  else if (checked == 0)
  {
    allCheckbox->setCheckState(Qt::Unchecked);
    allCheckbox->setTristate(false);
  }
  else
  {
    allCheckbox->setCheckState(Qt::PartiallyChecked);
  }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetColumnsVisibility::populateMenu(
  vtkSMViewProxy* proxy, pqSpreadSheetViewModel* model, QMenu* menu)
{
  menu->clear();

  std::vector<std::pair<std::string, bool>> columnLabels;
  std::set<std::string> columnLabelsSet;
  for (int col = 0, max = model->columnCount(); col < max; ++col)
  {
    if (model->headerData(col, Qt::Horizontal, pqSpreadSheetViewModel::SectionInternal).toBool())
    {
      continue; // skip internal columns.
    }
    else
    {
      const std::string label =
        model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString().toUtf8().data();
      bool checked =
        model->headerData(col, Qt::Horizontal, pqSpreadSheetViewModel::SectionVisible).toBool();
      if (columnLabelsSet.insert(label).second == true)
      {
        columnLabels.push_back(std::make_pair(label, checked));
      }
    }
  }

  // add a separator
  columnLabels.push_back(std::make_pair(std::string(), false));

  // if there are any columns already hidden that are not already added, we
  // add them so that the user can always unhide them.
  auto svp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("HiddenColumnLabels"));
  for (unsigned int cc = 0, max = svp->GetNumberOfElements(); cc < max; ++cc)
  {
    auto txt = svp->GetElement(cc);
    if (columnLabelsSet.insert(txt).second == true)
    {
      columnLabels.push_back(std::make_pair(txt, false));
    }
  }

  if (columnLabels.size() <= 1) // there's always 1 separator.
  {
    // menu is empty.
    return;
  }

  auto allCheckbox = addCheckableAction(menu, "All Columns", false);
  menu->addSeparator();
  auto checkboxes = std::make_shared<std::vector<QCheckBox*>>();

  for (const auto& pair : columnLabels)
  {
    if (pair.first.empty())
    {
      menu->addSeparator();
    }
    else
    {
      const std::string& label = pair.first;
      const bool& checked = pair.second;
      auto callback = [proxy, label, checkboxes, allCheckbox](bool is_checked) {
        auto vsvp =
          vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("HiddenColumnLabels"));
        std::vector<std::string> values;
        for (unsigned int cc = 0, max = vsvp->GetNumberOfElements(); cc < max; ++cc)
        {
          values.push_back(vsvp->GetElement(cc));
        }
        if (is_checked)
        {
          values.erase(std::remove(values.begin(), values.end(), label), values.end());
        }
        else if (std::find(values.begin(), values.end(), label) == values.end())
        {
          values.push_back(label);
        }

        SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy);
        SCOPED_UNDO_SET("SpreadSheet column visibilities");
        vsvp->SetElements(values);
        proxy->UpdateVTKObjects();
        proxy->StillRender();

        updateAllCheckState(allCheckbox, *checkboxes);
      };

      auto cb = addCheckableAction(menu, label.c_str(), checked);
      QObject::connect(cb, &QCheckBox::toggled, callback);
      checkboxes->push_back(cb);
    }
  }

  updateAllCheckState(allCheckbox, *checkboxes);
  QObject::connect(
    allCheckbox, &QCheckBox::stateChanged, [proxy, checkboxes, allCheckbox](int checkState) {
      std::vector<std::string> hidden_columns;
      for (auto cb : (*checkboxes))
      {
        QSignalBlocker sblocker(cb);
        cb->setChecked(checkState == Qt::Checked);
        if (checkState != Qt::Checked)
        {
          // all columns are hidden.
          hidden_columns.push_back(cb->text().toUtf8().toStdString());
        }
      }

      // turn off tristate to avoid the `All Columns` checkbox from entering the
      // partially-checked state through user clicks.
      allCheckbox->setTristate(false);

      auto vsvp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("HiddenColumnLabels"));

      SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy);
      SCOPED_UNDO_SET("SpreadSheet column visibilities");
      vsvp->SetElements(hidden_columns);
      proxy->UpdateVTKObjects();
      proxy->StillRender();
    });
}
