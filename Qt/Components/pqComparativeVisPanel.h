/*=========================================================================

   Program: ParaView
   Module:    pqComparativeVisPanel.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqComparativeVisPanel_h
#define pqComparativeVisPanel_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqView;
class vtkSMProxy;
class vtkSMProperty;
class vtkEventQtSlotConnect;

/**
* pqComparativeVisPanel is a properties page for the comparative view. It
* allows the user to change the layout of the grid as well as add/remove
* parameters to compare in the view.
*/
class PQCOMPONENTS_EXPORT pqComparativeVisPanel : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqComparativeVisPanel(QWidget* parent = 0);
  ~pqComparativeVisPanel() override;

  /**
  * Access the current view being shown by this panel.
  */
  pqView* view() const;

public Q_SLOTS:
  /**
  * Set the view to shown in this panel. If the view is not a comparative view
  * then the panel will be disabled, otherwise, it shows the properties of the
  * view.
  */
  void setView(pqView*);

protected Q_SLOTS:
  /**
  * If vtkSMProxy has a TimestepValues property then this method will set the
  * TimeRange property of vtkSMComparativeViewProxy to reflect the values.
  */
  // void setTimeRangeFromSource(vtkSMProxy*);

  /**
  * Called when the "+" button is clicked to add a new parameter.
  */
  void addParameter();

  /**
  * Updates the list of animated parameters from the proxy.
  */
  void updateParametersList();

  /**
  * Called when the selection in the active parameters widget changes.
  */
  void parameterSelectionChanged();

  void sizeUpdated();

  /**
  * Triggered when user clicks the delete button to remove a parameter.
  */
  void removeParameter(int index);

protected:
  // void activateCue(vtkSMProperty* cuesProperty,
  // vtkSMProxy* animatedProxy, const QString& animatedPName, int animatedIndex);

  /**
  * Finds the row (-1 if none found) for the given (proxy,property).
  */
  int findRow(vtkSMProxy* animatedProxy, const QString& animatedPName, int animatedIndex);

private:
  Q_DISABLE_COPY(pqComparativeVisPanel)

  vtkEventQtSlotConnect* VTKConnect;
  class pqInternal;
  pqInternal* Internal;
};

#endif
