/*=========================================================================

   Program: ParaView
   Module:    pqSummaryPanel.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#ifndef _pqSummaryPanel_h
#define _pqSummaryPanel_h

#include <QWidget>
#include <QMap>
#include <QPointer>

#include "pqComponentsExport.h"
#include "pqDisplayRepresentationWidget.h"
#include "pqObjectPanel.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqCollapsedGroup.h"

class QComboBox;
class QFrame;
class QGridLayout;
class QGroupBox;
class QPushButton;
class pqOutputPort;
class pqPropertyLinks;
class pqRepresentation;
class pqSignalAdaptorComboBox;

/// \class pqSummaryPanel
/// \brief The pqSummaryPanel class is used to display a subset
///        of proxy and representation properties to the user.
class PQCOMPONENTS_EXPORT pqSummaryPanel : public QWidget
{
  Q_OBJECT

public:
  /// Creates a new summary panel widget.
  pqSummaryPanel(QWidget *parent = 0);

  /// Destroys the summary panel widget.
  ~pqSummaryPanel();

  /// Returns the current view.
  pqView* view() const;

  /// Returns the current proxy.
  pqProxy* proxy() const;

  /// Returns the current representation.
  pqRepresentation* representation() const;

public slots:
  /// Sets the view.
  void setView(pqView* view);

  /// Sets the proxy.
  void setProxy(pqProxy *proxy);

  /// Sets the representation.
  void setRepresentation(pqDataRepresentation *representation);

  /// Sets the output port.
  void setOutputPort(pqOutputPort *port);

  /// Causes the current property settings to be applied.
  void accept();

  /// Resets the current property settings to their previous value.
  void reset();

  /// Sets the enabled state for the apply button.
  void canAccept(bool status);

  /// Updates the enabled state for the delete button.
  void updateDeleteButtonState();

signals:
  /// This signal is emitted when the view changes.
  void viewChanged(pqView *view);

  /// This signal is emitted when the help button is clicked.
  void helpRequested(const QString& proxyType);

protected slots:
  void removeProxy(pqPipelineSource* proxy);
  void deleteProxy();
  void handleConnectionChanged(pqPipelineSource* in, pqPipelineSource* out);
  void updateAcceptState();
  void show(pqPipelineSource *source);

private slots:
  void representionComboBoxChanged(const QString &text);
  void representationChanged(pqDataRepresentation *representation);

private:
  QWidget* createPropertiesPanel();
  QWidget* createButtonBox();
  QWidget* createRepresentationFrame();
  QWidget* createDisplayPanel();
  pqObjectPanel* createSummaryPropertiesPanel(pqProxy *proxy);
  QWidget* createSummaryDisplayPanel(pqDataRepresentation *representation);

private:
  pqView *View;
  bool ShowOnAccept;
  pqObjectPanel *CurrentPanel;
  QMap<pqProxy*, QPointer<pqObjectPanel> > PanelStore;
  pqProxy *Proxy;
  pqDataRepresentation *Representation;
  pqOutputPort *OutputPort;
  pqPropertyLinks *Links;
  QGridLayout *PropertiesLayout;
  QGridLayout *DisplayLayout;
  QWidget *DisplayWidget;
  pqSignalAdaptorComboBox *RepresentationSignalAdaptor;
  pqCollapsedGroup *PropertiesPanelFrame;
  QFrame *ButtonBoxFrame;
  QPushButton *AcceptButton;
  QPushButton *ResetButton;
  QPushButton *DeleteButton;
  QFrame *RepresentationFrame;
  pqDisplayRepresentationWidget *RepresentationSelector;
  pqCollapsedGroup *DisplayPanelFrame;
};

#endif
