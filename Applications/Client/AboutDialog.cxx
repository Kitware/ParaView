/*=========================================================================

   Program: ParaView
   Module:    $RCS $

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

#include "AboutDialog.h"

#include "ui_AboutDialog.h"

#include "pqOptions.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"

AboutDialog::AboutDialog(QWidget* Parent) :
  QDialog(Parent),
  Ui(new Ui::AboutDialog())
{
  this->Ui->setupUi(this);
  this->setObjectName("AboutDialog");

  // get extra information and put it in
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pqOptions* opts = pqOptions::SafeDownCast(pm->GetOptions());

  ostrstream str;
  vtkIndent indent;
  opts->PrintSelf(str, indent.GetNextIndent());
  str << ends;
  QString info = str.str();
  str.rdbuf()->freeze(0);
  int idx = info.indexOf("Runtime information:");
  info = info.remove(0, idx);
  this->Ui->Information->append("ParaView was developed by Kitware Inc.");
  this->Ui->Information->append("<a href=\"http://www.paraview.org\">www.paraview.org</a>");
  this->Ui->Information->append("<a href=\"http://www.kitware.com\">www.kitware.com</a>");
  QString version = QString("This is version %1").arg(PARAVIEW_VERSION_FULL);
  this->Ui->Information->append(version);
  
  // For now, don't add any runtime information, it's 
  // incorrect for PV3 (made sense of PV2).
  // this->Ui->Information->append("\n");
  // this->Ui->Information->append(info);
  this->Ui->Information->moveCursor(QTextCursor::Start);
  this->Ui->Information->viewport()->setBackgroundRole(QPalette::Window);
}

AboutDialog::~AboutDialog()
{
  delete this->Ui;
}
