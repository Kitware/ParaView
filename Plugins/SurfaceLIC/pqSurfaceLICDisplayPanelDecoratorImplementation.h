/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSurfaceLICDisplayPanelDecoratorImplementation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __pqSurfaceLICDisplayPanelDecoratorImplementation_h 
#define __pqSurfaceLICDisplayPanelDecoratorImplementation_h

#include <QObject>
#include "pqDisplayPanelDecoratorInterface.h"

class pqSurfaceLICDisplayPanelDecoratorImplementation : 
  public QObject, public pqDisplayPanelDecoratorInterface
{
  Q_OBJECT;
  Q_INTERFACES(pqDisplayPanelDecoratorInterface);
public:
  pqSurfaceLICDisplayPanelDecoratorImplementation(QObject* parent=0);
  virtual ~pqSurfaceLICDisplayPanelDecoratorImplementation();

  /// Returns true if this implementation can decorate the given panel type.
  virtual bool canDecorate(pqDisplayPanel* panel) const;

  /// Called to allow the implementation to decorate the panel. This is called
  /// only if canDecorate(panel) returns true.
  virtual void decorate(pqDisplayPanel* panel) const;

private:
  pqSurfaceLICDisplayPanelDecoratorImplementation(
    const pqSurfaceLICDisplayPanelDecoratorImplementation&); // Not implemented.
  void operator=(
    const pqSurfaceLICDisplayPanelDecoratorImplementation&); // Not implemented.
};

#endif
