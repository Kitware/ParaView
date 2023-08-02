// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPropertyWidget_h
#define pqPropertyWidget_h

#include "pqComponentsModule.h"

#include "pqPropertyLinks.h"
#include <QFrame>
#include <QPointer>
#include <QScopedPointer>

class pqPropertyWidgetDecorator;
class pqTimer;
class pqView;
class vtkPVXMLElement;
class vtkSMDomain;
class vtkSMProperty;
class vtkSMProxy;
/**
 * pqPropertyWidget represents a widget created for each property of a proxy on
 * the pqPropertiesPanel (for the proxy's properties or display properties).
 */
class PQCOMPONENTS_EXPORT pqPropertyWidget : public QFrame
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqPropertyWidget(vtkSMProxy* proxy, QWidget* parent = nullptr);
  ~pqPropertyWidget() override;

  virtual void apply();
  virtual void reset();

  ///@{
  /**
   * These methods are called by pqPropertiesPanel when the panel for proxy
   * becomes active/deactive.
   * Only widgets that have interactive widgets need to
   * override these methods to select/deselect the interactive widgets.
   * `selectPort(int)` allows to specify an output port index and conditionnaly
   * select the interactive widget if the XML hint `WidgetVisibilityLink`
   * has been set.
   * Default implementation does nothing.
   */
  virtual void select() { this->Selected = true; }
  virtual void selectPort(int portIndex) { Q_UNUSED(portIndex); }
  virtual void deselect() { this->Selected = false; }
  bool isSelected() const { return this->Selected; }
  ///@}

  // This method is called on pqPropertyWidget instances that pqProxyWidget
  // deems that should be shown in current configuration. Subclasses can
  // override this method to change the appearance of the widget based on
  // whether advanced properties are currently being shown by the pqProxyWidget
  // or not.
  virtual void updateWidget(bool showing_advanced_properties)
  {
    Q_UNUSED(showing_advanced_properties);
  }

  pqView* view() const;
  vtkSMProxy* proxy() const;
  vtkSMProperty* property() const;
  using Superclass::property; // Don't hide superclass method

  /**
   * Forward calls to vtkSMProperty. Are overwritten by pqPropertyGroupWidget
   * to forward calls to vtkSMPropertyGroup
   */
  virtual char* panelVisibility() const;
  virtual void setPanelVisibility(const char* vis);

  /**
   * Determines if the PropertyWidget must be constructed using a single row.
   *
   * Originally intended for PropertyWidgets which are a group of other
   * Propertywidgets, such as pqCheckableProperty. This mandates that when the
   * widget is rendered, its label to be placed in the same row as the
   * widget group.
   *
   * @see pqProxyWidgetItem::newGroupItem
   */
  virtual bool isSingleRowItem() const;

  bool showLabel() const;

  /**
   * Description:
   * This static utility method returns the XML name for an object as
   * a QString. This allows for code to get the XML name of an object
   * without having to explicitly check for a possibly nullptr char* pointer.
   *
   * This is templated so that it will work with a variety of objects such
   * as vtkSMProperty's and vtkSMDomain's. It can be called with anything
   * that has a "char* GetXMLName()" method.
   *
   * For example, to get the XML name of a vtkSMIntRangeDomain:
   * QString name = pqPropertyWidget::getXMLName(domain);
   */
  template <class T>
  static QString getXMLName(T* object)
  {
    return QString(object->GetXMLName());
  }

  /**
   * Provides access to the decorators for this widget.
   */
  const QList<QPointer<pqPropertyWidgetDecorator>>& decorators() const { return this->Decorators; }

  /**
   * unhide superclass method. Note this is not virtual in QObject so don't add
   * any other logic here.
   */
  using Superclass::setProperty;

  /**
   * Returns the tooltip to use for the property. May return an empty string.
   */
  static QString getTooltip(vtkSMProperty* property);

  /**
   * Helper method to return value from WidgetHeight XML hint, if any.
   * `<WidgetHeight number_of_rows="val">`,
   */
  static int hintsWidgetHeightNumberOfRows(vtkPVXMLElement* hints, int defaultValue = 10);

  /**
   * Parse a XML element as a list of label to use for this widget.
   * This is usually the XML node named `ShowComponentLabels` in the
   * hints of a proxy. elemCount is a hint of the number of labels to use.
   * Set to 0 to use the number of labels existing in the XML.
   */
  static std::vector<std::string> parseComponentLabels(
    vtkPVXMLElement* hints, unsigned int elemCount = 0);

Q_SIGNALS:
  /**
   * This signal is emitted when the current view changes.
   */
  void viewChanged(pqView* view);

  /**
   * This signal is fired as soon as the user starts editing in the widget. The
   * editing may not be complete.
   */
  void changeAvailable();

  /**
   * This signal is fired as soon as the user is done with making an atomic
   * change. changeAvailable() is always fired before changeFinished().
   */
  void changeFinished();

  /**
   * Indicates that a restart of the program is required for the setting
   * to take effect.
   */
  void restartRequired();

public Q_SLOTS:
  /**
   * called to set the active view. This will fire the viewChanged() signal.
   */
  virtual void setView(pqView*);

protected:
  void addPropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProperty* smproperty, int smindex = -1);
  void addPropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex = -1);
  void removePropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProperty* smproperty, int smindex = -1);
  void removePropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex = -1);
  void setShowLabel(bool show);

  /**
   * For most pqPropertyWidget subclasses a changeAvailable() signal,
   * corresponds to a changeFinished() signal. Hence by default we connect the
   * two together. For subclasses that don't follow this pattern should call
   * this method with 'false' to disconnect changeAvailable() and
   * changeFinished() signals. In that case, the subclass must explicitly fire
   * changeFinished() signal.
   */
  void setChangeAvailableAsChangeFinished(bool status)
  {
    this->ChangeAvailableAsChangeFinished = status;
  }

  /**
   * Register a decorator. The pqPropertyWidget takes over the ownership of the
   * decorator. The decorator will be deleted when the pqPropertyWidget is
   * destroyed.
   */
  void addDecorator(pqPropertyWidgetDecorator*);

  /**
   * Unregisters a decorator.
   */
  void removeDecorator(pqPropertyWidgetDecorator*);

  /**
   * Provides access to the pqPropertyLinks instance.
   */
  pqPropertyLinks& links() { return this->Links; }

public:
  void setProperty(vtkSMProperty* property);

private:
  friend class pqCompositePropertyWidgetDecorator;
  friend class pqPropertyWidgetDecorator;
  friend class pqProxyWidget;

private Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * check if changeFinished() must be fired as well.
   */
  void onChangeAvailable();

private: // NOLINT(readability-redundant-access-specifiers)
  vtkSMProxy* Proxy;
  vtkSMProperty* Property;
  QPointer<pqView> View;
  QList<QPointer<pqPropertyWidgetDecorator>> Decorators;

  pqPropertyLinks Links;
  bool ShowLabel;
  bool ChangeAvailableAsChangeFinished;
  bool Selected;

  const QScopedPointer<pqTimer> Timer;

  /**
   * Deprecated signals. Making private so developers get errors when they
   * use them.
   */
  void modified();
  void editingFinished();
};

#endif // pqPropertyWidget_h
