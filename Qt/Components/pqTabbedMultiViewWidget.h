/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef __pqTabbedMultiViewWidget_h 
#define __pqTabbedMultiViewWidget_h

#include <QWidget>
#include "pqComponentsExport.h"
#include "vtkType.h" // needed for vtkIdType

class pqProxy;
class pqServer;
class pqView;
class vtkImageData;
class vtkSMViewLayoutProxy;

/// pqTabbedMultiViewWidget is used to to enable adding of multiple
/// pqMultiViewWidget instances in tabs. This class directly listens to the
/// server-manager to automatically create pqMultiViewWidget instances for every
/// vtkSMViewLayoutProxy registered.
class PQCOMPONENTS_EXPORT pqTabbedMultiViewWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqTabbedMultiViewWidget(QWidget* parent=0);
  virtual ~pqTabbedMultiViewWidget();

  /// Returns the size for the tabs in the widget.
  QSize clientSize() const;

  /// Captures an image for the views in the layout. Note that there must be
  /// at least one valid view in the widget, otherwise returns NULL.
  vtkImageData* captureImage(int width, int height);

  /// setups up the environment for capture. Returns the magnification that can
  /// be used to capture the image for required size.
  int prepareForCapture(int width, int height);

  /// cleans up the environment after image capture.
  void cleanupAfterCapture();

signals:
  /// fired when lockViewSize() is called.
  void viewSizeLocked(bool);

public slots:
  void createTab();
  void createTab(pqServer*);
  void createTab(vtkSMViewLayoutProxy*);
  void closeTab(int);

  /// toggles fullscreen state.
  void toggleFullScreen();

  /// Locks the maximum size for each view-frame to the given size.
  /// Use empty QSize() instance to indicate no limits.
  void lockViewSize(const QSize&);

  /// cleans up the layout.
  void reset();

protected slots:
  /// slots connects to corresponding signals on pqServerManagerObserver.
  void proxyAdded(pqProxy*);
  void proxyRemoved(pqProxy*);
  void serverRemoved(pqServer*);

  /// called when the active tab changes. If the active tab is the "+" tab, then
  /// add a new tab to the widget.
  void currentTabChanged(int);

  /// called when a frame in pqMultiViewWidget is activated. Ensures that that
  /// widget is visible.
  void frameActivated();

  /// verifies that all views loaded from state are indeed assigned to some view
  /// layout, or we just assign them to one.
  void onStateLoaded();

  /// called when pqObjectBuilder is about to create a new view. We ensure that
  /// a layout exists to accept that view. This is essential for collaborative
  /// mode to work correctly without ending up multiple layouts on the two
  /// processes.
  void aboutToCreateView(pqServer*);

protected:
  bool eventFilter(QObject *obj, QEvent *event);

  /// assigns a frame to the view.
  void assignToFrame(pqView*, bool warnIfTabCreated);

private:
  Q_DISABLE_COPY(pqTabbedMultiViewWidget);

  class pqInternals;
  pqInternals* Internals;
};

#endif
