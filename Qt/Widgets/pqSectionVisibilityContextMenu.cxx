// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSectionVisibilityContextMenu.h"

#include <QHeaderView>

#include "pqSetName.h"

//-----------------------------------------------------------------------------
pqSectionVisibilityContextMenu::pqSectionVisibilityContextMenu(QWidget* _p)
  : QMenu(_p)
{
  this->HeaderView = nullptr;
  QObject::connect(
    this, SIGNAL(triggered(QAction*)), this, SLOT(toggleSectionVisibility(QAction*)));
}

//-----------------------------------------------------------------------------
pqSectionVisibilityContextMenu::~pqSectionVisibilityContextMenu() = default;

//-----------------------------------------------------------------------------
void pqSectionVisibilityContextMenu::setHeaderView(QHeaderView* header)
{
  this->clear();
  this->HeaderView = header;
  if (header)
  {
    QAbstractItemModel* model = header->model();
    for (int cc = 0; cc < header->count(); cc++)
    {
      QString headertext = model->headerData(cc, header->orientation()).toString();
      QAction* action = this->addAction(headertext) << pqSetName(headertext);
      action->setCheckable(true);
      action->setChecked(!header->isSectionHidden(cc));
    }
  }
}

//-----------------------------------------------------------------------------
void pqSectionVisibilityContextMenu::toggleSectionVisibility(QAction* action)
{
  QHeaderView* header = this->HeaderView;
  if (!header)
  {
    return;
  }
  QString headertext = action->text();
  QAbstractItemModel* model = header->model();
  for (int cc = 0; cc < header->count(); cc++)
  {
    if (headertext == model->headerData(cc, Qt::Horizontal).toString())
    {
      if (action->isChecked())
      {
        header->showSection(cc);
      }
      else
      {
        header->hideSection(cc);
      }
      break;
    }
  }
}
