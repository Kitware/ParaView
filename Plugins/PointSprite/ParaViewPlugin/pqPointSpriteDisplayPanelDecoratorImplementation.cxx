/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqPointSpriteDisplayPanelDecoratorImplementation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqPointSpriteDisplayPanelDecoratorImplementation
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "pqPointSpriteDisplayPanelDecoratorImplementation.h"

// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqDisplayProxyEditor.h"
#include "pqPointSpriteDisplayPanelDecorator.h"
#include "vtkProxyManagerExtension.h"
#include "vtkSMProxyManager.h"

#include <iostream>

//-----------------------------------------------------------------------------
pqPointSpriteDisplayPanelDecoratorImplementation::
  pqPointSpriteDisplayPanelDecoratorImplementation(QObject* p): QObject(p)
{
  vtkProxyManagerExtension* ext = vtkProxyManagerExtension::New();
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->RegisterExtension(ext);
  ext->Delete();
}

//-----------------------------------------------------------------------------
pqPointSpriteDisplayPanelDecoratorImplementation::~pqPointSpriteDisplayPanelDecoratorImplementation()
{
}

//-----------------------------------------------------------------------------
bool pqPointSpriteDisplayPanelDecoratorImplementation::canDecorate(
  pqDisplayPanel* panel) const
{
  pqDisplayProxyEditor *editor = qobject_cast<pqDisplayProxyEditor*>(panel);
  return (editor != 0);
}

//-----------------------------------------------------------------------------
void pqPointSpriteDisplayPanelDecoratorImplementation::decorate(
  pqDisplayPanel* panel) const
{
  pqDisplayProxyEditor *editor = qobject_cast<pqDisplayProxyEditor*>(panel);
  new pqPointSpriteDisplayPanelDecorator(editor);
}
