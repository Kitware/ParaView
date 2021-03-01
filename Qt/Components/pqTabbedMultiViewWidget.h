/*=========================================================================

   Program: ParaView
   Module:  pqTabbedMultiViewWidget.h

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
#include "vtkType.h"  // needed for vtkIdType
#include <QStyle>     // needed for QStyle:StandardPixmap
#include <QTabBar>    // needed for QTabBar::ButtonPosition
#include <QTabWidget> // needed for QTabWidget.

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
  ~pqTabbedMultiViewWidget() override;

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

  /**
  * Return the layout proxy for the current tab.
  */
  vtkSMViewLayoutProxy* layoutProxy() const;

  /**
   * Returns whether frame decorations are shown.
   */
  bool decorationsVisibility() const;

  /**
   * Locate the pqMultiViewWidget associated with the vtkSMViewLayoutProxy held
   * by this pqTabbedMultiViewWidget instance, if any.
   */
  pqMultiViewWidget* findTab(vtkSMViewLayoutProxy*) const;

  //@{
  /**
   * APIs for filtering of tab widgets. This matches the API exposed by
   * pqPipelineBrowserWidget.
   */
  void enableAnnotationFilter(const QString& annotationKey);
  void disableAnnotationFilter();
  void setAnnotationFilterMatching(bool matching);
  //@}

  /**
   * While generally not necessary to call this, if the annotations for the
   * layout proxies are changed after they are created, applications can use
   * this method to refresh the tabs that are visible.
   */
  void updateVisibleTabs();

  /**
   * This is primarily for testing purposes. Returns list of names for visible
   * tabs.
   */
  QList<QString> visibleTabLabels() const;

Q_SIGNALS:
  /**
  * fired when lockViewSize() is called.
  */
  void viewSizeLocked(bool);

public Q_SLOTS:
  virtual int createTab();
  virtual int createTab(pqServer*);
  virtual int createTab(vtkSMViewLayoutProxy*);
  virtual void closeTab(int);

  /**
   * Makes the tab at the given index current.
   */
  void setCurrentTab(int index);

  //@{
  /**
   * When set to false, all decorations including title frames, separators,
   * tab-bars are hidden.
   */
  void setDecorationsVisibility(bool);
  void showDecorations() { this->setDecorationsVisibility(true); }
  void hideDecorations() { this->setDecorationsVisibility(false); }
  //@}

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

  /**
   * Enter (or exit) preview mode.
   *
   * Preview mode is a mode were various widget's decorations
   * are hidden and the widget is locked to the specified size. If the widget's
   * current size is less than the size specified, then the widget is locked to
   * a size with similar aspect ratio as requested. Pass in invalid (or empty)
   * size to exit preview mode.
   *
   * Preview mode is preferred over `toggleWidgetDecoration` and `lockViewSize`
   * and is mutually exclusive with either. Mixing them can have unintended
   * consequences.
   *
   * @returns the size to which the widget was locked. When unlocked, this will
   * be QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX). When entering preview mode this
   * will same as requested `previewSize` or a smaller size preserving aspect
   * ratio as much as possible.
   */
  QSize preview(const QSize& previewSize = QSize());

protected Q_SLOTS:
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
  * verifies that all views loaded from state are indeed assigned to some view
  * layout, or we just assign them to one.
  */
  virtual void onStateLoaded();

  /**
  * called when context menu need to be created on the tab title.
  */
  void contextMenuRequested(const QPoint&);

  void onLayoutNameChanged(pqServerManagerModelItem*);

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

  /**
  * Internal class used as the TabWidget.
  */
  class pqTabWidget : public QTabWidget
  {
    typedef QTabWidget Superclass;

  public:
    pqTabWidget(QWidget* parentWdg = nullptr);
    ~pqTabWidget() override;

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

    /**
     * Enter/exit preview mode
     */
    QSize preview(const QSize&);

    //@{
    /**
     * Get/Set tab bar visibility. Use this instead of directly calling
     * `this->tabBar()->setVisible()` as that avoid interactions with preview
     * mode.
     */
    void setTabBarVisibility(bool);
    bool tabBarVisibility() const { return this->TabBarVisibility; }
    //@}
  private:
    Q_DISABLE_COPY(pqTabWidget)
    bool ReadOnly;
    bool TabBarVisibility;
    friend class pqTabbedMultiViewWidget;
  };

private:
  Q_DISABLE_COPY(pqTabbedMultiViewWidget)

  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
