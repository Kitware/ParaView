/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqViewUpdater.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME pqViewUpdater - Updates views automatically
//
// .SECTION Description
// pqViewUpdater listens for change events from an annotation link,
// updating views that depend on that link.
//
// .SECTION See Also
// vtkViewUpdater

#ifndef __pqViewUpdater_h
#define __pqViewUpdater_h

#include "OverViewCoreExport.h"
#include "vtkObject.h"

class pqProxy;
class pqView;

class OVERVIEW_CORE_EXPORT pqViewUpdater
{
public:
  pqViewUpdater();
  ~pqViewUpdater();

  void SetView(pqView* const view);
  void AddLink(pqProxy* annnotation_link);

//BTX
private:
  pqViewUpdater(const pqViewUpdater&);  // Not implemented.
  void operator=(const pqViewUpdater&);  // Not implemented.

  class Implementation;
  Implementation* Internal;
//ETX
};

#endif
