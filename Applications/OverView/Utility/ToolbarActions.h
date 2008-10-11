/*=========================================================================

   Program: ParaView
   Module:    ToolbarActions.h

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
#ifndef _ToolbarActions_h
#define _ToolbarActions_h

#include "OverViewUtilityExport.h"

#include <QActionGroup>
#include <QString>

class pqPipelineSource;

class OVERVIEW_UTILITY_EXPORT ToolbarActions : public QActionGroup
{
  Q_OBJECT
public:
  ToolbarActions(QObject* p);
  ~ToolbarActions();

private slots:

  // Description:
  // Creates a view of the type stored with the action.
  // Then attempts to add the currently selected pipeline source
  // to the it.
  void createView();

  // Description:
  // Creates a source of the type stored with the action.
  void createSource();

  // Description:
  // Creates a filter of the type stored with the action.
  void createFilter();

  // Description:
  // Creates a filter of the type stored with the action.
  // Then executes the filter using whatever default property
  // values are given.
  void createAndExecuteFilter();

  // Description:
  // Creates a filter that requires two inputs, a selection and a graph, 
  // and modified the global selection using its output selection.
  void createSelectionFilter();

  // Description:
  // Load server manager state defined in the file name stored in the sending
  // QAction's data().
  void loadState();

protected:
  void createView(const QString &type);
  void createSource(const QString &type);
  void createFilter(const QString &type);
  void createAndExecuteFilter(const QString &type);
  void createSelectionFilter(const QString &type);

  pqPipelineSource *getActiveSource() const;

};
#endif

