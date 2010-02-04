/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSurfaceLICDisplayPanelDecoratorImplementation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqSurfaceLICDisplayPanelDecoratorImplementation.h"

// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqDisplayProxyEditor.h"
#include "pqSurfaceLICDisplayPanelDecorator.h"

//-----------------------------------------------------------------------------
pqSurfaceLICDisplayPanelDecoratorImplementation::
  pqSurfaceLICDisplayPanelDecoratorImplementation(QObject* p): QObject(p)
{
}

//-----------------------------------------------------------------------------
pqSurfaceLICDisplayPanelDecoratorImplementation::~pqSurfaceLICDisplayPanelDecoratorImplementation()
{
}

//-----------------------------------------------------------------------------
bool pqSurfaceLICDisplayPanelDecoratorImplementation::canDecorate(
  pqDisplayPanel* panel) const
{
  pqDisplayProxyEditor *editor = qobject_cast<pqDisplayProxyEditor*>(panel);
  return (editor != 0);
}

//-----------------------------------------------------------------------------
void pqSurfaceLICDisplayPanelDecoratorImplementation::decorate(
  pqDisplayPanel* panel) const
{
  pqDisplayProxyEditor *editor = qobject_cast<pqDisplayProxyEditor*>(panel);
  new pqSurfaceLICDisplayPanelDecorator(editor);
}
