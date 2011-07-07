/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "XMLUtils.h"

//*****************************************************************************
int GetRequiredAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue)
{
  *attValue=elem->GetAttribute(attName);
  if (*attValue==NULL)
    {
    sqErrorMacro(pCerr(),"No attribute named " << attName);
    return 1;
    }
  return 0;
}

//*****************************************************************************
vtkPVXMLElement *GetRequiredElement(
      vtkPVXMLElement *root,
      const char *name)
{
  vtkPVXMLElement *elem=root->FindNestedElementByName(name);
  if (elem==0)
    {
    sqErrorMacro(pCerr(),"No element named " << name << ".");
    //exit(SQ_EXIT_ERROR);
    }
  return elem;
}

//*****************************************************************************
vtkPVXMLElement *GetOptionalElement(
      vtkPVXMLElement *root,
      const char *name)
{
  vtkPVXMLElement *elem=root->FindNestedElementByName(name);
  return elem;
}
