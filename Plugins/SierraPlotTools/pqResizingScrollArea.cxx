// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqResizingScrollArea.cxx

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

#include "pqResizingScrollArea.h"

#include <QApplication>
#include <QDesktopWidget>

pqResizingScrollArea::pqResizingScrollArea(QWidget* _parent)
  : QScrollArea(_parent)
{
  setWidgetResizable(true);
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

QSize pqResizingScrollArea::sizeHint() const
{
  // return QScrollArea::sizeHint();

  QWidget* currentWidget = widget();

  if (!currentWidget)
  {
    return QScrollArea::sizeHint();
  }

  QSize newSize;
  newSize.setWidth(QScrollArea::sizeHint().width());

  // retrieve the scrollarea margins
  int leftMargin, topMargin, rightMargin, bottomMargin;
  getContentsMargins(&leftMargin, &topMargin, &rightMargin, &bottomMargin);

  int widgetHintHeight = currentWidget->sizeHint().height() + topMargin + bottomMargin;
  int scrollHintHeight = QScrollArea::sizeHint().height();

  newSize.setHeight(qMax(widgetHintHeight, scrollHintHeight));

  int deskTopHeight = QApplication::desktop()->availableGeometry().height();

  // but don't get too big relative to the deskTopHeight
  newSize.setHeight(qMin(newSize.height(), int(deskTopHeight * 0.4)));

  return newSize;
}
