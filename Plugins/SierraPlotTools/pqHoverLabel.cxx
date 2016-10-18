// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqHoverLabel.cxx

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

#include "pqHoverLabel.h"
#include "pqPlotter.h"

#include <QLabel>
#include <QMouseEvent>
#include <QToolTip>

///////////////////////////////////////////////////////////////////////////////
pqHoverLabel::pqHoverLabel(QWidget* _parent)
  : QLabel(_parent)
{
  this->setMouseTracking(true);
}

// We overload the MouseMoveEvent
//
//
void pqHoverLabel::mouseMoveEvent(QMouseEvent* theEvent)
{
  QLabel::mouseMoveEvent(theEvent);

#if 0
  int globalX = theEvent->globalX();
  int globalY = theEvent->globalY();
  const QPoint pos = theEvent->pos();
  QPointF posF = theEvent->posF();
  int x = theEvent->x();
  int y = theEvent->y();
#endif

  QString currentToolTip;
  if (this->plotter != NULL)
  {
    currentToolTip = plotter->getPlotterHeadingHoverText();
  }
  else
  {
    currentToolTip =
      QString("pqHoverLabel::mouseMoveEvent: current tool tip REALLY SHOULD NOT BE HERE");
    ;
  }

  QToolTip::showText(theEvent->globalPos(), currentToolTip);
}

void pqHoverLabel::setPlotter(pqPlotter* thePlotter)
{
  plotter = thePlotter;
}

bool pqHoverLabel::event(QEvent* theEvent)
{
  QEvent::Type type = theEvent->type();
  if (type == QEvent::ToolTip)
  {
    // possibly do something...
  }

  return this->QLabel::event(theEvent);
}
