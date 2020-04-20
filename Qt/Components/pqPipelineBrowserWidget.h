/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowserWidget.h

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
#ifndef pqPipelineBrowserWidget_h
#define pqPipelineBrowserWidget_h

#include "pqComponentsModule.h"
#include "pqFlatTreeView.h"

#include <QPointer>

class pqPipelineModel;
class pqPipelineAnnotationFilterModel;
class pqPipelineSource;
class pqOutputPort;
class pqView;
class QMenu;
class vtkSession;

/**
* pqPipelineBrowserWidget is the widget for the pipeline  browser. This is a
* replacement for pqPipelineBrowser.
*/
class PQCOMPONENTS_EXPORT pqPipelineBrowserWidget : public pqFlatTreeView
{
  Q_OBJECT
  typedef pqFlatTreeView Superclass;

public:
  pqPipelineBrowserWidget(QWidget* parent = 0);
  ~pqPipelineBrowserWidget() override;

  /**
  * Used to monitor the key press events in the tree view.
  * Returns True if the event should not be sent to the object.
  */
  bool eventFilter(QObject* object, QEvent* e) override;

  /**
  * Set the visibility of selected items.
  */
  void setSelectionVisibility(bool visible);

  /**
  * Set Annotation filter to use
  */
  void enableAnnotationFilter(const QString& annotationKey);

  /**
  * Disable any Annotation filter
  */
  void disableAnnotationFilter();

  /**
   * Choose wether Annotation filter should display matching or non-matching sources.
   * Default is matching.
   */
  void setAnnotationFilterMatching(bool matching);

  /**
  * Set Session filter to use
  */
  void enableSessionFilter(vtkSession* session);

  /**
  * Disable any Session filter
  */
  void disableSessionFilter();

  /**
  * Overload of pqFlatTreeView::setModel
  */
  void setModel(pqPipelineModel* model);

  /**
  * TODO document
  * @note Moved from proteced
  */
  const QModelIndex pipelineModelIndex(const QModelIndex& index) const;
  const pqPipelineModel* getPipelineModel(const QModelIndex& index) const;

  /**
  * static method to sets the visibility of a pqOutputPort
  */
  static void setVisibility(bool visible, pqOutputPort* port);

  /**
  * Provides access to the context menu.
  */
  QMenu* contextMenu() const;

Q_SIGNALS:
  /**
  * Fired when the delete key is pressed.
  * Typically implies that the selected items need to be deleted.
  */
  void deleteKey();

public Q_SLOTS:
  /**
  * Set the active view. By default connected to
  * pqActiveObjects::viewChanged() so it keeps track of the active view.
  */
  void setActiveView(pqView*);

protected Q_SLOTS:
  void handleIndexClicked(const QModelIndex& index);
  void expandWithModelIndexTranslation(const QModelIndex&);

protected:
  /**
  * sets the visibility for items in the indices list.
  */
  void setVisibility(bool visible, const QModelIndexList& indices);

  void contextMenuEvent(QContextMenuEvent* e) override;

  /**
   * Overridden to pass changed font to pqPipelineModel.
   */
  bool viewportEvent(QEvent* e) override;

  pqPipelineModel* PipelineModel;
  pqPipelineAnnotationFilterModel* FilteredPipelineModel;
  QPointer<QMenu> ContextMenu;

private:
  /**
  * Set up the current pqPipelineModel.
  */
  void configureModel();

  Q_DISABLE_COPY(pqPipelineBrowserWidget)
};

#endif
