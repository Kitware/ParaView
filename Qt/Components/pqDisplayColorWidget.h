/*=========================================================================

   Program: ParaView
   Module:    pqDisplayColorWidget.h

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

#ifndef _pqDisplayColorWidget_h
#define _pqDisplayColorWidget_h

#include "pqVariableType.h"
#include "pqComponentsExport.h"

#include <QWidget>
#include <QPointer>

class QComboBox;
class QHBoxLayout;

class pqPipelineDisplay;
class pqPipelineSource;
class pqRenderModule;
class vtkEventQtSlotConnect;

/// Provides a standard user interface for selecting among a collection 
/// of dataset variables (both cell and node variables).
class PQCOMPONENTS_EXPORT pqDisplayColorWidget : public QWidget
{
  Q_OBJECT

public:
  pqDisplayColorWidget( QWidget *parent=0 );
  ~pqDisplayColorWidget();
  
  /// Removes all variables from the collection.
  void clear();

  /// Adds a variable to the collection.
  void addVariable(pqVariableType type, const QString& name);

  /// Makes the given variable the "current" selection.  Emits the 
  /// variableChanged() signal.
  void chooseVariable(pqVariableType type, const QString& name);
 
public slots:
  /// Call to update the variable selector to show the variables
  /// provided by the \c source. \c source can be NULL.
  void updateVariableSelector(pqPipelineSource* source);

  /// Called when the variable selection changes. 
  /// Affects the \c SelectedSource
  void onVariableChanged(pqVariableType type, const QString& name);

  /// Called to set the current render module. This widget
  /// shows the source's display properties in this render module 
  /// alone.
  void setRenderModule(pqRenderModule* renModule);

signals:
  /// Signal emitted whenever the user chooses a variable, 
  /// or chooseVariable() is called.
  void variableChanged(pqVariableType type, const QString& name);

private slots:
  /// Called to emit the variableChanged() signal in response to user input 
  /// or the chooseVariable() method.
  void onVariableActivated(int row);

  /// Called when any important property on the display changes.
  /// This updates the selected value.
  void updateGUI();

  /// Called when the GUI must reload the arrays shown in the widget.
  void reloadGUI();

  /// Called when a display gets added to the selected render module.
  /// Since a source may not have any display in a particular render module
  /// when the rendermodule becomes active, we listen to this signal to 
  /// update the GUI when a display gets added.
  void displayAdded();
private:
  /// Converts a variable type and name into a packed string representation 
  /// that can be used with a combo box.
  static const QString variableData(pqVariableType, const QString& name);
 
  QIcon* CellDataIcon;
  QIcon* PointDataIcon;
  QIcon* SolidColorIcon;

  QHBoxLayout* Layout;
  QComboBox* Variables;
  bool BlockEmission;
  bool PendingDisplayPropertyConnections;
  QPointer<pqPipelineSource> SelectedSource;
  vtkEventQtSlotConnect* VTKConnect;
  QPointer<pqRenderModule> RenderModule;
};

#endif
