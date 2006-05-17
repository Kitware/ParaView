/*=========================================================================

   Program:   ParaQ
   Module:    pqVariableSelectorWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqVariableSelectorWidget_h
#define _pqVariableSelectorWidget_h

#include "pqVariableType.h"
#include "pqWidgetsExport.h"

#include <QWidget>
#include <QPointer>

class QComboBox;
class QHBoxLayout;

class pqPipelineSource;

/// Provides a standard user interface for selecting among a collection 
/// of dataset variables (both cell and node variables).
class PQWIDGETS_EXPORT pqVariableSelectorWidget : public QWidget
{
  Q_OBJECT

public:
  pqVariableSelectorWidget( QWidget *parent=0 );
  ~pqVariableSelectorWidget();
  
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

signals:
  /// Signal emitted whenever the user chooses a variable, or chooseVariable() is called.
  void variableChanged(pqVariableType type, const QString& name);

private slots:
  /// Called to emit the variableChanged() signal in response to user input or the chooseVariable() method.
  void onVariableActivated(int row);

private:
  /// Converts a variable type and name into a packed string representation that can be used with a combo box.
  static const QString variableData(pqVariableType, const QString& name);
  
  QHBoxLayout* Layout;
  QComboBox* Variables;
  bool BlockEmission;
  QPointer<pqPipelineSource> SelectedSource;
  bool IgnoreWidgetChanges;
};

#endif
