
// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSierraPlotToolsUtils.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef pqSierraPlotToolsUtils_h
#define pqSierraPlotToolsUtils_h

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

//=============================================================================
class pqSierraPlotToolsUtils
{

public:
  pqSierraPlotToolsUtils();
  ~pqSierraPlotToolsUtils();

  QString removeAllWhiteSpace(const QString& inString);

  bool validChar(char inChar);

  int getNumber(int begIndex, int endIndex, QString lineEditText);

protected:
};

#endif // pqSierraPlotToolsUtils_h
