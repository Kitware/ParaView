// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSierraPlotToolsUtils.cxx

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

#include "warningState.h"

#include "pqSierraPlotToolsUtils.h"

// used to show line number in #pragma messages
#define STRING2(x) #x
#define STRING(x) STRING2(x)

///////////////////////////////////////////////////////////////////////////////
pqSierraPlotToolsUtils::pqSierraPlotToolsUtils()
{
}

///////////////////////////////////////////////////////////////////////////////
pqSierraPlotToolsUtils::~pqSierraPlotToolsUtils()
{
}

///////////////////////////////////////////////////////////////////////////////
QString pqSierraPlotToolsUtils::removeAllWhiteSpace(const QString& inString)
{
  QString retString;

  int i = 0;
  for (i = 0; i < inString.length(); i++)
  {
    if (!QChar(inString[i]).isSpace())
    {
      retString.append(inString[i]);
    }
  }

  return retString;
}

///////////////////////////////////////////////////////////////////////////////
bool pqSierraPlotToolsUtils::validChar(char inChar)
{
  if (inChar == ',' || inChar == '-')
    return true;
  if (inChar < '0')
    return false;
  if (inChar > '9')
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////////////
int pqSierraPlotToolsUtils::getNumber(int begIndex, int endIndex, QString lineEditText)
{
  int retVal = -1;

  QString numStr = lineEditText.midRef(begIndex, endIndex - begIndex + 1).toString();

  int attemptRetVal;
  bool ok;
  attemptRetVal = numStr.toInt(&ok);
  if (ok)
  {
    retVal = attemptRetVal;
  }

  return retVal;
}
