/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqPointSpriteDisplayPanelDecoratorImplementation.h

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

#ifndef __pqPointSpriteDisplayPanelDecoratorImplementation_h
#define __pqPointSpriteDisplayPanelDecoratorImplementation_h

#include <QObject>
#include "pqDisplayPanelDecoratorInterface.h"

class pqPointSpriteDisplayPanelDecoratorImplementation :
  public QObject, public pqDisplayPanelDecoratorInterface
{
  Q_OBJECT
  Q_INTERFACES(pqDisplayPanelDecoratorInterface);
public:
  pqPointSpriteDisplayPanelDecoratorImplementation(QObject* parent=0);
  virtual ~pqPointSpriteDisplayPanelDecoratorImplementation();

  /// Returns true if this implementation can decorate the given panel type.
  virtual bool canDecorate(pqDisplayPanel* panel) const;

  /// Called to allow the implementation to decorate the panel. This is called
  /// only if canDecorate(panel) returns true.
  virtual void decorate(pqDisplayPanel* panel) const;

private:
  pqPointSpriteDisplayPanelDecoratorImplementation(
    const pqPointSpriteDisplayPanelDecoratorImplementation&); // Not implemented.
  void operator=(
    const pqPointSpriteDisplayPanelDecoratorImplementation&); // Not implemented.
};

#endif


