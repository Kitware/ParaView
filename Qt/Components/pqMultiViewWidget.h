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
#ifndef pqMultiViewWidget_h
#define pqMultiViewWidget_h

#include "pqComponentsModule.h"
#include "vtkSetGet.h" // for VTK_LEGACY
#include <QWidget>

class pqProxy;
class pqView;
class pqViewFrame;
class vtkImageData;
class vtkObject;
class vtkSMProxy;
class vtkSMViewLayoutProxy;
class vtkSMViewProxy;

/**
* pqMultiViewWidget is a widget that manages layout of multiple views. It
* works together with a vtkSMViewLayoutProxy instance to keep track of the layout
* for the views. It's acceptable to create multiple instances of
* pqMultiViewWidget in the same application.
*
* @sa pqHierarchicalGridLayout, pqHierarchicalGridWidget
*
*/
class PQCOMPONENTS_EXPORT pqMultiViewWidget : public QWidget
{
  Q_OBJECT;
  typedef QWidget Superclass;
  Q_PROPERTY(bool decorationsVisibility READ decorationsVisibility WRITE setDecorationsVisibility
      NOTIFY decorationsVisibilityChanged);

public:
  pqMultiViewWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqMultiViewWidget() override;

  /**
  * Get/Set the vtkSMViewLayoutProxy instance this widget is using as the layout
  * manager.
  */
  void setLayoutManager(vtkSMViewLayoutProxy*);
  vtkSMViewLayoutProxy* layoutManager() const;

  /**
  * Returns whether window decorations and splitter handles are visible.
  */
  bool decorationsVisibility() const;

  /**
  * Returns list of views assigned to frames in this widget.
  */
  QList<vtkSMViewProxy*> viewProxies() const;

  /**
  * Returns true if the view has been assigned to this layout.
  */
  bool isViewAssigned(pqView*) const;

  /**
  * pqMultiViewWidget supports popout mode i.e. the views could be laid out
  * in separate popup widget rather than simply placing them  under this
  * pqMultiViewWidget frame. Use this method to toggle that. Returns true if
  * the view is popped out at the end of this call, false otherwise.
  */
  bool togglePopout();

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

  /**
   * Returns the location of the active frame, if any, else -1.
   */
  int activeFrameLocation() const;

  /**
   * @deprecated use `decorationsVisibility` instead.
   */
  VTK_LEGACY(bool isDecorationsVisible() const);

  /**
   * @deprecated, use `setDecorationsVisibility` instead.
   */
  VTK_LEGACY(void setDecorationsVisible(bool val));

Q_SIGNALS:
  /**
  * fired when a frame in this widget becomes active.
  */
  void frameActivated();

  /**
   * fired when the decorations visibility is changed (by calling
   * setDecorationsVisible).
   */
  void decorationsVisibilityChanged(bool visible);

public Q_SLOTS:
  /**
  * This forces the pqMultiViewWidget to reload its layout from the
  * vtkSMViewLayoutProxy instance. One does not need to call this method
  * explicitly, it is called automatically when the layoutManager is modified.
  */
  void reload();

  /**
  * In a tabbed setup, when pqMultiViewWidget becomes active, this method
  * should be called to ensure that the first view/frame in this widget is
  * indeed made active, as the user would expect.
  */
  void makeFrameActive();

  //@{
  /**
   * Set the visibility for frame decorations and splitter handles.
   */
  void setDecorationsVisibility(bool);
  void showDecorations() { this->setDecorationsVisibility(true); }
  void hideDecorations() { this->setDecorationsVisibility(false); }
  //@}

  /**
  * Locks the maximum size for each view-frame to the given size.
  * Use empty QSize() instance to indicate no limits.
  */
  void lockViewSize(const QSize&);

  /**
  * cleans up the layout.
  */
  void reset();

  /**
  * destroys each of the views present in this layout.
  * Useful when user closes the frame expecting that all containing views are
  * destroyed.
  */
  void destroyAllViews();

protected Q_SLOTS:
  /**
  * Slots called on different signals fired by the nested frames or splitters.
  * Note that these slots use this->sender(), hence these should not be called
  * directly. These result in updating the layoutManager.
  */
  void standardButtonPressed(int);

  /**
  * Makes a frame active. This also call pqActiveObjects::setActiveView() to
  * make the corresponding view active.
  */
  void makeActive(pqViewFrame* frame);

  /**
  * Marks the frame corresponding to the view, if present in the widget, as
  * active. Note that this method does not fire the activeChanged() signal.
  */
  void markActive(pqView* view);
  void markActive(pqViewFrame* frame);

  /**
  * swap frame positions.
  */
  void swapPositions(const QString&);

  /**
  * when a view proxy is unregistered, we ensure that the frame is marked as
  * empty.
  */
  void proxyRemoved(pqProxy*);

  /**
  * called when a new view is added. we update the layout if the view added
  * belongs to this layout.
  */
  void viewAdded(pqView*);

  //@{
  /**
   * If in preview mode, we need to update the widget's constraints based on the
   * current widget geometry. We do that in this overridden method which is
   * called when the geometry may have potentially changed.
   */
  void resizeEvent(QResizeEvent* evt) override;
  //@}

protected:
  /**
  * Called whenever a new frame needs to be created for a view. Note that view
  * may be null, in which case a place-holder frame is expected. The caller
  * takes over the ownership of the created frame and will delete/re-parent it
  * as and when appropriate.
  */
  virtual pqViewFrame* newFrame(vtkSMProxy* view);

  /**
  * Event filter callback to detect when a sub-frame becomes active, so that
  * we can mark it as such.
  */
  bool eventFilter(QObject* caller, QEvent* evt) override;

private:
  void layoutPropertyModified(vtkObject*, unsigned long, void*);

private:
  Q_DISABLE_COPY(pqMultiViewWidget)

  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
