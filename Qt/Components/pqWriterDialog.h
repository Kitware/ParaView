/*=========================================================================

   Program: ParaView
   Module:    pqWriterDialog.h

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

/// \file pqWriterDialog.h
/// \brief
///   The pqWriterDialog class is used to display the properties
///   of a writer proxy in an editable form.
///
/// \date 11/25/2005

#ifndef _pqWriterDialog_h
#define _pqWriterDialog_h

#include "pqComponentsExport.h"
#include <QDialog>
#include <vtkSMProxy.h>


/// \class pqWriterDialog
/// \brief
///   The pqWriterDialog class is used to display the properties
///   of a writer proxy in an editable form.
class PQCOMPONENTS_EXPORT pqWriterDialog : public QDialog
{
  Q_OBJECT
public:
  pqWriterDialog(vtkSMProxy *proxy, QWidget *parent=0);
  virtual ~pqWriterDialog();

  /// Return whether or not there are any properties that can be configured.
  /// This could be zero once the properties in the hints are hidden.
  bool hasConfigurableProperties();

  /// hint for sizing this widget
  virtual QSize sizeHint() const;
  
private:
  /// The ok button was pressed.
  /// Accept the changes made to the properties
  /// changes will be propogated down to the server manager
  void accept();

  /// The cancel button was pressed. Unlink the properties and return.
  void reject();

  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
