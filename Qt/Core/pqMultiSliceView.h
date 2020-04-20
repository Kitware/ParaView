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

#ifndef pqMultiSliceView_h
#define pqMultiSliceView_h

#include "pqCoreModule.h"
#include "pqRenderView.h"

#include <QtCore>
#include <QtGui>

class pqMultiSliceAxisWidget;
class pqQVTKWidget;

class PQCORE_EXPORT pqMultiSliceView : public pqRenderView
{
  Q_OBJECT
  typedef pqRenderView Superclass;

public:
  static QString multiSliceViewType() { return "MultiSlice"; }

  /**
  * constructor takes a bunch of init stuff and must have this signature to
  * satisfy pqView
  */
  pqMultiSliceView(const QString& viewtype, const QString& group, const QString& name,
    vtkSMViewProxy* viewmodule, pqServer* server, QObject* p);
  ~pqMultiSliceView() override;

  /**
  * Provide access to visible slices positions for any axis.
  * Precondition: 0 <= axisIndex <= 2
  */
  const double* GetVisibleSlices(int axisIndex, int& numberOfSlices);

  /**
  * @deprecated. Use GetVisibleSlices() or GetAllSlices(). This method simply
  * calls GetVisibleSlices().
  */
  const double* GetSlices(int axisIndex, int& numberOfSlices)
  {
    return this->GetVisibleSlices(axisIndex, numberOfSlices);
  }

  /**
  * Provides access to all (visible and invisible) slice positions for any
  * Precondition: 0 <= axisIndex <= 2
  */
  const double* GetAllSlices(int axisIndex, int& numberOfSlices);

  /**
  * Provide access to slices normal for any axis.
  * 0 <= axisIndex <= 2
  */
  const double* GetSliceNormal(int axisIndex);

  /**
  * Provide access to slices origin for any axis.
  * 0 <= axisIndex <= 2
  */
  const double* GetSliceOrigin(int axisIndex);

  /**
  * Override for custom management
  */
  void setCursor(const QCursor&) override;

  /**
  * Update Outline visibility
  */
  bool getOutlineVisibility();
  void setOutlineVisibility(bool visible);

Q_SIGNALS:
  // Fired when the slices are changed by user interaction.
  // Provides information about which slice is being
  // changed. axisIndex is the index of axis [0,2], while sliceIndex is the
  // index for the slice in the slices returned by GetAllSlices(). If a slice is
  // deleted, the sliceIndex will point to its index before the slice was
  // deleted.
  void sliceAdded(int axisIndex, int sliceIndex);
  void sliceRemoved(int axisIndex, int sliceIndex);
  void sliceModified(int axisIndex, int sliceIndex);

  void sliceClicked(int axisIndex, double sliceOffsetOnAxis, int button, int modifier);

public Q_SLOTS:
  void updateSlices();

private Q_SLOTS:
  void updateAxisBounds();
  void onSliceAdded(int activeSliceIndex);
  void onSliceRemoved(int activeSliceIndex);
  void onSliceModified(int activeSliceIndex);

protected:
  void updateViewModelCallBack(vtkObject*, unsigned long, void*);

  /**
  * Override to decorate the pqQVTKWidgetBase
  */
  QWidget* createWidget() override;

  /**
  * Helper method to get the concreate 3D widget
  */
  pqQVTKWidget* getInternalWidget();

  /**
  * Get axis index.
  */
  int getAxisIndex(QObject*);

  QPointer<pqQVTKWidget> InternalWidget;
  bool UserIsInteracting;
  QPointer<pqMultiSliceAxisWidget> AxisX;
  QPointer<pqMultiSliceAxisWidget> AxisY;
  QPointer<pqMultiSliceAxisWidget> AxisZ;
  QPointer<pqMultiSliceAxisWidget> AxisXYZ[3];

  QMap<pqRepresentation*, unsigned int> ObserverIdX;
  QMap<pqRepresentation*, unsigned int> ObserverIdY;
  QMap<pqRepresentation*, unsigned int> ObserverIdZ;

  double NormalValuesHolder[9];
  double OriginValuesHolder[9];

protected Q_SLOTS:
  // Internal slot that will emit sliceClicked()
  void onSliceClicked(int button, int modifier, double value);

private:
  Q_DISABLE_COPY(pqMultiSliceView)
};

#endif
