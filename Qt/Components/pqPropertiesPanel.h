/*=========================================================================

   Program: ParaView
   Module: pqPropertiesPanel.h

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

#ifndef _pqPropertiesPanel_h
#define _pqPropertiesPanel_h

#include "pqComponentsExport.h"

#include "vtkNew.h"
#include "vtkEventQtSlotConnect.h"

#include <QWidget>
#include <QPointer>

namespace Ui {
  class pqPropertiesPanel;
}

class pqView;
class pqProxy;
class pqOutputPort;
class pqPipelineSource;
class pqPropertyWidget;
class pqRepresentation;
struct pqPropertiesPanelItem;

/// \class pqPropertiesPanel
/// \brief The pqPropertiesPanel class is used to display the properties of
///        proxies and representations in an editable form.
class PQCOMPONENTS_EXPORT pqPropertiesPanel : public QWidget
{
  Q_OBJECT

public:
  /// Creates a new properties panel widget with parent \p p.
  pqPropertiesPanel(QWidget *p = 0);

  /// Destroys the properties panel object.
  virtual ~pqPropertiesPanel();

  /// Returns the current render view.
  pqView* view() const;

public slots:
  /// Sets the current render view to \p view.
  void setView(pqView *view);

  /// Sets the current proxy to \p proxy.
  void setProxy(pqProxy *proxy);

  /// Sets the output port to \p port.
  void setOutputPort(pqOutputPort *port);

  /// Sets the representation to \p repr.
  void setRepresentation(pqRepresentation *repr);

signals:
  // This signal is emitted after the user clicks the apply button.
  void applied();

  /// This signal is emitted when the current view changes.
  void viewChanged(pqView *view);

  /// These signals are emitted when the user clicks the help button.
  void helpRequested(const QString &proxyType);
  void helpRequested(const QString &groupname, const QString &proxyType);

private slots:
  /// Apply the changes properties to the proxies.
  void apply();

  /// Reset the changes made.
  void reset();

  /// Removes the proxy.
  void removeProxy(pqPipelineSource *proxy);

  /// Deletes the current proxy.
  void deleteProxy();

  /// Shows the help dialog.
  void showHelp();

  /// Sets the enabled state of the delete button.
  void handleConnectionChanged(pqPipelineSource *in, pqPipelineSource *out);

  /// Called when the search string changes.
  void searchTextChanged(const QString &string);

  /// Called when the advanced button is toggled.
  void advancedButtonToggled(bool state);

  // Called when the representation type changes.
  void representationPropertyChanged(vtkObject *object, unsigned long event, void *data);

  /// Updates the state of the apply button.
  void updateButtonState();

  /// Called when a property on the current proxy changes.
  void proxyPropertyChanged();

private:
  /// Shows the source.
  void show(pqPipelineSource *source);

  /// Creates and returns a group of property widgets for \p proxy.
  QList<pqPropertiesPanelItem> createWidgetsForProxy(pqProxy *proxy);

  /// Returns \c true if \p item should be visible.
  bool isPanelItemVisible(const pqPropertiesPanelItem &item) const;

private:
  Ui::pqPropertiesPanel *Ui;
  QPointer<pqView> View;
  QPointer<pqProxy> Proxy;
  pqOutputPort *OutputPort;
  QPointer<pqRepresentation> Representation;
  vtkNew<vtkEventQtSlotConnect> RepresentationTypeSignal;
  QList<pqPropertiesPanelItem> ProxyPropertyItems;
  QList<pqPropertiesPanelItem> RepresentationPropertyItems;
};

#endif // _pqPropertiesPanel_h
