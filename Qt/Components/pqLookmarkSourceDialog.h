/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkSourceDialog.h

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

/// \file pqLookmarkSourceDialog.h
/// \date 12/5/2006

#ifndef _pqLookmarkSourceDialog_h
#define _pqLookmarkSourceDialog_h


#include "pqComponentsExport.h"
#include <QDialog>

class pqFlatTreeView;
class pqPipelineBrowserStateManager;
class pqPipelineSource;
class pqPipelineModel;
class QGroupBox;
class QItemSelection;
class QLabel;
class QPushButton;
class QString;
class QStringList;
class QStandardItemModel;
class QStandardItem;

/// \class pqLookmarkSourceDialog
/// \brief
///   Select which non-filter, non-server, pipeline source to apply a lookmark to.
///

class PQCOMPONENTS_EXPORT pqLookmarkSourceDialog : public QDialog
{
  Q_OBJECT

public:
  pqLookmarkSourceDialog(QStandardItemModel *lookmarkModel, pqPipelineModel *pipelineModel, QWidget *parent=0);

  // Tell it which source in the lookmark pipeline model is being replaced. item's text gets set to bold to distinguish it from the rest. 
  // Should be called before exec().
  void setLookmarkSource(QStandardItem *item);

  // Called after the dialog is accepted to get the source the user selected
  pqPipelineSource* getSelectedSource(){return this->SelectedSource;};

private slots:
  void selectSource(const QItemSelection &selection);

protected:
  void setModels(QStandardItemModel *lookmarkModel, pqPipelineModel *pipelineModel);

private:
  QStandardItemModel *LookmarkPipelineModel;
  pqPipelineModel *CurrentPipelineModel;
  pqFlatTreeView *CurrentPipelineView;
  pqFlatTreeView *LookmarkPipelineView;
  QStandardItem *CurrentLookmarkItem;
  pqPipelineSource *SelectedSource;
  QLabel *CurrentPipelineViewLabel;
  QPushButton *OkButton;

};

#endif
