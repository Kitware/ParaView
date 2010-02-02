/*=========================================================================

   Program: ParaView
   Module:    AnnotationManagerPanel.h

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

#ifndef _AnnotationManagerPanel_h
#define _AnnotationManagerPanel_h

#include "OverViewCoreExport.h"
#include <QWidget>

class QModelIndex;
class AnnotationManagerPanelCommand;
class vtkSMSourceProxy;

/// AnnotationManagerPanel is a panel that shows shows the active annotations.
/// It makes is possible for the user to view/change the active annotation.
class OVERVIEW_CORE_EXPORT AnnotationManagerPanel :
  public QWidget
{
  Q_OBJECT
  
public:
  AnnotationManagerPanel(QWidget* parent);
  ~AnnotationManagerPanel();

  void setAnnotationLink(vtkSMSourceProxy*);
  vtkSMSourceProxy* getAnnotationLink();

  int getNumberOfAnnotations();
  int getRowFromName(QString myName);
  //turn on (or off) the annotation referred to by the given row
  void activateAnnotation(int row, bool onOrOff);

public slots:
  void activateItem(const QModelIndex &index);
  void annotationsChanged();
  void modelChanged();

private slots:
  void onUpButtonPressed();
  void onDownButtonPressed();
  void onSelectButtonPressed();
  void onDeleteButtonPressed();

private:
  struct pqImplementation;
  pqImplementation* const Implementation;

  friend class AnnotationManagerPanelCommand;
  AnnotationManagerPanelCommand* Command;
};

#endif
