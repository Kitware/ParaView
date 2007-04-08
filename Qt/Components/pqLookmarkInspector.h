/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkInspector.h

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

/// \file pqLookmarkInspector.h
/// \brief
///   The pqLookmarkInspector class is used to display the properties
///   of lookmarks in an editable form.
///
/// \date 11/25/2005

#ifndef _pqLookmarkInspector_h
#define _pqLookmarkInspector_h

#include "pqComponentsExport.h"
#include <QWidget>

class QModelIndex;
class QItemSelection;
class pqLookmarkInspectorForm;
class pqLookmarkModel;
class vtkPVXMLElement;
class QStandardItem;
//class QStandardItemModel;
class pqLookmarkManagerModel;

/// \class pqLookmarkInspector
/// \brief
///   The pqLookmarkInspector class is used to display the properties
///   of lookmarks in an editable form.
class PQCOMPONENTS_EXPORT pqLookmarkInspector : public QWidget
{
  Q_OBJECT
public:
  pqLookmarkInspector(pqLookmarkManagerModel *model, QWidget *parent=0);
  virtual ~pqLookmarkInspector();

  /// hint for sizing this widget
  virtual QSize sizeHint() const;

public slots:

  // Called when the selection has changed in the lookmark browser
  // Which widgets are displayed is based on whether there are 0, 1, or more lookmarks currently selected
  // selected: names of selected lookmarks
  void onLookmarkSelectionChanged(const QStringList &selected);

  // This modifies/deletes the the data at the selected indices of the pqLookmarkBrowserModel
  void save();
  void remove();

  // Invoke the lookmark in the current view
  void load();

  void onModified();

signals:
  void saved(pqLookmarkModel*);
  void modified();
  void loadLookmark(const QString &name);
  void removeLookmark(const QString &name);

protected:
  void generatePipelineView();
  void addChildItems(vtkPVXMLElement *elem, QStandardItem *item);
  
private:

  // The current selection in the lookmark browser
  QList<QString> SelectedLookmarks;
  pqLookmarkModel *CurrentLookmark;
  pqLookmarkManagerModel *Model;
  //QStandardItemModel *PipelineModel;
  pqLookmarkInspectorForm *Form;   ///< Defines the gui layout.
};

#endif
