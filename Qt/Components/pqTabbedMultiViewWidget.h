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
#ifndef pqTabbedMultiViewWidget_h
#define pqTabbedMultiViewWidget_h

#include "pqComponentsModule.h"
#include "vtkSetGet.h" // needed for VTK_LEGACY
#include "vtkType.h"   // needed for vtkIdType
#include <QStyle>      // needed for QStyle:StandardPixmap
#include <QTabBar>     // needed for QTabBar::ButtonPosition
#include <QTabWidget>  // needed for QTabWidget.

class pqMultiViewWidget;
class pqProxy;
class pqServer;
class pqServerManagerModelItem;
class pqView;
class vtkImageData;
class vtkSMViewLayoutProxy;

/**
* pqTabbedMultiViewWidget is used to to enable adding of multiple
* pqMultiViewWidget instances in tabs. This class directly listens to the
* server-manager to automatically create pqMultiViewWidget instances for every
* vtkSMViewLayoutProxy registered.
*/
class PQCOMPONENTS_EXPORT pqTabbedMultiViewWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
  Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly)
public:
  pqTabbedMultiViewWidget(QWidget* parent = 0);
  virtual ~pqTabbedMultiViewWidget();

  /**
  * Returns the size for the tabs in the widget.
  */
  virtual QSize clientSize() const;

  /**
  * When set to true (off by default), the widget will not allow
  * adding/removing tabs trough user interactions.
  */
  void setReadOnly(bool val);
  bool readOnly() const;

  /**
  * Set the tab visibility. To save some screen space when only one tab is
  * needed, this can be set to false. True by default.
  */
  void setTabVisibility(bool visible);
  bool tabVisibility() const;

  //@{
  /**
   * @deprecated in ParaView 5.4. `vtkSMSaveScreenshotProxy` now encapsulates
   * all logic to capture images. See `pqSaveScreenshotReaction` for details on
   * using it.
   */
  VTK_LEGACY(vtkImageData* captureImage(int width, int height));
  VTK_LEGACY(int prepareForCapture(int width, int height));
  VTK_LEGACY(void cleanupAfterCapture());
  VTK_LEGACY(bool writeImage(const QString& filename, int width, int height, int quality = -1));
  //@}

signals:
  /**
  * fired when lockViewSize() is called.
  */
  void viewSizeLocked(bool);

public slots:
  virtual void createTab();
  virtual void createTab(pqServer*);
  virtual void createTab(vtkSMViewLayoutProxy*);
  virtual void closeTab(int);

  /**
  * toggles fullscreen state.
  */
  virtual void toggleFullScreen();

  /**
  * toggles decoration visibility on the current widget
  */
  virtual void toggleWidgetDecoration();

  /**
  * Locks the maximum size for each view-frame to the given size.
  * Use empty QSize() instance to indicate no limits.
  */
  virtual void lockViewSize(const QSize&);

  /**
  * cleans up the layout.
  */
  virtual void reset();

protected slots:
  /**
  * slots connects to corresponding signals on pqServerManagerObserver.
  */
  virtual void proxyAdded(pqProxy*);
  virtual void proxyRemoved(pqProxy*);
  virtual void serverRemoved(pqServer*);

  /**
  * called when the active tab changes. If the active tab is the "+" tab, then
  * add a new tab to the widget.
  */
  virtual void currentTabChanged(int);

  /**
  * called when a frame in pqMultiViewWidget is activated. Ensures that that
  * widget is visible.
  */
  virtual void frameActivated();

  /**
  * verifies that all views loaded from state are indeed assigned to some view
  * layout, or we just assign them to one.
  */
  virtual void onStateLoaded();

  /**
  * called when pqObjectBuilder is about to create a new view. We ensure that
  * a layout exists to accept that view. This is essential for collaborative
  * mode to work correctly without ending up multiple layouts on the two
  * processes.
  */
  virtual void aboutToCreateView(pqServer*);

  /**
  * called when context menu need to be created on the tab title.
  */
  void contextMenuRequested(const QPoint&);

  void onLayoutNameChanged(pqServerManagerModelItem*);

protected:
  virtual bool eventFilter(QObject* obj, QEvent* event);

  /**
  * assigns a frame to the view.
  */
  virtual void assignToFrame(pqView*, bool warnIfTabCreated);

  /**
  * Internal class used as the TabWidget.
  */
  class pqTabWidget : public QTabWidget
  {
    typedef QTabWidget Superclass;

  public:
    pqTabWidget(QWidget* parentWdg = NULL);
    virtual ~pqTabWidget();

    /**
    * Set a button to use on the tab bar.
    */
    virtual void setTabButton(int index, QTabBar::ButtonPosition position, QWidget* wdg);

    /**
    * Given the QWidget pointer that points to the buttons (popout or close)
    * in the tabbar, this returns the index of that that the button corresponds
    * to.
    */
    virtual int tabButtonIndex(QWidget* wdg, QTabBar::ButtonPosition position) const;

    /**
    * Add a pqTabbedMultiViewWidget instance as a new tab. This will setup the
    * appropriate tab-bar for this new tab (with 2 buttons for popout and
    * close).
    */
    virtual int addAsTab(pqMultiViewWidget* wdg, pqTabbedMultiViewWidget* self);

    /**
    * Returns the label/tooltip to use for the popout button given the
    * popped_out state.
    */
    static const char* popoutLabelText(bool popped_out);

    /**
    * Returns the icon to use for the popout button given the popped_out state.
    */
    static QStyle::StandardPixmap popoutLabelPixmap(bool popped_out);

    /**
    * When true the widget disable changes to the tabs i.e adding/removing tabs
    * by user interaction.
    */
    void setReadOnly(bool val);
    bool readOnly() const { return this->ReadOnly; }

  private:
    Q_DISABLE_COPY(pqTabWidget)
    bool ReadOnly;
    friend class pqTabbedMultiViewWidget;
  };

private:
  Q_DISABLE_COPY(pqTabbedMultiViewWidget)

  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
