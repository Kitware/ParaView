/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqTransferFunctionDialog.cxx

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

#include "pqTransferFunctionDialog.h"
#include "ui_pqTransferFunctionDialog.h"

#include "pqTransferFunctionEditor.h"

class pqTransferFunctionDialog::pqInternals: public Ui::pqTransferFunctionDialog
{

};

pqTransferFunctionDialog::pqTransferFunctionDialog(QWidget* parentObject) : QDialog(parentObject)
{
  this->Internals = new pqTransferFunctionDialog::pqInternals();
  this->Internals->setupUi(this);
  opacityEditor()->configure(pqTransferFunctionEditor::Opacity);
  radiusEditor()->configure(pqTransferFunctionEditor::Radius);
}

pqTransferFunctionDialog::~pqTransferFunctionDialog()
{
}

void  pqTransferFunctionDialog::setRepresentation(pqPipelineRepresentation* repr)
{
  opacityEditor()->setRepresentation(repr);
  radiusEditor()->setRepresentation(repr);
}

void  pqTransferFunctionDialog::show(pqTransferFunctionEditor* editor)
{
  this->Internals->TransferFunctionTabs->setCurrentWidget(editor);
  this->Superclass::show();
}

pqTransferFunctionEditor* pqTransferFunctionDialog::opacityEditor()
{
  return this->Internals->OpacityPage;
}

pqTransferFunctionEditor* pqTransferFunctionDialog::radiusEditor()
{
  return this->Internals->RadiusPage;
}



