/*=========================================================================

   Program: ParaView
   Module:  pqInteractivePropertyWidget.h

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
#ifndef pqInteractivePropertyWidget_h
#define pqInteractivePropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"
#include "vtkBoundingBox.h"
#include <QScopedPointer>

class vtkObject;
class vtkSMNewWidgetRepresentationProxy;
class vtkSMPropertyGroup;

/**
* pqInteractivePropertyWidget is an abstract pqPropertyWidget subclass
* designed to serve as the superclass for all pqPropertyWidget types that have
* interactive widget (also called 3D Widgets) associated with them.
*
* pqInteractivePropertyWidget is intended to provide a Qt widget (along with an
* interactive widget in the active view) for controlling properties on the \c
* proxy identified by a vtkSMPropertyGroup passed to the constructor.
* Subclasses are free to determine which interactive widget to create and how
* to setup the UI for it.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqInteractivePropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(pqSMProxy dataSource READ dataSource WRITE setDataSource);

public:
  pqInteractivePropertyWidget(const char* widget_smgroup, const char* widget_smname,
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqInteractivePropertyWidget() override;

  /**
  * Overridden to call this->render() to ensure that the widget is redrawn.
  */
  void reset() override;

  /**
  * Returns the proxy for the interactive widget.
  */
  vtkSMNewWidgetRepresentationProxy* widgetProxy() const;

  /**
  * Overridden to show the widget proxy in the new view.
  */
  void setView(pqView* view) override;

  /**
  * Returns the interactive widget's visibility. Note that the widget may
  * still not be visible in the view if the pqPropertyWidget is not
  * selected. An interactive widget may be visible in the view when
  * this->isWidgetVisible() and this->isSelected() both return true and the view
  * is of the right type.
  */
  bool isWidgetVisible() const;

  /**
  * Returns the data source.
  */
  vtkSMProxy* dataSource() const;

  /**
  * In these methods, we show/hide the widget since the interactive widget is not
  * supposed to be visible except when the panel is "active" or "selected".
  */
  void select() override;
  void deselect() override;

  /**
  * Returns bounds from the dataSource, if possible. May return invalid bounds
  * when no dataSource exists of hasn't been updated to produce valid data.
  */
  vtkBoundingBox dataBounds() const;

  /**
   * Returns the vtkSMPropertyGroup pass to the constructor.
   */
  vtkSMPropertyGroup* propertyGroup() const;

  /**
   * Overriden in order to hide the VTK widget.
   */
  void hideEvent(QHideEvent*) override;

  /**
   * Overriden in order to show the VTK widget.
   */
  void showEvent(QShowEvent*) override;

public slots:
  /**
  * Toggle the interactive widget's visibility. This, along with
  * pqPropertyWidget's selected state controls whether the widget proxy is
  * visible in a view.
  */
  void setWidgetVisible(bool val);

  /**
  * DataSource is used by interactive widgets to determine now to place the
  * widget in the view e.g. the bounds to use for placing the widget or when
  * re-centering the interactive widget.
  */
  void setDataSource(vtkSMProxy* dataSource);

protected slots:
  /**
  * Places the interactive widget using current data source information.
  */
  virtual void placeWidget() = 0;

  /**
  * Safe call render on the view.
  */
  void render();

signals:
  /**
  * Fired whenever setWidgetVisible() changes the widget's visibility.
  */
  void widgetVisibilityToggled(bool);

  /**
  * Fired whenever the widgets visibility is updated for whatever reason, be
  * it because the panel was selected/deselected or the view changed, etc.
  */
  void widgetVisibilityUpdated(bool);

  /**
  * Signals fired when the interactive widget proxy fires the corresponding
  * events.
  */
  void startInteraction();
  void interaction();
  void endInteraction();

  void dummySignal();

private slots:
  /**
  * This method is called to update the state of Visibility and Enabled
  * properties on the widget based on the state of isWidgetVisible() and
  * isSelected().
  */
  void updateWidgetVisibility();

protected:
  bool VisibleState = true;

private:
  void handleUserEvent(vtkObject*, unsigned long, void*);

private:
  Q_DISABLE_COPY(pqInteractivePropertyWidget)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
