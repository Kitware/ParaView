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

#include "pqCoreModule.h"
#include "pqRenderView.h"

#include <QtCore>
#include <QtGui>

class pqMultiSliceAxisWidget;

class PQCORE_EXPORT pqMultiSliceView : public pqRenderView
{
  Q_OBJECT
  typedef pqRenderView Superclass;
public:
  static QString multiSliceViewType() { return "MultiSlice"; }

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

  /// Provide access to slices positions for any axis.
  /// 0 <= axisIndex <= 2
  const double* GetSlices(int axisIndex, int &numberOfSlices);

  /// Provide access to slices normal for any axis.
  /// 0 <= axisIndex <= 2
  const double* GetSliceNormal(int axisIndex);

  /// Provide access to slices origin for any axis.
  /// 0 <= axisIndex <= 2
  const double* GetSliceOrigin(int axisIndex);

  /// Override for custom management
  virtual void setCursor(const QCursor &);

  /// Update Outline visibility
  bool getOutlineVisibility();
  void setOutlineVisibility(bool visible);

signals:
  void slicesChanged();
  void sliceClicked(int axisIndex, double sliceOffsetOnAxis, int button, int modifier);

public slots:
  void updateAxisBounds();
  void updateAxisBounds(double bounds[6]);
  void updateSlices();

protected:
  void updateViewModelCallBack(vtkObject*,unsigned long, void*);

  /// Helper method to get the concreate 3D widget
  QVTKWidget* getInternalWidget();

  QPointer<QVTKWidget> InternalWidget;
  bool UserIsInteracting;
  QPointer<pqMultiSliceAxisWidget> AxisX;
  QPointer<pqMultiSliceAxisWidget> AxisY;
  QPointer<pqMultiSliceAxisWidget> AxisZ;

  QMap<pqRepresentation*, unsigned int> ObserverIdX;
  QMap<pqRepresentation*, unsigned int> ObserverIdY;
  QMap<pqRepresentation*, unsigned int> ObserverIdZ;

  double NormalValuesHolder[9];
  double OriginValuesHolder[9];

protected slots:
  // Internal slot that will emit sliceClicked()
  void onSliceClicked(int button, int modifier, double value);

private:
  pqMultiSliceView(const pqMultiSliceView&); // Not implemented.
  void operator=(const pqMultiSliceView&); // Not implemented.
};

#endif
