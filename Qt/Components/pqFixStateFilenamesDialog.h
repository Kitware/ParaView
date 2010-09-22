/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef __pqFixStateFilenamesDialog_h
#define __pqFixStateFilenamesDialog_h

#include <QDialog>
#include "pqComponentsExport.h"

class vtkPVXMLElement;
class vtkFileSequenceParser;

/// pqFixStateFilenamesDialog can be used to prompt the user with a dialog to
/// edit the filenames referred to in a state xml. Set the state xml
/// root-element and then call exec() on the dialog. When exec() returns with
/// QDialog::Accepted, it will have modified that XML root element with user
/// specified filenames where ever applicable.
class PQCOMPONENTS_EXPORT pqFixStateFilenamesDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;
public:
  /// \c xml is the state xml root-element. Note that the xml may get modified, if
  /// the user picks different filesnames.
  pqFixStateFilenamesDialog(vtkPVXMLElement* xml,
    QWidget* parent=0, Qt::WindowFlags f=0);
  virtual ~pqFixStateFilenamesDialog();

  /// Call this method to check if the dialog needs to be shown at all.
  bool hasFileNames() const;

  /// Provides access to the xml root.
  vtkPVXMLElement* xmlRoot() const;

  /// Overridden to update xml tree based on user chosen filenames.
  virtual void accept();

protected:

  /// Detect File paterns, constructing the filename to be shown in the pipeline browser.
  QString ConstructPipelineName(QStringList files);

private slots:
  void onFileNamesChanged();

private:
  Q_DISABLE_COPY(pqFixStateFilenamesDialog)

  class pqInternals;
  pqInternals* Internals;
  vtkFileSequenceParser * SequenceParser;
};

#endif
