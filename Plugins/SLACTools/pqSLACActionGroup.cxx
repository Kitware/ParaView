// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-CLAUSE
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "pqSLACActionGroup.h"

#include "pqSLACManager.h"

//=============================================================================
pqSLACActionGroup::pqSLACActionGroup(QObject* p)
  : QActionGroup(p)
{
  pqSLACManager* manager = pqSLACManager::instance();
  if (!manager)
  {
    qFatal("Cannot get SLAC Tools manager.");
    return;
  }

  this->addAction(manager->actionDataLoadManager());
  this->addAction(manager->actionShowEField());
  this->addAction(manager->actionShowBField());
  this->addAction(manager->actionShowParticles());
  this->addAction(manager->actionSolidMesh());
  this->addAction(manager->actionWireframeSolidMesh());
  this->addAction(manager->actionWireframeAndBackMesh());
  this->addAction(manager->actionPlotOverZ());
  this->addAction(manager->actionToggleBackgroundBW());
  this->addAction(manager->actionShowStandardViewpoint());
  this->addAction(manager->actionTemporalResetRange());
  this->addAction(manager->actionCurrentTimeResetRange());

  // Action groups are usually used to establish radio-button like
  // functionality.  We don't really want that, so turn it off.
  this->setExclusive(false);
}
