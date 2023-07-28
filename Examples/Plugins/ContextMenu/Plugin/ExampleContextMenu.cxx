// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "ExampleContextMenu.h"

#include "pqDataRepresentation.h"
#include "pqPipelineSource.h"

#include <QAction>
#include <QMenu>
#include <QString>

#include <iostream>

//-----------------------------------------------------------------------------
ExampleContextMenu::ExampleContextMenu() = default;

//-----------------------------------------------------------------------------
ExampleContextMenu::ExampleContextMenu(QObject* parent)
  : QObject(parent)
{
}

//-----------------------------------------------------------------------------
ExampleContextMenu::~ExampleContextMenu() = default;

bool ExampleContextMenu::contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
  pqRepresentation* dataContext, const QList<unsigned int>& dataBlockContext) const
{
  (void)viewContext;
  (void)viewPoint;
  (void)dataBlockContext;

  auto rep = dynamic_cast<pqDataRepresentation*>(dataContext);
  auto inp = rep ? rep->getInput() : nullptr;

  // Only provide a context menu for box sources (i.e., pipeline
  // objects created by clicking on the "Sources->Alphabetical->Box"
  // menu item).
  //
  // Note that we could also make our menu dependent on the view type
  // or other arguments that we are currently ignoring.
  if (inp && inp->getSMName().startsWith("Box", Qt::CaseInsensitive) &&
    inp->getSMGroup() == "sources")
  {
    // Provide a single contextual action for boxes:
    QAction* twiddleThumbs = menu->addAction(QString("Twiddle thumbs"));
    QObject::connect(twiddleThumbs, SIGNAL(triggered()), this, SLOT(twiddleThumbsAction()));
    // Returning true here indicates that lower-priority pqContextMenuInterface objects
    // should *not* be given a chance to modify the context menu (i.e., terminate early).
    // Falling through and returning false allows other interfaces to add or even modify
    // existing menu entries.
    return true;
  }

  // When this plugin cannot provide a relevant context menu, it should return false.
  // This will give other plugins a chance to provide a menu; if they cannot, it
  // ParaView will provide a default.
  return false;
}

void ExampleContextMenu::twiddleThumbsAction()
{
  // Do something in response to user choosing our contextual
  // action. Note that we could have stored the click location,
  // data representation, and selected blocks as internal
  // state to be used by this action.
  std::cout << "Twiddling thumbs as we speak.\n";
}
