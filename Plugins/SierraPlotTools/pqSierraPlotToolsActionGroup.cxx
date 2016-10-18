// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSierraPlotToolsActionGroup.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "warningState.h"

#include "pqSierraPlotToolsActionGroup.h"

#include "pqSierraPlotToolsManager.h"

//=============================================================================
pqSierraPlotToolsActionGroup::pqSierraPlotToolsActionGroup(QObject* p)
  : QActionGroup(p)
{
  pqSierraPlotToolsManager* manager = pqSierraPlotToolsManager::instance();
  if (!manager)
  {
    qFatal("Cannot get SierraPlotTools Tools manager.");
    return;
  }

  /*
  The section of code below, the this->addAction() calls, are what tells Qt
  to add an action for each of the toolbar buttons,

  * * * _AND_ * * *

  TELLS the ParaView plugin architecture to add a button on to the toolbar for
  this toolbar plugin.

  Some doucumentation from the ParaView Wiki,
  From: http://www.paraview.org/Wiki/Plugin_HowTo

    Please refer to Examples/Plugins/SourceToolbar for this section. There we
    are adding a toolbar with two buttons to create a sphere and a cylinder
    source. For adding a toolbar, one needs to implement a subclass for
    QActionGroup which adds the QActions for each of the toolbar button and then
    implements the handler for the callback when the user clicks any of the
    buttons. In the example SourceToobarActions.h|cxx is the QActionGroup subclass
    that adds the two tool buttons.
  */

  this->addAction(manager->actionDataLoadManager());
  this->addAction(manager->actionPlotVars());
  this->addAction(manager->actionSolidMesh());
  this->addAction(manager->actionWireframeSolidMesh());
  this->addAction(manager->actionWireframeAndBackMesh());
  this->addAction(manager->actionToggleBackgroundBW());
  this->addAction(manager->actionPlotDEBUG());

  // Action groups are usually used to establish radio-button like
  // functionality.  We don't really want that, so turn it off.
  this->setExclusive(false);
}
