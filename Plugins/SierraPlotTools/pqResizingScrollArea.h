// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqResizingScrollArea.h

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

#ifndef pqResizingScrollArea_h
#define pqResizingScrollArea_h

#include <QScrollArea>

//
// This class provides a scroll area that can resize (to a point) as widgets are
// added to the contents
//

class pqResizingScrollArea : public QScrollArea
{
  Q_OBJECT;

public:
  pqResizingScrollArea(QWidget* parent = 0);

  QSize sizeHint() const;
};

#endif // pqResizingScrollArea_h
