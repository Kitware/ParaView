/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqPointSpriteDisplayPanelDecorator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqPointSpriteDisplayPanelDecorator
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

#ifndef __pqPointSpriteDisplayPanelDecorator_h
#define __pqPointSpriteDisplayPanelDecorator_h

#include <QGroupBox>
class pqDisplayPanel;
class pqPipelineRepresentation;
class pqWidgetRangeDomain;
class vtkSMProperty;

#include "pqVariableType.h"

class pqPointSpriteDisplayPanelDecorator : public QGroupBox
{
  Q_OBJECT
  typedef QGroupBox Superclass;
public:
  pqPointSpriteDisplayPanelDecorator(pqDisplayPanel* panel);
  ~pqPointSpriteDisplayPanelDecorator();

protected slots:
  void representationTypeChanged();

  void updateEnableState();

  // slots called when the radius array settings change
  void  onRadiusArrayChanged(pqVariableType type, const QString& name);
  void  onRadiusComponentChanged(int vectorMode, int comp);

  // slots called when the alpha array settings change
  void  onOpacityArrayChanged(pqVariableType type, const QString& name);
  void  onOpacityComponentChanged(int vectorMode, int comp);

  void  showRadiusDialog();
  void  showOpacityDialog();

  void  reloadGUI();


protected :
  // setup the connections between the GUI and the proxies
  void setupGUIConnections();

  // called when the representation has been modified to update the menus
  void setRepresentation(pqPipelineRepresentation* repr);

  virtual void updateAllViews();

  void  LinkWithRange(QWidget* widget, const char* signal, vtkSMProperty* prop, pqWidgetRangeDomain*& widgetRangeDomain);


private:
  pqPointSpriteDisplayPanelDecorator(const pqPointSpriteDisplayPanelDecorator&); // Not implemented.
  void operator=(const pqPointSpriteDisplayPanelDecorator&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
};

#endif


