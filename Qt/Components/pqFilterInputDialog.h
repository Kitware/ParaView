/*=========================================================================

   Program: ParaView
   Module:    pqFilterInputDialog.h

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
class pqPipelineFilter;
class pqPipelineModel;
class QButtonGroup;
class QGroupBox;
class QItemSelection;
class QLabel;
class QPushButton;
class QScrollArea;
class QString;


/// \class pqFilterInputDialog
/// \brief
///   The pqFilterInputDialog class is used to change or set up the
///   inputs for a filter.
class PQCOMPONENTS_EXPORT pqFilterInputDialog : public QDialog
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a filter input dialog.
  /// \param parent The parent widget.
  pqFilterInputDialog(QWidget *parent=0);
  virtual ~pqFilterInputDialog();

  /// \brief
  ///   Gets the pipeline model.
  /// \return
  ///   A pointer to the pipeline model.
  pqPipelineModel *getModel() const {return this->Model;}

  /// \brief
  ///   Gets the pipeline filter.
  /// \return
  ///   A pointer to the pipeline filter.
  pqPipelineFilter *getFilter() const {return this->Filter;}

  /// \brief
  ///   Sets up the input filter dialog.
  /// \param model The pipeline model to use.
  /// \param filter The filter to modify.
  /// \param namedInputs A map of the filter inputs. This is used
  ///   instead of the current filter inputs, which is usefull when
  ///   the filter hasn't actually been added to the pipeline.
  void setModelAndFilter(pqPipelineModel *model, pqPipelineFilter *filter,
      const QMap<QString, QList<pqOutputPort*> > &namedInputs);

  /// \brief
  ///   Gets the list of output ports connected to the given input
  ///   port.
  /// \param port The name of the input port.
  /// \return
  ///   A list of the output ports connected to the given input port.
  QList<pqOutputPort *> getFilterInputs(const QString &port) const;

private slots:
  /// \brief
  ///   Updates the dialog when the user changes the current input
  ///   port.
  /// \param id The input port radio button identifier.
  void changeCurrentInput(int id);

  /// \brief
  ///   Changes the filter input to match the user's selection.
  /// \param selected The newly selected sources.
  /// \param deselected The sources being deselected.
  void changeInput(const QItemSelection &selected,
      const QItemSelection &deselected);

private:
  pqFilterInputDialogInternal *Internal;
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
