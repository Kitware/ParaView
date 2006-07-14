/*=========================================================================

   Program: ParaView
   Module:    pq3DWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pq3DWidget_h
#define __pq3DWidget_h

#include <QWidget>
#include "pqWidgetsExport.h"
#include "pqSMProxy.h"

class vtkPVXMLElement;
class vtkSMProperty;
class pqProxy;
class vtkSMNew3DWidgetProxy;
class pq3DWidgetInternal;

/// pq3DWidget is the abstract superclass for all 3D widgets.
/// This class represents a 3D Widget proxy as well as the GUI for the
/// widget.
class PQWIDGETS_EXPORT pq3DWidget : public QWidget
{
  Q_OBJECT

public:
  pq3DWidget(QWidget* parent=0);
  virtual ~pq3DWidget();

  // This method creates widgets using the hints provided by 
  // the proxy. If a proxy needs more that one
  // 3D widget, this method will create all the 3D widgets and
  // return them. There is no parent associated with the newly
  // created 3D widgets, it's the responsibility of the caller
  // to do the memory management for the 3D widgets.
  static QList<pq3DWidget*> createWidgets(vtkSMProxy* proxy);

  /// Reference proxy is a proxy which is used to determine the bounds
  /// for the 3D widget.
  virtual void setReferenceProxy(pqProxy*);
  pqProxy* getReferenceProxy() const;

  /// Controlled proxy is a proxy which is controlled by the 3D widget.
  /// A controlled proxy must provide "Hints" describing how
  /// the properties of the controlled proxy are controlled by the
  /// 3D widget.
  virtual void setControlledProxy(vtkSMProxy*);
  vtkSMProxy* getControlledProxy() const;

  /// Set the hints XML to be using to map the 3D widget to the controlled
  /// proxy. This method must be called only after the controlled
  /// proxy has been set. The argument is the element
  /// <PropertyGroup /> which will be controlled by this widget.
  void setHints(vtkPVXMLElement* element);

  /// Return the 3D Widget proxy.
  vtkSMNew3DWidgetProxy* getWidgetProxy() const;

  /// Returns if the 3D widget is visible for the current
  /// reference proxy.
  bool widgetVisibile() const;

signals:
  /// Notifies observers that the user is dragging the 3D widget
  void widgetStartInteraction();
  /// Notifies observers that the widget has been modified
  void widgetChanged();
  /// Notifies observers that the user is done dragging the 3D widget
  void widgetEndInteraction();

public slots:
  /// Makes the 3D widget visible. 
  virtual void showWidget();

  /// Hides the 3D widget.
  virtual void hideWidget();

  /// Activates the widget. Respects the visibility flag.
  virtual void select();

  /// Deactivates the widget.
  virtual void deselect();

  /// Accepts pending changes. Default implementation
  /// pushes the 3D widget information property values to
  /// the corresponding properties on the controlled widget.
  /// The correspondence is determined from the <Hints />
  /// associated with the controlled proxy.
  virtual void accept();

  /// Resets pending changes. Default implementation
  /// pushes the property values of the controlled widget to the 
  /// 3D widget properties.
  /// The correspondence is determined from the <Hints />
  /// associated with the controlled proxy.
  virtual void reset();

protected:
  /// Subclasses can override this method to map properties to
  /// GUI. Default implementation updates the internal datastructures
  /// so that default implementations can be provided for 
  /// accept/reset.
  virtual void setControlledProperty(const char* function,
    vtkSMProperty * controlled_property);

  /// Internal method to change the widget visibility.
  /// Changes made to visibility using this method are not saved in the
  /// internal datastructure, use showWidget()/hideWidget() if you want
  /// the change to be recorded so that it will be respected when
  /// the panel calls select().
  virtual void set3DWidgetVisibility(bool visible);

  // Subclasses must set the widget proxy.
  void setWidgetProxy(vtkSMNew3DWidgetProxy*);

  /// Called when one of the controlled properties change (e.g: by undo/redo)
  virtual void onControlledPropertyChanged();

  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  virtual void resetBounds() =0;

  /// Used to avoid recursion when updating the controlled properties
  bool IgnorePropertyChange;
  
private:
  pq3DWidgetInternal* Internal;
};


#endif

