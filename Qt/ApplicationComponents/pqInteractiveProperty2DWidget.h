/*=========================================================================

  Program:   ParaView
  Module:    pqInteractiveProperty2DWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef pqInteractiveProperty2DWidget_h
#define pqInteractiveProperty2DWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqInteractivePropertyWidgetAbstract.h"
#include "pqSMProxy.h"
#include "vtkBoundingBox.h"
#include "vtkSMNew2DWidgetRepresentationProxy.h"

#include <QScopedPointer>

class vtkObject;
class vtkSMPropertyGroup;

/**
 * pqInteractiveProperty2DWidget is an abstract pqInteractivePropertyWidgetAbstract subclass
 * designed to serve as the superclass for all pqInteractivePropertyWidgetAbstract types that have
 * interactive widget (also called 2D Widgets) associated with them.
 *
 * pqInteractiveProperty2DWidget is intended to provide a Qt widget (along with an
 * interactive widget in the active view) for controlling properties on the \c
 * proxy identified by a vtkSMPropertyGroup passed to the constructor.
 * Subclasses are free to determine which interactive widget to create and how
 * to setup the UI for it.
 *
 * pqInteractiveProperty2DWidget same as pqInteractivePropertyWidget but use
 * vtkSMNew2DWidgetRepresentationProxy instead vtkSMNewWidgetRepresentationProxy
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqInteractiveProperty2DWidget
  : public pqInteractivePropertyWidgetAbstract
{
  Q_OBJECT
  typedef pqInteractivePropertyWidgetAbstract Superclass;
  Q_PROPERTY(pqSMProxy dataSource READ dataSource WRITE setDataSource);

public:
  pqInteractiveProperty2DWidget(const char* widget_smgroup, const char* widget_smname,
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqInteractiveProperty2DWidget() override;

  /**
   * Returns the proxy for the interactive widget.
   */
  vtkSMNew2DWidgetRepresentationProxy* widgetProxy() const { return this->WidgetProxy; };

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

public Q_SLOTS:
  /**
   * Toggle the interactive widget's visibility. This, along with
   * pqPropertyWidget's selected state controls whether the widget proxy is
   * visible in a view.
   */
  void setWidgetVisible(bool val) override;

  /**
   * DataSource is used by interactive widgets to determine now to place the
   * widget in the view e.g. the bounds to use for placing the widget or when
   * re-centering the interactive widget.
   */
  void setDataSource(vtkSMProxy* dataSource);

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  virtual void placeWidget() = 0;

  /**
   * Safe call render on the view.
   */
  void render();

Q_SIGNALS:
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

private Q_SLOTS:
  /**
   * This method is called to update the state of Visibility and Enabled
   * properties on the widget based on the state of isWidgetVisible() and
   * isSelected().
   */
  void updateWidgetVisibility() override;

  /**
   * Get the internal instance of the widget proxy.
   */
  vtkSMNewWidgetRepresentationProxyAbstract* _widgetProxy() final { return this->WidgetProxy; };

private:
  Q_DISABLE_COPY(pqInteractiveProperty2DWidget)

  vtkSmartPointer<vtkSMNew2DWidgetRepresentationProxy> WidgetProxy;
};

#endif
