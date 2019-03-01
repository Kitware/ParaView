/*=========================================================================

   Program: ParaView
   Module: pqPropertyWidget.h

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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
#ifndef _pqPropertyWidget_h
#define _pqPropertyWidget_h

#include "pqComponentsModule.h"
#include "vtkSetGet.h" // for VTK_LEGACY_REMOVE.

#include "pqPropertyLinks.h"
#include <QPointer>
#include <QScopedPointer>
#include <QWidget>

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
class PQCOMPONENTS_EXPORT pqPropertyWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqPropertyWidget(vtkSMProxy* proxy, QWidget* parent = 0);
  ~pqPropertyWidget() override;

  virtual void apply();
  virtual void reset();

  /**
  * These methods are called by pqPropertiesPanel when the panel for proxy
  * becomes active/deactive.
  * Only widgets that have 3D widgets need to
  * override these methods to select/deselect the 3D widgets.
  * Default implementation does nothing.
  */
  virtual void select() { this->Selected = true; }
  virtual void deselect() { this->Selected = false; }
  bool isSelected() const { return this->Selected; }

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
  *   to forward calls to vtkSMPropertyGroup
  */
  virtual char* panelVisibility() const;
  virtual void setPanelVisibility(const char* vis);

  bool showLabel() const;

  /**
  * Description:
  * This static utility method returns the XML name for an object as
  * a QString. This allows for code to get the XML name of an object
  * without having to explicitly check for a possibly NULL char* pointer.
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
  const QList<QPointer<pqPropertyWidgetDecorator> >& decorators() const { return this->Decorators; }

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

signals:
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

public slots:
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

private slots:
  /**
  * check if changeFinished() must be fired as well.
  */
  void onChangeAvailable();

private:
  vtkSMProxy* Proxy;
  vtkSMProperty* Property;
  QPointer<pqView> View;
  QList<QPointer<pqPropertyWidgetDecorator> > Decorators;

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

#if !defined(VTK_LEGACY_REMOVE)
#include "pqDebug.h"
#define PV_DEBUG_PANELS() pqDebug("PV_DEBUG_PANELS")
#endif

#endif // _pqPropertyWidget_h
