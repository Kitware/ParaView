/*=========================================================================

   Program: ParaView
   Module:    pqComparativeCueWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef pqComparativeCueWidget_h
#define pqComparativeCueWidget_h

#include "pqComponentsModule.h"
#include "pqTimer.h"
#include "vtkSmartPointer.h"
#include <QTableWidget>

class vtkEventQtSlotConnect;
class vtkSMComparativeAnimationCueProxy;
class vtkSMProxy;

/**
* pqComparativeCueWidget is designed to be used by
* pqComparativeVisPanel to show/edit the values for an
* vtkSMComparativeAnimationCueProxy.
*/
class PQCOMPONENTS_EXPORT pqComparativeCueWidget : public QTableWidget
{
  Q_OBJECT
  typedef QTableWidget Superclass;

public:
  pqComparativeCueWidget(QWidget* parent = 0);
  ~pqComparativeCueWidget() override;

  /**
  * Get/Set the cue that is currently being shown/edited by this widget.
  */
  void setCue(vtkSMProxy*);
  vtkSMComparativeAnimationCueProxy* cue() const;

  QSize size() const { return this->Size; }

  // Returns if this cue can accept more than 1 value as a parameter value.
  bool acceptsMultipleValues() const;

public Q_SLOTS:
  /**
  * Set the comparative grid size.
  */
  void setSize(int w, int h)
  {
    this->Size = QSize(w, h);
    this->updateGUIOnIdle();
  }

Q_SIGNALS:
  // triggered every time the user changes the values.
  void valuesChanged();

protected Q_SLOTS:
  /**
  * refreshes the GUI with values from the proxy.
  */
  void updateGUI();

  void updateGUIOnIdle() { this->IdleUpdateTimer.start(); }

  void onSelectionChanged() { this->SelectionChanged = true; }

  void onCellChanged(int x, int y);

protected:
  /**
  * called when mouse is released. We use this to popup the range editing
  * dialog if the selection changed.
  */
  void mouseReleaseEvent(QMouseEvent* evt) override;

  void editRange();

private:
  Q_DISABLE_COPY(pqComparativeCueWidget)

  vtkEventQtSlotConnect* VTKConnect;
  bool InUpdateGUI;
  bool SelectionChanged;
  pqTimer IdleUpdateTimer;
  QSize Size;
  vtkSmartPointer<vtkSMComparativeAnimationCueProxy> Cue;
};

#endif
