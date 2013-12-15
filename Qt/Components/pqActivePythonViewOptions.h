/*=========================================================================

  Program:   ParaView
  Module:    pqActivePythonViewOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef _pqActivePythonViewOptions_h
#define _pqActivePythonViewOptions_h


#include "pqActiveViewOptions.h"


/// \class pqActivePythonViewOptions
/// \brief
///   The pqActivePythonViewOptions class is used to display an
///   options dialog for the Python render view.
class PQCOMPONENTS_EXPORT pqActivePythonViewOptions :
    public pqActiveViewOptions
{
  Q_OBJECT

public:
  pqActivePythonViewOptions(QObject *parent=0);
  virtual ~pqActivePythonViewOptions();

  /// \name pqActiveViewOptions Methods
  //@{
  virtual void showOptions(pqView *view, const QString &page, QWidget *parent=0);
  virtual void changeView(pqView *view);
  virtual void closeOptions();
  //@}

protected slots:
  void finishDialog();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
