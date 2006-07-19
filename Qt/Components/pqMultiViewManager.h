/*=========================================================================

   Program: ParaView
   Module:    pqMultiViewManager.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/

#ifndef _pqMultiViewManager_h
#define _pqMultiViewManager_h

class pqMultiViewFrame;
class QSplitter;
class QWidget;
class vtkPVXMLElement;

#include "pqWidgetsExport.h"
#include "pqMultiView.h"

/// multi-view manager
class PQWIDGETS_EXPORT pqMultiViewManager : public pqMultiView
{
  Q_OBJECT
public:
  pqMultiViewManager(QWidget* parent=NULL);
  virtual ~pqMultiViewManager();

  /// \brief
  ///   Resets the multi-view to its original state.
  /// \param removed Used to return all the removed widgets.
  /// \param newWidget Unused. The multi-view manager provides its own widget.
  virtual void reset(QList<QWidget*> &removed, QWidget *newWidget=0);

  void saveState(vtkPVXMLElement *root);
  void loadState(vtkPVXMLElement *root);

signals:
  /// signal for new frame added
  void frameAdded(pqMultiViewFrame*);
  /// signal for frame removed
  void frameRemoved(pqMultiViewFrame*);

public slots:
  void removeWidget(QWidget *widget);
  void splitWidgetHorizontal(QWidget *widget);
  void splitWidgetVertical(QWidget *widget);

protected slots:
  void maximizeWidget(QWidget*);
  void restoreWidget(QWidget*);

protected:

  bool eventFilter(QObject*, QEvent* e);
  void splitWidget(QWidget*, Qt::Orientation);

  void setup(pqMultiViewFrame*);
  void cleanup(pqMultiViewFrame*);

private:
  void saveSplitter(vtkPVXMLElement *element, QSplitter *splitter, int index);
  void restoreSplitter(QWidget *widget, vtkPVXMLElement *element);

};

#endif // _pqMultiViewManager_h

