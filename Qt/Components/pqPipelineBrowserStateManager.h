/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowserStateManager.h

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

/// \file pqPipelineBrowserStateManager.h
/// \date 1/10/2007

#ifndef _pqPipelineBrowserStateManager_h
#define _pqPipelineBrowserStateManager_h


#include "pqComponentsExport.h"
#include <QObject>

class pqFlatTreeView;
class pqPipelineBrowserStateManagerInternal;
class pqPipelineModel;
class QModelIndex;
class vtkPVXMLElement;


/// \class pqPipelineBrowserStateManager
/// \brief
///   The pqPipelineBrowserStateManager class is used to save and
///   restore the view state.
class PQCOMPONENTS_EXPORT pqPipelineBrowserStateManager : public QObject
{
  Q_OBJECT

public:
  pqPipelineBrowserStateManager(QObject *parent=0);
  virtual ~pqPipelineBrowserStateManager();

  void setModelAndView(pqPipelineModel *model, pqFlatTreeView *view);

  void saveState(vtkPVXMLElement *root) const;
  void restoreState(vtkPVXMLElement *root);

public slots:
  void saveState(const QModelIndex &index);
  void restoreState(const QModelIndex &index);

private:
  void saveState(const QModelIndex &index, vtkPVXMLElement *root) const;
  void restoreState(const QModelIndex &index, vtkPVXMLElement *root);

private:
  pqPipelineBrowserStateManagerInternal *Internal;
  pqPipelineModel *Model;
  pqFlatTreeView *View;
};

#endif
