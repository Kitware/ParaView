/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "XMLUtils.h"

//*****************************************************************************
int GetRequiredAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue)
{
  return GetAttribute(elem,attName,attValue,false);
}

//*****************************************************************************
int GetOptionalAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue)
{
  return GetAttribute(elem,attName,attValue,true);
}

//*****************************************************************************
int GetAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue,
      bool optional)
{
  *attValue=elem->GetAttribute(attName);
  if (*attValue==NULL)
    {
    if (!optional)
      {
      sqErrorMacro(pCerr(),"No attribute named " << attName);
      return -1;
      }
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

//*****************************************************************************
std::istream &Delim(std::istream &s,char c)
{
    char w=(char)s.peek();
    while (s && (w=((char)s.peek())) && (((char)s.peek())==c))
    {
        s.get();
    }
    return s;
}
