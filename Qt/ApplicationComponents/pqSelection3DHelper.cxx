/*=========================================================================

   Program: ParaView
   Module:    pqSelection3DHelper.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqSelection3DHelper.h"

//-----------------------------------------------------------------------------
pqSelection3DHelper::pqSelection3DHelper(QObject* _parent/*=null*/)
: pqRubberBandHelper(_parent)
{
  this->ActionPickObject          = 0;
  this->ActionSelect_Block        = 0;
  this->ActionSelectFrustumPoints = 0;
  this->ActionSelect_Frustum      = 0;
  this->ActionSelectSurfacePoints = 0;
  this->ActionSelectionMode       = 0;
  this->ActionSelectPolygonPoints = 0;

  this->PickObject          = false;
  this->Select_Block        = false;
  this->SelectFrustumPoints = false;
  this->Select_Frustum      = false;
  this->SelectSurfacePoints = false;
  this->SelectionMode       = false;
  this->SelectPolygonPoints = false;

  this->ModeGroup = new QActionGroup(this);
  QObject::connect(
    this,
    SIGNAL(selectionModeChanged(int)),
    this,
    SLOT(onSelectionModeChanged(int)));
  QObject::connect(
    this,
    SIGNAL(selectionFinished(int, int, int, int)),
    this, SLOT(endSelection()));
}

//-----------------------------------------------------------------------------
pqSelection3DHelper::~pqSelection3DHelper()
{
}

//-----------------------------------------------------------------------------
void pqSelection3DHelper::toggleSurfaceSelection()
{
  if(this->SelectionMode)
    {
    this->endSelection();
    }
  else
    {
    this->setRubberBandOn(SELECT);
    }
}

//-----------------------------------------------------------------------------
void pqSelection3DHelper::toggleSurfacePointsSelection()
{
  if(this->SelectSurfacePoints)
    {
    this->endSelection();
    }
  else
    {
    this->setRubberBandOn(SELECT_POINTS);
    }
}

//-----------------------------------------------------------------------------
void pqSelection3DHelper::toggleFrustumSelection()
{
  if(this->Select_Frustum)
    {
    this->endSelection();
    }
  else
    {
    this->setRubberBandOn(FRUSTUM);
    }
}

//-----------------------------------------------------------------------------
void pqSelection3DHelper::toggleFrustumPointsSelection()
{
  if(this->SelectFrustumPoints)
    {
    this->endSelection();
    }
  else
    {
    this->setRubberBandOn(FRUSTUM_POINTS);
    }
}

//-----------------------------------------------------------------------------
void pqSelection3DHelper::toggleBlockSelection()
{
  if(this->Select_Block)
    {
    this->endSelection();
    }
  else
    {
    this->setRubberBandOn(BLOCKS);
    }
}

//-----------------------------------------------------------------------------
void pqSelection3DHelper::togglePick()
{
  if(this->PickObject)
    {
    this->endSelection();
    }
  else
    {
    this->setRubberBandOn(PICK);
    }
}

//-----------------------------------------------------------------------------
void pqSelection3DHelper::togglePolygonPointsSelection()
{
  if(this->SelectPolygonPoints)
    {
    this->endSelection();
    }
  else
    {
    this->setRubberBandOn(POLYGON_POINTS);
    }
}

// ----------------------------------------------------------------------------
void pqSelection3DHelper::setActionSelectionMode(QAction *action)
{
  this->ActionSelectionMode = action;
  this->ModeGroup->addAction(this->ActionSelectionMode);
  QObject::connect(
    this,
    SIGNAL(enableSurfaceSelection(bool)),
    this->ActionSelectionMode, SLOT(setEnabled(bool)));
  QObject::connect(
    this->ActionSelectionMode, SIGNAL(triggered()),
    this, SLOT(toggleSurfaceSelection()));
}

// ----------------------------------------------------------------------------
void pqSelection3DHelper::setActionSelectSurfacePoints(QAction *action)
{
  this->ActionSelectSurfacePoints = action;
  this->ModeGroup->addAction(this->ActionSelectSurfacePoints);
  QObject::connect(
    this,
    SIGNAL(enableSurfacePointsSelection(bool)),
    this->ActionSelectSurfacePoints, SLOT(setEnabled(bool)));
  QObject::connect(
    this->ActionSelectSurfacePoints, SIGNAL(triggered()),
    this, SLOT(toggleSurfacePointsSelection()));
}

// ----------------------------------------------------------------------------
void pqSelection3DHelper::setActionSelect_Frustum(QAction *action)
{
  this->ActionSelect_Frustum = action;
  this->ModeGroup->addAction(this->ActionSelect_Frustum);
  QObject::connect(
    this,
    SIGNAL(enableFrustumSelection(bool)),
    this->ActionSelect_Frustum, SLOT(setEnabled(bool)));
  QObject::connect(
    this->ActionSelect_Frustum, SIGNAL(triggered()),
    this, SLOT(toggleFrustumSelection()));
}

