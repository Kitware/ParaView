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

class pqFilterInputDialogInternal;
class pqFlatTreeView;
class pqPipelineFilter;
class pqPipelineModel;
class QButtonGroup;
class QGroupBox;
class QItemSelection;
class QPushButton;
class QScrollArea;
class QString;
class QStringList;


class PQCOMPONENTS_EXPORT pqFilterInputDialog : public QDialog
{
  Q_OBJECT

public:
  pqFilterInputDialog(QWidget *parent=0);
  virtual ~pqFilterInputDialog() {}

  pqPipelineModel *getModel() const {return this->Model;}
  pqPipelineFilter *getFilter() const {return this->Filter;}
  void setModelAndFilter(pqPipelineModel *model, pqPipelineFilter *filter);

  void getFilterInputPorts(QStringList &ports) const;
  void getFilterInputs(const QString &port, QStringList &inputs) const;
  void getCurrentFilterInputs(const QString &port, QStringList &inputs) const;

private slots:
  void changeCurrentInput(int id);
  void changeInput(const QItemSelection &selected,
      const QItemSelection &deselected);

private:
  pqFilterInputDialogInternal *Internal;
  pqPipelineModel *Model;
  pqPipelineFilter *Filter;
  pqFlatTreeView *TreeView;
  QGroupBox *FilterBox;
  QScrollArea *InputFrame;
  QPushButton *OkButton;
  QPushButton *CancelButton;
  QButtonGroup *InputGroup;
  bool InChangeInput;
};

#endif
