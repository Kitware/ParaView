/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __XMLUtils_h
#define __XMLUtils_h

#include "vtkPVXMLElement.h"
#include "SQMacros.h"
#include "postream.h"

#include <sstream>
using std::istringstream;

// //*****************************************************************************
// template<typename T>
// T GetRequiredAttribute(vtkPVXMLElement *elem, const char *attName)
// {
//   const char *attValue=elem->GetAttribute(attName);
//   if (attValue==NULL)
//     {
//     sqErrorMacro(pCerr(),"No attribute named " << attName);
//     exit(SQ_EXIT_ERROR);
//     }
//   T val;
//   istringstream is(attValue);
//   is >> val;
//   return val;
// }

/**
In the element elem return the value of attribute attName
in attValue. Return 0 if successful.
*/
int GetRequiredAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue);

/**
In the element elem return the value of attribute attName
in attValue. Return 0 if successful.
*/
template<typename T, int N>
int GetRequiredAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      T *attValue)
{
  const char *attValueStr=elem->GetAttribute(attName);
  if (attValueStr==NULL)
    {
    sqErrorMacro(pCerr(),"No attribute named " << attName << ".");
    return 1;
    }

  T *pAttValue=attValue;
  istringstream is(attValueStr);
  for (int i=0; i<N; ++i)
    {
    if (!is.good())
      {
      sqErrorMacro(pCerr(),"Wrong number of values in " << attName <<".");
      return 1;
      }
    is >> *pAttValue;
    ++pAttValue;
    }
  return 0;
}

/**
Return the element named name in the hierarchy root. Return 0 if
unsuccessful.
*/
vtkPVXMLElement *GetRequiredElement(
      vtkPVXMLElement *root,
      const char *name);

/**
Return the element named name in the hierarchy root. Return 0 if
unsuccessful.
*/
vtkPVXMLElement *GetOptionalElement(
      vtkPVXMLElement *root,
      const char *name);

#endif
