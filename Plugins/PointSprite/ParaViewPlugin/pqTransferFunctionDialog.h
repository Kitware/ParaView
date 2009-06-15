/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqTransferFunctionDialog.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqTransferFunctionDialog
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#ifndef __pqTransferFunctionDialog_h
#define __pqTransferFunctionDialog_h

#include <QDialog>

class pqTransferFunctionEditor;
class pqPipelineRepresentation;

class pqTransferFunctionDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;
public:
  pqTransferFunctionDialog(QWidget* parent);
  ~pqTransferFunctionDialog();

  pqTransferFunctionEditor* opacityEditor();
  pqTransferFunctionEditor* radiusEditor();

  void  setRepresentation(pqPipelineRepresentation* repr);

  void  show(pqTransferFunctionEditor* editor);

private:
  class pqInternals;
  pqInternals* Internals;

  pqTransferFunctionDialog(const pqTransferFunctionDialog&); // Not implemented.
  void operator=(const pqTransferFunctionDialog&); // Not implemented.
};

#endif


