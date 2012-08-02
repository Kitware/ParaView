/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqMultiSliceView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME pqMultiSliceView - QT GUI that allow multi slice control

#ifndef __pqMultiSliceView_h
#define __pqMultiSliceView_h

#include "pqRenderView.h"

#include <QtCore>
#include <QtGui>

class pqMultiSliceAxisWidget;

class pqMultiSliceView : public pqRenderView
{
  Q_OBJECT
  typedef pqRenderView Superclass;
public:
  static QString pqMultiSliceViewType() { return "MultiSliceView"; }
  static QString pqMultiSliceViewTypeName() { return "Multi Slice 3D View"; }

  /// constructor takes a bunch of init stuff and must have this signature to
  /// satisfy pqView
  pqMultiSliceView( const QString& viewtype,
                    const QString& group,
                    const QString& name,
                    vtkSMViewProxy* viewmodule,
                    pqServer* server,
                    QObject* p);
  virtual ~pqMultiSliceView();

  /// Override to decorate the QVTKWidget
  virtual QWidget* createWidget();

public slots:
  void updateAxisBounds();
  void updateAxisBounds(double bounds[6]);
  void updateSlices();

protected:
  /// Helper method to get the concreate 3D widget
  QVTKWidget* getInternalWidget() { return this->InternalWidget; }

  QVTKWidget* InternalWidget;
  QPointer<pqMultiSliceAxisWidget> AxisX;
  QPointer<pqMultiSliceAxisWidget> AxisY;
  QPointer<pqMultiSliceAxisWidget> AxisZ;

private:
  pqMultiSliceView(const pqMultiSliceView&); // Not implemented.
  void operator=(const pqMultiSliceView&); // Not implemented.
};

#endif
