/*=========================================================================

   Program: ParaView
   Module:    pqSourceComboBox.h

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

========================================================================*/
#ifndef __pqSourceComboBox_h 
#define __pqSourceComboBox_h

#include <QComboBox>
#include "pqComponentsExport.h"

class pqServerManagerModelItem;
class pqPipelineSource;
class vtkSMProxy;

/// Many times we need a combo box that shows the list of currently available
/// source/filters. pqSourceComboBox can be used in such scenarios. It also
/// supports updating the currently selected entry with the application
/// selection. For this to work, one must connect the addSource and removeSource
/// slots to appropriate signals, typically those from pqServerManagerModel.
class PQCOMPONENTS_EXPORT pqSourceComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;
public:
  pqSourceComboBox(QWidget* parent=0);
  ~pqSourceComboBox();

  /// When set, the current source in this combo will be updated to the current
  /// item (if any) selected on the selection model maintained by the application
  /// core. false by default.
  void setUpdateCurrentWithSelection(bool state)
    { this->UpdateCurrentWithSelection = state; }

  /// When set, when ever the current item in this combo changes, we update the
  /// selection model maintained by the application core to reflect the
  /// change in current. false by default.
  void setUpdateSelectionWithCurrent(bool state)
    { this->UpdateSelectionWithCurrent = state; }

  /// When set to something other than the empty string, only allows sources
  /// which output a specific data type, empty by default.
  void setAllowedDataType(const QString& type)
    { this->AllowedDataType = type; }
  QString allowedDataType()
    { return this->AllowedDataType; }

  /// Returns the currently selected source, if any.
  pqPipelineSource* currentSource() const;

public slots:
  /// Called when a new source is added.
  void addSource(pqPipelineSource* source);

  /// Called when a new source is removed.
  void removeSource(pqPipelineSource* source);

  /// Change the current item to the indicated source.
  void setCurrentSource(pqPipelineSource* source);
  void setCurrentSource(vtkSMProxy* source);

  /// Populate the combobox with all relevant sources.
  void populateComboBox();

signals:
  /// Fired when a new source is added.
  void sourceAdded(pqPipelineSource*);

  /// Fired when a source is removed.
  void sourceRemoved(pqPipelineSource*);
  
  /// Fired when a source is renamed.
  void renamed(pqPipelineSource*);

  /// Fired when the current index changes.
  void currentIndexChanged(pqPipelineSource*);
  void currentIndexChanged(vtkSMProxy*);

protected slots:
  /// Called when a source's name might have changed.
  void nameChanged(pqServerManagerModelItem* item);

  /// Called when current in the server manager selection changes.
  void onCurrentChanged(pqServerManagerModelItem* item);

  /// Called when currentIndexChanged(int) is fired.
  /// We fire currentIndexChanged(pqPipelineSource*) and
  //currentIndexChanged(vtkSMProxy*);
  void onCurrentIndexChanged(int index);
protected:
  bool UpdateCurrentWithSelection;
  bool UpdateSelectionWithCurrent;
  QString AllowedDataType;

private:
  pqSourceComboBox(const pqSourceComboBox&); // Not implemented.
  void operator=(const pqSourceComboBox&); // Not implemented.
};

#endif


