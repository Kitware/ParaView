// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqInteractivePropertyWidgetAbstract_h
#define pqInteractivePropertyWidgetAbstract_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"
#include "vtkBoundingBox.h"
#include <QScopedPointer>

class pqServer;
class vtkObject;
class vtkSMNewWidgetRepresentationProxy;
class vtkSMPropertyGroup;
class vtkSMNewWidgetRepresentationProxyAbstract;

/**
 * pqInteractivePropertyWidgetAbstract is an abstract pqPropertyWidget subclass
 * designed to serve as the superclass for all pqPropertyWidget types that have
 * interactive widget, either in a chart or in a 3D view.
 *
 * pqInteractivePropertyWidgetAbstract is intended to provide a Qt widget (along with an
 * interactive widget in the active view) for controlling properties on the \c
 * proxy identified by a vtkSMPropertyGroup passed to the constructor.
 * Subclasses are free to determine which interactive widget to create and how
 * to setup the UI for it.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqInteractivePropertyWidgetAbstract : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(pqSMProxy dataSource READ dataSource WRITE setDataSource);

public:
  pqInteractivePropertyWidgetAbstract(const char* widget_smgroup, const char* widget_smname,
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqInteractivePropertyWidgetAbstract() override;

  /**
   * Overridden to call this->render() to ensure that the widget is redrawn.
   */
  void reset() override;

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

  ///@{
  /**
   * In these methods, we show/hide the widget since the interactive widget is not
   * supposed to be visible except when the panel is "active" or "selected".
   * selectPort(int) allows to make the selection only if the given port index
   * is linked with the vtkSMPropertyGroup this interactive widget represents.
   */
  void select() override;
  void deselect() override;
  void selectPort(int portIndex) final;
  ///@}

  /**
   * Returns bounds from the dataSource, if possible. May return invalid bounds
   * when no dataSource exists of hasn't been updated to produce valid data.
   * If the input source is a vtkMultiBlockDataSet and "visibleOnly" is set to "true",
   * this function returns the bounds of the visible blocks only.
   */
  vtkBoundingBox dataBounds(bool visibleOnly = false) const;

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

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Toggle the interactive widget's visibility. This, along with
   * pqPropertyWidget's selected state controls whether the widget proxy is
   * visible in a view.
   */
  virtual void setWidgetVisible(bool val);

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

  /**
   * This method is called to update the state of Visibility and Enabled
   * properties on the widget based on the state of isWidgetVisible(),
   * isSelected() and the active view.
   */
  virtual void updateWidgetVisibility();

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

  ///@{
  /**
   * Fired by the underlying interactive widget representation proxy, for each
   * respective events.
   */
  void startInteraction();
  void interaction();
  void endInteraction();
  ///@}

protected:
  /**
   * Get the internal instance of the widget proxy. Subclasses should implement this
   * function and handle the instance of their own widget, wich will be exposed internally
   * via this function.
   */
  virtual vtkSMNewWidgetRepresentationProxyAbstract* internalWidgetProxy() = 0;

  /**
   * Setup all the links and events for the given widget and SM property group.
   * This should be call by subclasses in their constructor once their widget is initialized.
   */
  void setupConnections(vtkSMNewWidgetRepresentationProxyAbstract* widget,
    vtkSMPropertyGroup* smgroup, vtkSMProxy* smproxy);

  void setupUserObserver(vtkSMProxy* smproxy);

  bool VisibleState = true;
  bool WidgetVisibility = false;
  int LinkedPortIndex = -1;

private:
  Q_DISABLE_COPY(pqInteractivePropertyWidgetAbstract)

  void handleUserEvent(vtkObject*, unsigned long, void*);

  struct pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
