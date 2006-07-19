/*=========================================================================

   Program: ParaView
   Module:    pqMultiView.h

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

#ifndef _pqMultiView_h
#define _pqMultiView_h

#include "pqComponentsExport.h"
#include <QFrame>
#include <QList>

class pqMultiViewFrame;
class vtkPVXMLElement;
class QSplitter;

/// class to manage locations of multiple view widgets
class PQCOMPONENTS_EXPORT pqMultiView : public QFrame
{
  Q_OBJECT
public:
  typedef QList<int> Index;
  
  pqMultiView(QWidget* parent = NULL);
  virtual ~pqMultiView();

  /// \brief
  ///   Resets the multi-view to its original state.
  /// \param removed Used to return all the removed widgets.
  virtual void reset(QList<QWidget*> &removed);

  /// replace a widget at index with a new widget, returns the old one
  virtual QWidget* replaceView(Index index, QWidget* widget);

  /// split a location in a direction
  /// a dummy widget is inserted and an index for it is returned
  virtual Index splitView(Index index, Qt::Orientation orientation);

  virtual Index splitView(pqMultiView::Index index, 
  Qt::Orientation orientation, float percent);

  pqMultiViewFrame* splitWidget(QWidget* widget, Qt::Orientation o,float percent);

  /// remove a widget inserted by replaceWidget or splitView
  virtual void removeView(QWidget* widget);

  /// get the index of the widget in its parent's layout
  Index indexOf(QWidget*) const;
  
  /// get the widget from an index
  QWidget* widgetOfIndex(Index index);

  void saveState(vtkPVXMLElement *root);
  void loadState(vtkPVXMLElement *root);
signals:
  /// signal for new frame added
  void frameAdded(pqMultiViewFrame*);
  /// signal for frame removed
  void frameRemoved(pqMultiViewFrame*);

public slots:
  void removeWidget(QWidget *widget);
  pqMultiViewFrame* splitWidgetHorizontal(QWidget *widget);
  pqMultiViewFrame* splitWidgetVertical(QWidget *widget);

protected slots:
  void maximizeWidget(QWidget*);
  void restoreWidget(QWidget*);

protected:

  bool eventFilter(QObject*, QEvent* e);
  pqMultiViewFrame* splitWidget(QWidget*, Qt::Orientation);
  void setup(pqMultiViewFrame*);
  void cleanup(pqMultiViewFrame*);

private:
  void saveSplitter(vtkPVXMLElement *element, QSplitter *splitter, int index);
  void restoreSplitter(QWidget *widget, vtkPVXMLElement *element);
  void cleanSplitter(QSplitter *splitter, QList<QWidget*> &removed);
};


#endif //_pqMultiView_h

