/*=========================================================================

   Program: ParaView
   Module:    pqFilterInputDialog.h

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

/// \file pqFilterInputDialog.h
/// \date 12/5/2006

#ifndef _pqFilterInputDialog_h
#define _pqFilterInputDialog_h


#include "pqComponentsExport.h"
#include <QDialog>
#include <QList>

class pqFilterInputDialogInternal;
class pqFlatTreeView;
class pqOutputPort;
class pqPipelineBrowserStateManager;
class pqPipelineFilter;
class pqPipelineModel;
class QButtonGroup;
class QGroupBox;
class QItemSelection;
class QLabel;
class QPushButton;
class QScrollArea;
class QString;


class PQCOMPONENTS_EXPORT pqFilterInputDialog : public QDialog
{
  Q_OBJECT

public:
  pqFilterInputDialog(QWidget *parent=0);
  virtual ~pqFilterInputDialog();

  pqPipelineModel *getModel() const {return this->Model;}
  pqPipelineFilter *getFilter() const {return this->Filter;}

  /// Set the original pipeline model, and the filter whose input this dialog is
  /// going to change.
  /// \c namedInputs is the map of input port name to the list of inputs on that
  /// port. This is used as the current state of the selection for the filter
  /// inputs.
  void setModelAndFilter(pqPipelineModel *model, pqPipelineFilter *filter,
    const QMap<QString, QList<pqOutputPort*> > &namedInputs);

  /// Used to obtain the input of inputs for the given input port.
  QList<pqOutputPort*>& getFilterInputs(const QString &port) const;

  bool isInputAcceptable(pqOutputPort*) const;
private slots:
  /// Called when the user selection for the current input port changes.
  void changeCurrentInput(int id);

  void changeInput(const QItemSelection &selected,
      const QItemSelection &deselected);

private:
  pqFilterInputDialogInternal *Internal;
  pqPipelineBrowserStateManager *Manager;
  pqPipelineFilter *Filter;
  pqPipelineModel *Model;
  pqPipelineModel *Pipeline;
  pqFlatTreeView *Sources;
  pqFlatTreeView *Preview;
  QScrollArea *InputFrame;
  QLabel *SourcesLabel;
  QLabel *MultiHint;
  QPushButton *OkButton;
  QPushButton *CancelButton;
  QButtonGroup *InputGroup;
  bool InChangeInput;
};

#endif
