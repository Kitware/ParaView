// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSLACActionGroup.h

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

#ifndef __pqSLACActionGroup_h
#define __pqSLACActionGroup_h

#include <QActionGroup>

/// Adds actions that are helpful for setting up visualization of SLAC
/// simulation result files.
class pqSLACActionGroup : public QActionGroup
{
  Q_OBJECT;
public:
  pqSLACActionGroup(QObject *p);

private:
  pqSLACActionGroup(const pqSLACActionGroup &);     // Not implemented
  void operator=(const pqSLACActionGroup &);        // Not implemented
};

#endif //__pqSLACActionGroup_h