// ----------------------------------------------------------------------------
void pqSelection3DHelper::setActionSelectFrustumPoints(QAction *action)
{
  this->ActionSelectFrustumPoints = action;
  this->ModeGroup->addAction(this->ActionSelectFrustumPoints);
  QObject::connect(
    this,
    SIGNAL(enableFrustumPointSelection(bool)),
    this->ActionSelectFrustumPoints, SLOT(setEnabled(bool)));
  QObject::connect(
    this->ActionSelectFrustumPoints, SIGNAL(triggered()),
    this, SLOT(toggleFrustumPointsSelection()));
}

// ----------------------------------------------------------------------------
void pqSelection3DHelper::setActionSelect_Block(QAction *action)
{
  this->ActionSelect_Block = action;
  this->ModeGroup->addAction(this->ActionSelect_Block);
  QObject::connect(
    this,
    SIGNAL(enableBlockSelection(bool)),
    this->ActionSelect_Block, SLOT(setEnabled(bool)));
  QObject::connect(
    this->ActionSelect_Block, SIGNAL(triggered()),
    this,
    SLOT(toggleBlockSelection()));
}

// ----------------------------------------------------------------------------
void pqSelection3DHelper::setActionPickObject(QAction *action)
{
  this->ActionPickObject = action;
  this->ModeGroup->addAction(this->ActionPickObject);
  QObject::connect(
    this,
    SIGNAL(enablePick(bool)),
    this->ActionPickObject, SLOT(setEnabled(bool)));
  QObject::connect(
    this->ActionPickObject, SIGNAL(triggered()),
    this,
    SLOT(togglePick()));
}

// ----------------------------------------------------------------------------
void pqSelection3DHelper::setActionSelectPolygonPoints(QAction *action)
{
  this->ActionSelectPolygonPoints = action;
  this->ModeGroup->addAction(this->ActionSelectPolygonPoints);
  QObject::connect(
    this,
    SIGNAL(enablePolygonPointsSelection(bool)),
    this->ActionSelectPolygonPoints, SLOT(setEnabled(bool)));
  QObject::connect(
    this->ActionSelectPolygonPoints, SIGNAL(triggered()),
    this,
    SLOT(togglePolygonPointsSelection()));
}

//-----------------------------------------------------------------------------
void pqSelection3DHelper::onSelectionModeChanged(int mode)
{
  switch (mode)
    {
    case pqSelection3DHelper::SELECT:
      if(this->ActionSelectionMode)
        {
        this->ActionSelectionMode->setChecked(true);
        this->SelectionMode = true;
        }
      break;

    case pqSelection3DHelper::SELECT_POINTS:
      if(this->ActionSelectSurfacePoints)
        {
        this->ActionSelectSurfacePoints->setChecked(true);
        this->SelectSurfacePoints = true;
        }
      break;

    case pqSelection3DHelper::FRUSTUM:
      if (this->ActionSelect_Frustum)
        {
        this->ActionSelect_Frustum->setChecked(true);
        this->Select_Frustum = true;
        }
      break;

    case pqSelection3DHelper::FRUSTUM_POINTS:
      if(this->ActionSelectFrustumPoints)
        {
        this->ActionSelectFrustumPoints->setChecked(true);
        this->SelectFrustumPoints = true;
        }
      break;

    case pqSelection3DHelper::BLOCKS:
      if(this->ActionSelect_Block)
        {
        this->ActionSelect_Block->setChecked(true);
        this->Select_Block = true;
        }
      break;

    case pqSelection3DHelper::PICK:
      if(this->ActionPickObject)
        {
        this->ActionPickObject->setChecked(true);
        this->PickObject = true;
        }
      break;

    case pqSelection3DHelper::POLYGON_POINTS:
      if(this->ActionSelectPolygonPoints)
        {
        this->ActionSelectPolygonPoints->setChecked(true);
        this->SelectPolygonPoints = true;
        }
      break;

    case pqSelection3DHelper::INTERACT:
      this->endSelection();
      if(this->SelectionMode)
        {
        this->ActionSelectionMode->setChecked(false);
        this->SelectionMode = false;
        }
      if(this->SelectSurfacePoints)
        {
        this->ActionSelectSurfacePoints->setChecked(false);
        this->SelectSurfacePoints = false;
        }
      if (this->Select_Frustum)
        {
        this->ActionSelect_Frustum->setChecked(false);
        this->Select_Frustum = false;
        }
      if (this->SelectFrustumPoints)
        {
        this->ActionSelectFrustumPoints->setChecked(false);
        this->SelectFrustumPoints = false;
        }
      if (this->Select_Block)
        {
        this->ActionSelect_Block->setChecked(false);
        this->Select_Block = false;
        }
      if (this->PickObject)
        {
        this->ActionPickObject->setChecked(false);
        this->PickObject = false;
        }
      if (this->SelectPolygonPoints)
        {
        this->ActionSelectPolygonPoints->setChecked(false);
        this->SelectPolygonPoints = false;
        }
      break;

    default:
      break;
    }
}
