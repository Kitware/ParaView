/*=========================================================================

   Program: ParaView
   Module:  ExampleContextMenu.cxx

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
#include "ExampleContextMenu.h"

#include "pqDataRepresentation.h"
#include "pqPipelineSource.h"

#include <QAction>
#include <QMenu>
#include <QString>

#include <iostream>

//-----------------------------------------------------------------------------
ExampleContextMenu::ExampleContextMenu()
{
}

//-----------------------------------------------------------------------------
ExampleContextMenu::ExampleContextMenu(QObject* parent)
  : QObject(parent)
{
}

//-----------------------------------------------------------------------------
ExampleContextMenu::~ExampleContextMenu()
{
}

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
