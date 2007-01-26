/*=========================================================================

   Program: ParaView
   Module:    pqObjectInspectorDriver.h

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

/// \file pqObjectInspectorDriver.h
/// \date 1/12/2007

#ifndef _pqObjectInspectorDriver_h
#define _pqObjectInspectorDriver_h


#include "pqComponentsExport.h"
#include <QObject>

class pqConsumerDisplay;
class pqGenericViewModule;
class pqPipelineSource;
class pqProxy;
class pqServerManagerSelectionModel;


/// \class pqObjectInspectorDriver
/// \brief
///   The pqObjectInspectorDriver class uses the server manager
///   selection to signal which object panel to display.
class PQCOMPONENTS_EXPORT pqObjectInspectorDriver : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates an object inspector driver instance.
  /// \param parent The parent object.
  pqObjectInspectorDriver(QObject *parent=0);
  virtual ~pqObjectInspectorDriver() {}

  /// \brief
  ///   Gets whether or not the current is shown for multiple selections.
  /// \return
  ///   True if the current is shown for multiple selections.
  bool isCurrentShownForMultiple() const {return this->ShowCurrent;}

  /// \brief
  ///   Sets whether or not the current is shown for multiple selections.
  /// \param shown True if the current should be shown for multiple
  ///   selections.
  void setCurrentShownForMultiple(bool shown) {this->ShowCurrent = shown;}

  /// \brief
  ///   Sets the server manager selection model to use.
  /// \param model The selection model.
  void setSelectionModel(pqServerManagerSelectionModel *model);

public slots:
  /// \brief
  ///   Sets the active view.
  ///
  /// The active view and the active source are used to determine the
  /// active display.
  ///
  /// \param view The new active view.
  void setActiveView(pqGenericViewModule *view);

signals:
  /// \brief
  ///   Emitted when the object panel to be shown changes.
  /// \param proxy The source to show in the object inspector.
  void sourceChanged(pqProxy *proxy);

  /// \brief
  ///   Emitted when the display to be shown changes.
  /// \param display The display to show.
  /// \param view The view the display is in.
  void displayChanged(pqConsumerDisplay *display, pqGenericViewModule *view);

private slots:
  /// Determines the source to show and emits the signal.
  void updateSource();

  /// \brief
  ///   Checks if the source being removed is the active source.
  ///
  /// If the source being shown is being removed, the \c sourceChanged
  /// signal is emitted.
  ///
  /// \param source The source being removed.
  void checkSource(pqPipelineSource *source);

  /// Checks for a new display on the current source.
  void checkForDisplay();

  /// \brief
  ///   Checks whether or not the current display is being removed.
  /// \param source The source owning the display.
  /// \param display The display being removed.
  void checkDisplay(pqPipelineSource *source, pqConsumerDisplay *display);

private:
  /// \brief
  ///   Sets the current source.
  ///
  /// The current source is used to determine the active display. The
  /// source needs to be monitored for display changes.
  ///
  /// \param source The new active source.
  void setActiveSource(pqPipelineSource *source);

  /// \brief
  ///   Gets the source that should be shown in the object inspector.
  /// \return
  ///   A pointer to the active source.
  pqPipelineSource *findSource() const;

  /// \brief
  ///   Gets the display that should be shown in the display panel.
  /// \return
  ///   A pointer to the active display.
  pqConsumerDisplay *findDisplay() const;

private:
  /// Used to find the selected item(s).
  pqServerManagerSelectionModel *Selection;
  pqPipelineSource *Source;   ///< Stores the active source.
  pqConsumerDisplay *Display; ///< Stores the active display.
  pqGenericViewModule *View;  ///< Stores the active view.
  bool ShowCurrent;           ///< True if the current is shown for multiple.
};

#endif
