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

#include <string>
#include <sstream>

#include <QWidget>
#include <QTextStream>

#include "pqPropertyLinks.h"

class pqView;
class vtkSMProxy;
class vtkSMDomain;
class vtkSMProperty;

class PQCOMPONENTS_EXPORT pqPropertyWidget : public QWidget
{
  Q_OBJECT

public:
  pqPropertyWidget(vtkSMProxy *proxy, QWidget *parent = 0);
  virtual ~pqPropertyWidget();

  virtual void apply();
  virtual void reset();

  /// These methods are called when the pqProxyPropertiesPanel containing the
  /// widget is activated/deactivated. Only widgets that have 3D widgets need to
  /// override these methods to select/deselect the 3D widgets.
  /// Default implementation does nothing.
  virtual void select() {}
  virtual void deselect() {}

  pqView* view() const;
  vtkSMProxy* proxy() const;
  vtkSMProperty* property() const;

  bool showLabel() const;

  // Description:
  // This static utility method returns the XML name for an object as
  // a std::string. This allows for code to get the XML name of an object
  // without having to explicitly check for a possibly NULL char* pointer.
  //
  // This is templated so that it will work with a variety of objects such
  // as vtkSMProperty's and vtkSMDomain's. It can be called with anything
  // that has a "char* GetXMLName()" method.
  //
  // For example, to get the XML name of a vtkSMIntRangeDomain:
  // std::string name = pqPropertyWidget::getXMLName(domain);
  template<class T>
  static std::string getXMLName(T *object)
  {
    return object->GetXMLName() ? object->GetXMLName() : std::string();
  }

signals:
  /// This signal is emitted when the widget's value is changed by the user.
  void modified();

  /// This signal is emitted when the user is finished editing a property
  /// either by pressing enter or focusing away from the widget.
  void editingFinished();

  /// This signal is emitted when the current view changes.
  void viewChanged(pqView *view);

public slots:
  void updateDependentDomains();

protected:
  void addPropertyLink(QObject *qobject,
                       const char *qproperty,
                       const char *qsignal,
                       vtkSMProperty *smproperty,
                       int smindex = -1);
  void setShowLabel(bool show);

  void setReason(const QString &message);
  std::stringstream& setReason();
  QString reason() const;

private:
  void setAutoUpdateVTKObjects(bool autoUpdate);
  void setUseUncheckedProperties(bool useUnchecked);
  void setProperty(vtkSMProperty *property);

  friend class pqPropertiesPanel;

private:
  vtkSMProxy *Proxy;
  vtkSMProperty *Property;
  pqPropertyLinks Links;
  bool ShowLabel;
  std::stringstream Reason;
};

#endif // _pqPropertyWidget_h
