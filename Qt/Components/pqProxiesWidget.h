/*=========================================================================

   Program: ParaView
   Module:  pqProxiesWidget.h

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
#ifndef pqProxiesWidget_h
#define pqProxiesWidget_h

#include "pqComponentsModule.h"
#include <QScopedPointer>
#include <QWidget>

class vtkSMProxy;
class pqView;

/**
* pqProxiesWidget similar to pqProxyWidget with the difference being that
* unlink pqProxyWidget, pqProxiesWidget supports showing of multiple proxies in
* the same widget. Internally, it indeed creates a pqProxyWidget for each of
* the added proxies.
*
* To use this class, add all proxies to show in this panel using addProxy()
* and then call updateLayout() to update the layout.
*
* pqProxiesWidget supports grouping of proxies in components (separated by
* using a pqExpanderButton). To use pqExpanderButton simply use a non-empty
* componentName when calling addProxy().
*
* pqProxiesWidget provides API such as filterWidgets(), apply(), etc. the
* calls to which are simply forwarded to the internal pqProxyWidget instances.
* Similarly, pqProxiesWidget also forwards signals fired by pqProxyWidget such
* as changeFinished(), changeAvailable(), and restartRequired().
*/
class PQCOMPONENTS_EXPORT pqProxiesWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqProxiesWidget(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  virtual ~pqProxiesWidget();

public slots:
  /**
  * Remove all proxy widgets added to the panel.
  */
  void clear();

  /**
  * Add the widgets for a proxy.
  */
  void addProxy(vtkSMProxy*, const QString& componentName = QString(),
    const QStringList& properties = QStringList(), bool applyChangesImmediately = false);

  /**
  * Call this method once after all proxies have been added (or after clear)
  * to update the layout for the panel.
  */
  void updateLayout();

  /**
  * Updates the property widgets shown based on the filterText or
  * show_advanced flag. Calling filterWidgets() without any arguments will
  * result in the panel showing all the non-advanced properties.
  * Returns true, if any widgets were shown.
  */
  bool filterWidgets(bool show_advanced = false, const QString& filterText = QString());

  /**
  * Accepts the property widget changes changes.
  */
  void apply() const;

  /**
  * Cleans the property widget changes and resets the widgets.
  */
  void reset() const;

  /**
  * Set the current view to use to show 3D widgets, if any for the panel.
  */
  void setView(pqView*);

  /**
  * Same as calling filterWidgets() with the arguments specified to the most
  * recent call to filterWidgets().
  */
  void updatePanel();

signals:
  /**
  * This signal is fired as soon as the user starts editing in the widget. The
  * editing may not be complete.
  */
  void changeAvailable(vtkSMProxy* proxy);

  /**
  * This signal is fired as soon as the user is done with making an atomic
  * change. changeAvailable() is always fired before changeFinished().
  */
  void changeFinished(vtkSMProxy* proxy);

  /**
  * Indicates that a restart of the program is required for the setting
  * to take effect.
  */
  void restartRequired(vtkSMProxy* proxy);

private slots:
  void triggerChangeFinished();
  void triggerChangeAvailable();
  void triggerRestartRequired();

private:
  Q_DISABLE_COPY(pqProxiesWidget)

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
