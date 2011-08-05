/*=========================================================================

   Program: ParaView
   Module:    pqDisplayColorWidget.h

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

#ifndef _pqDisplayColorWidget_h
#define _pqDisplayColorWidget_h

#include "pqVariableType.h"
#include "pqComponentsExport.h"

#include <QWidget>
#include <QPointer>

class QComboBox;
class QHBoxLayout;

class pqDataRepresentation;
class pqPipelineRepresentation;
class pqTriggerOnIdleHelper;
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
  void addVariable(pqVariableType type, const QString& name, bool is_partial);

  /// Makes the given variable the "current" selection.  Emits the 
  /// variableChanged() signal.
  void chooseVariable(pqVariableType type, const QString& name);

  /// Returns the display whose color this widget is currently 
  /// editing.
  pqPipelineRepresentation* getRepresentation() const;

  /// Returns the current text in the combo box.
  QString getCurrentText() const;

public slots:
  /// Called when the variable selection changes. 
  void onVariableChanged(pqVariableType type, const QString& name);

  /// When set, the source/renModule is not used to locate the
  /// display, instead this display is used.
  void setRepresentation(pqDataRepresentation* display);

  /// Called when the GUI must reload the arrays shown in the widget.
  /// i.e. this updates the domain for the combo box.
  void reloadGUI();

signals:
  /// Signal emitted whenever the user chooses a variable, 
  /// or chooseVariable() is called.
  /// This signal is fired only when the change is triggered by the wiget
  /// i.e. if the ServerManager property changes, this signal won't be fired,
  /// use \c modified() instead.
  void variableChanged(pqVariableType type, const QString& name);

  /// Fired when ever the color mode on the display changes
  /// either thorough this widget itself, of when the underlying SMObject
  /// changes.
  void modified();

private slots:
  /// Called to emit the variableChanged() signal in response to user input 
  /// or the chooseVariable() method.
  void onVariableActivated(int row);
  void onComponentActivated(int row);

  /// Called when any important property on the display changes.
  /// This updates the selected value.
  void updateGUI();
  void updateComponents();

  void reloadGUIInternal();
private:
  /// Converts a variable type and name into a packed string representation 
  /// that can be used with a combo box.
  static const QStringList variableData(pqVariableType, const QString& name);
 
  QIcon* CellDataIcon;
  QIcon* PointDataIcon;
  QIcon* SolidColorIcon;

  QHBoxLayout* Layout;
  QComboBox* Variables;
  QComboBox* Components;
  int BlockEmission;
  bool Updating;
  vtkEventQtSlotConnect* VTKConnect;
  QPointer<pqPipelineRepresentation> Representation;
  QList<QString> AvailableArrays;
  pqTriggerOnIdleHelper* ReloadGUIHelper;
};

#endif
