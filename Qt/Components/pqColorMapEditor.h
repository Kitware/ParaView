/*=========================================================================

   Program: ParaView
   Module:    pqColorMapEditor.h

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

/// \file pqColorMapEditor.h
/// \date 7/31/2006

#ifndef _pqColorMapEditor_h
#define _pqColorMapEditor_h


#include "pqComponentsExport.h"
#include <QDialog>

class pqColorMapEditorForm;
class pqColorTableModel;
class pqPipelineDisplay;
class QCloseEvent;
class QColor;
class QModelIndex;
class QString;
class QTimer;
class vtkSMProxy;


class PQCOMPONENTS_EXPORT pqColorMapEditor : public QDialog
{
  Q_OBJECT

public:
  pqColorMapEditor(QWidget *parent=0);
  virtual ~pqColorMapEditor();

  void setDisplay(pqPipelineDisplay *display);
  int getTableSize() const;

protected:
  virtual void closeEvent(QCloseEvent *e);

  // Updates the elements in the editor based on the type of the 
  // lookup table.
  void updateEditor();

  void resetGUI();

private slots:
  void setUseDiscreteColors(bool on);
  void setUsingGradient(bool on);
  void handleTextEdit(const QString &text);
  void setSizeFromText();
  void setSizeFromSlider(int tableSize);
  void setTableSize(int tableSize);
  void changeControlColor(int index, const QColor &color);
  void getTableColor(const QModelIndex &index);
  void changeTableColor(int index, const QColor &color);
  void updateTableRange(int first, int last);
  void closeForm();

private:
  pqColorMapEditorForm *Form;
  pqColorTableModel *Model;
  vtkSMProxy *LookupTable;
  QTimer *EditDelay;

  void resetFromPVLookupTable();
  void resetFromLookupTable();
};

#endif

