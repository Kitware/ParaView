/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqDisplayArrayWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqDisplayArrayWidget
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>


#ifndef _pqDisplayArrayWidget_h
#define _pqDisplayArrayWidget_h

#include "pqVariableType.h"
#include "pqComponentsExport.h"

#include <QWidget>
#include <QPointer>

class QComboBox;
class QHBoxLayout;

class pqDataRepresentation;
class pqPipelineRepresentation;
class vtkEventQtSlotConnect;
class pqScalarsToColors;

/// Provides a standard user interface for selecting among a collection
/// of dataset variables and .
class PQCOMPONENTS_EXPORT pqDisplayArrayWidget: public QWidget
{
  Q_OBJECT

public:
  pqDisplayArrayWidget( QWidget *parent=0 );
  ~pqDisplayArrayWidget();

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

  /// Set/Get the name to associate to non varying value.
  void setConstantVariableName(const QString& name);
  const QString& getConstantVariableName() const;

  // Set/Get the name of the property that controls the array name
  void  setPropertyArrayName(const QString&);
  const QString& propertyArrayName();

  // Set/Get the name of the property that controals the array component
  void  setPropertyArrayComponent(const QString&);
  const QString& propertyArrayComponent();

  QString currentVariableName();
  pqVariableType currentVariableType();

  void  setToolTip(const QString&);

public slots:
  /// set the display to get possible arrays.
  void setRepresentation(pqPipelineRepresentation* display);

  /// Called when the GUI must reload the arrays shown in the widget.
  /// i.e. this updates the domain for the combo box.
  void reloadGUI();
  void reloadComponents();

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

  // Fired when the component has been modified
  // vectorMode is either vtkScalarToColors::MAGNITUDE
  // or vtkScalarToColors::COMPONENT.
  // comp is the component index.
  void componentChanged(int vectorMode, int comp);

protected slots:
  /// Called to emit the variableChanged() signal in response to user input
  /// or the chooseVariable() method.
  virtual void onVariableActivated(int row);
  virtual void onComponentActivated(int row);

  /// Called when any important property on the display changes.
  /// This updates the selected value.
  virtual void updateGUI();
  virtual void needReloadGUI();
  virtual void updateComponents();

protected:
  /// Converts a variable type and name into a packed string representation
  /// that can be used with a combo box.
  static const QStringList variableData(pqVariableType, const QString& name);

  const QString getArrayName() const;

private:
  class pqInternal;
  pqInternal* Internal;

};

#endif
