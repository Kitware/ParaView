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

signals:
  /// \brief
  ///   Emitted when the object panel to be shown changes.
  /// \param proxy The object to show in the object inspector.
  void objectChanged(pqProxy *proxy);

private slots:
  /// Determines the object to show and emits the signal.
  void updateObject();

  /// \brief
  ///   Checks if the object being removed is the shown object.
  ///
  /// If the object being shown is being removed, the \c objectChanged
  /// signal is emitted.
  ///
  /// \param source The object being removed.
  void checkObject(pqPipelineSource *source);

private:
  /// \brief
  ///   Gets the object that should be shown in the object inspector.
  /// \return
  ///   A pointer to the object to show.
  pqProxy *getObject() const;

private:
  /// Used to find the selected item(s).
  pqServerManagerSelectionModel *Selection;
  bool ShowCurrent; ///< True if the current is shown for multiple.
};

#endif
