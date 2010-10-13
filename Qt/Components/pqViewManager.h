/*=========================================================================

   Program: ParaView
   Module:    pqViewManager.h

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

=========================================================================*/
#ifndef __pqViewManager_h
#define __pqViewManager_h

#include "pqMultiView.h"

class pqMultiViewFrame;
class pqServer;
class pqView;
class vtkImageData;
class vtkPVXMLElement;
class vtkSMProxyLocator;

/// This class manages all view windows. View windows occupy the central
/// area in the application are all layed out using split windows. This 
/// split window management is take care by the superclass pqMultiView.
/// This class controls creation/deletion of view modules of the frames 
/// created/deleted by superclass. Note that all view modules,
/// includes 3D render modules, plot views etc are organized by this class.
class PQCOMPONENTS_EXPORT pqViewManager : public pqMultiView 
{
  Q_OBJECT
  typedef pqMultiView Superclass;
public:
  pqViewManager(QWidget* parent=NULL);
  virtual ~pqViewManager();

  /// returns the active view module.
  pqView* getActiveView() const;

  /// This option is used for testing. When size.isEmpty() is true,
  /// it resets the maximum bounds on the view windows.
  /// This is useful when running tests, so that we are guranteed that 
  ///  the view size is fixed.
  void setMaxViewWindowSize(const QSize& size);

  /// Given a view module, get the frame in which the view is contained,
  /// if any.
  pqMultiViewFrame* getFrame(pqView* view) const;

  /// Given a frame, returns the view, if any contained in it.
  pqView* getView(pqMultiViewFrame* frame) const;

  /// Prepare the multiview for a screen capture for the given size. Returns the
  /// magnification to be used while performing the capture, if the
  /// requested size is greater than the widget size. One must call
  /// finishedCapture() after the capture has finished to return to normal
  /// operating state. Note that prepareForCapture and finishedCapture are
  /// needed only when not using saveImage() directly.
  /// Nesting of prepareForCapture and finishedCapture calls is currently not
  /// supported.
  int prepareForCapture(const QSize& size);

  /// Must be called revert adjustments done by prepareForCapture() to return to
  /// normal operating state.
  /// Nesting of prepareForCapture and finishedCapture calls is currently not
  /// supported.
  void finishedCapture();

  /// Saves an image for the entire view layout and returns a new vtkImageData. 
  /// This internally calls prepareForCapture and finishedCapture hence not need 
  /// to call them explicitly.
  vtkImageData* captureImage(int width, int height);

  /// \brief
  ///   Resets the multi-view to its original state.
  /// \param removed Used to return all the removed widgets.
  virtual void reset(QList<QWidget*> &removed);
  virtual void reset();

protected slots:
  /// Save the state of the view window manager.
  void saveState(vtkPVXMLElement* root);

  /// Loads the state for the view window manager.
  bool loadState(vtkPVXMLElement* rwRoot, vtkSMProxyLocator* loader);

  /// Called when server disconnects, we reset the view layout.
  void onServerDisconnect();

signals:
  /// Fired when the active view module changes.
  void activeViewChanged(pqView*);

  /// Fired when setMaxViewWindowSize.  Argument set to true when given a
  /// nonempty size, false otherwise.
  void maxViewWindowSizeSet(bool);

private slots:
  /// This will create a view module to fill the frame.
  /// the render window is created on the active server
  /// which must be set by the application.
  void onFrameAdded(pqMultiViewFrame* frame);
  void onFrameRemoved(pqMultiViewFrame* frame);

  /// Called when a frame close request is made.
  /// We add an undo element to the stack to undo/redo the close.
  void onPreFrameRemoved(pqMultiViewFrame*);

  /// When ever a new view module is noticed, the active 
  /// frame is split and the view module is shown in the new 
  /// split frame.
  void onViewAdded(pqView* rm);

  /// When ever a view module is removed, we also close
  /// the frame containing the view module.
  void onViewRemoved(pqView* rm);

  /// Called when a frame becomes active. It will
  /// inactivate all other frames and trigger activeViewChanged().
  void onActivate(QWidget* obj);

  /// Called when user requests conversion of view type.
  void onConvertToTriggered(QAction* action);

  /// Called when the create view button is clicked in an
  /// empty frame.
  void onConvertToButtonClicked();

  /// Called before context menu is shown for the frame.
  /// We update menu enable state depending on view type.
  void onFrameContextMenuRequested(QWidget*);

  /// Slots to manage drag/drop of frames.
  void frameDragStart(pqMultiViewFrame*);
  void frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*);
  void frameDragMove(pqMultiViewFrame*,QDragMoveEvent*);
  void frameDrop(pqMultiViewFrame*,QDropEvent*);

  /// Called when a split frame request is made.
  /// We add an undo element to the stack to undo/redo the split.
  void onSplittingView(const Index&, Qt::Orientation, float, const Index&);

  /// Destroys any frame overlays.
  void destroyFrameOverlays();

public slots:
  /// Called to change the active view. If view==null and then if the view
  /// manager is currently focused on an empty frame, then it does not change
  /// that. Otherwise the frame containing the view is activated.
  void setActiveView(pqView* view);

protected:
  /// Event filter callback.
  bool eventFilter(QObject* caller, QEvent* e);

  /// This method will either assign an empty frame
  /// to the view module or split the active view
  /// (if any, otherwise splits the first view)
  /// to create a new frame and add the view
  /// to it.
  void assignFrame(pqView* view);

  // Create/disconnects appropriate signal/slot connections between
  // the view and the frame.
  void connect(pqMultiViewFrame* frame, pqView* view);
  void disconnect(pqMultiViewFrame* frame, pqView* view);

  /// Hiding superclasses loadState. Don't use this API
  /// since it is not aware of the loader which gives us 
  /// the render modules to put in the window.
  virtual void loadState(vtkPVXMLElement* /*root*/) { }

  /// Updates the context menu.
  void updateConversionActions(pqMultiViewFrame* frame);

  /// Called when a frame close request is made.
  /// We add an undo element to the stack to undo/redo the close.
  void onFrameRemovedInternal(pqMultiViewFrame*);

  /// Shows the overlay widgets showing all the view sizes.
  void showFrameOverlays();

  QAction* getAction(pqMultiViewFrame* frame,QString name);

  /// need access to the loadState()/saveState() methods.
  friend class pqCloseViewUndoElement;
  friend class pqSplitViewUndoElement;

private:
  pqViewManager(pqViewManager&); // Not implemented.
  void operator=(const pqViewManager&); // Not implemented.

  /// Updates the converto menu.
  void buildConvertMenu();


  class pqInternals;
  pqInternals* Internal;
};

#endif

