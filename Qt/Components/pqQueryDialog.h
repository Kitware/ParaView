/*=========================================================================

   Program: ParaView
   Module:    pqQueryDialog.h

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
#ifndef __pqQueryDialog_h 
#define __pqQueryDialog_h

#include <QDialog>
#include "pqComponentsExport.h"

class pqOutputPort;
class pqQueryClauseWidget;
class vtkPVDataSetAttributesInformation;
class vtkSMProxy;
class pqView;
class pqRepresentation;

/// pqQueryDialog is the dialog that allows the user to query/search for
/// cells/points satisfying a particular criteria.
/// The user is searching for data matching the given criteria in the output of
/// the set source/filter.
class PQCOMPONENTS_EXPORT pqQueryDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;
public:
  /// \c producer cannot be NULL.
  pqQueryDialog(pqOutputPort* producer,
    QWidget* parent=0, Qt::WindowFlags flags=0);
  virtual ~pqQueryDialog();

  /// Get the source whose data is to be queried.
  pqOutputPort* producer() const
    { return this->Producer; }

signals:
  /// fired every time user submits a query.
  void selected(pqOutputPort*);

  /// fired everytime the user click on the ExtractSelection button
  void extractSelection();
  /// fired everytime the user click on the ExtractSelectionOverTime button
  void extractSelectionOverTime();

  /// Fired when the user clicks on the help button.
  void helpRequested();

protected slots:
  /// Must be triggered before server disconnect to release all SMProxy links
  void freeSMProxy();

  /// Triggered when the data to process has changed
  void onSelectionChange(pqOutputPort*);

  /// Triggerd when the active view has changed
  void onActiveViewChanged(pqView*);

  /// Based on the data type produced by the producer, this will update the
  /// options in the selection type combo-box.
  void populateSelectionType();

  /// reset the currently chosen clauses
  void resetClauses();

  /// adds a new clause.
  void addClause();

  /// Called when user click the "Run Query" button.
  void runQuery();

  /// Called when user selects a label item.
  void setLabel(int index);

  void onExtractSelection()
    { 
    this->extractSelection();
    this->accept();
    }
  void onExtractSelectionOverTime()
    {
    this->extractSelectionOverTime();
    this->accept();
    }

protected:
  /// populate the list of available labels.
  void updateLabels();

  /// link the label-color widget with the active label-color property.
  void linkLabelColorWidget(vtkSMProxy*, const QString& propname);

  /// creates the proxies needed for the spreadsheet view.
  void setupSpreadSheet();

private:
  Q_DISABLE_COPY(pqQueryDialog)
  class pqInternals;

  pqOutputPort* Producer;
  pqInternals* Internals;
};

#endif


