/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqDoubleEdit.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqDoubleEdit
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

#ifndef __pqDoubleEdit_h__
#define __pqDoubleEdit_h__

#include <QLineEdit>

class pqDoubleEdit : public QLineEdit
{
  Q_OBJECT
  Q_PROPERTY(double value READ value WRITE setValue)

public :
  pqDoubleEdit(QWidget* parent);
  ~pqDoubleEdit();

  double value();
signals :
  void  valueChanged(double);

public slots:
  void  setValue(double);

protected slots :
  void  valueEdited(const QString&);

};
#endif// __pqDoubleEdit_h__
