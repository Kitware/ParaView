/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSurfaceLICDisplayPanelDecorator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __pqSurfaceLICDisplayPanelDecorator_h 
#define __pqSurfaceLICDisplayPanelDecorator_h

#include <QObject>
class pqDisplayProxyEditor;
class pqDisplayPanel;

class pqSurfaceLICDisplayPanelDecorator : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqSurfaceLICDisplayPanelDecorator(pqDisplayPanel* panel);
  ~pqSurfaceLICDisplayPanelDecorator();

protected slots:
  void representationTypeChanged();

private:
  pqSurfaceLICDisplayPanelDecorator(const pqSurfaceLICDisplayPanelDecorator&); // Not implemented.
  void operator=(const pqSurfaceLICDisplayPanelDecorator&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
};

#endif
