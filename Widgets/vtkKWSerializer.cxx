/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSerializer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWSerializer.h"
#include <ctype.h>
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"


//------------------------------------------------------------------------------
vtkKWSerializer* vtkKWSerializer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWSerializer");
  if(ret)
    {
    return (vtkKWSerializer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWSerializer;
}




// Internal function used to consume whitespace when reading in
// an InputString.
void vtkKWSerializer::EatWhiteSpace(istream *is)
{
  char c;
  while (is->get(c)) 
    {
    if (isspace(c)==0)
      {
      is->putback(c);
      break;
      }
    }
}

int vtkKWSerializer::GetNextToken(istream *is, char result[1024])
{
  int success;
  success = 0;
  int count = 0;
  
  vtkKWSerializer::EatWhiteSpace(is);  
  char c;
  while (is->get(c)) 
    {
    if (c == '\n' || isspace(c)!= 0)
      {
      is->putback(c);
      break;
      }
    else if (c == '"' && !count)
      {
      // reading a quoted string, return entire quoted string as a token
      while (is->get(c)) 
        {
        if (c == '"')
          {
          break;
          }
        if (c == '\\')
          {
          if (is->get(c) && c != '"')
            {
            result[count] = '\\';
            count++;
            }
          result[count] = c;
          count++;
          }
	      else
					{
					result[count] = c;
          count++;
					}
        if (count >= 1024)
          {
          result[count] ='\0';
          vtkGenericWarningMacro("A token exceeding the maximum token size was found! The token was: " << result);
          }
        }
      success = 1;
      break;
      }
    else if ((c == '{' || c == '}') && count)
      {
      is->putback(c);
      break;
      }
    else if (c == '{' || c == '}')
      {
      success = 1;
      result[0] = c;
      count = 1;
      break;
      }
    else
      {
      success = 1;
      result[count] = c;
      count++;
      if (count == 1024)
        {
        result[count] ='\0';
        vtkGenericWarningMacro("A token exceeding the maximum token size was found! The token was: " << result);
        }
      }
    }
  
  result[count] = '\0';
  return success;
}

void vtkKWSerializer::FindClosingBrace(istream *is, vtkObject *obj)
{
  int count = 0;
  char token[1024];
  int balance = 1;
  int res;
  
  while (balance && vtkKWSerializer::GetNextToken(is,token))
    {
    res = vtkKWSerializer::GetNextToken(is,token);
    if (token[0] == '{')
      {
      balance++;
      }
    if (token[0] == '}')
      {
      balance--;
      }
    }

  if (balance)
    {
    vtkGenericWarningMacro("Error trying to match open brace for object " << obj->GetClassName() );
    }
}

void vtkKWSerializer::ReadNextToken(istream *is, const char *tok,
                                    vtkObject *obj)
{
  char result[1024];
  if (!vtkKWSerializer::GetNextToken(is,result))
    {
    vtkGenericWarningMacro("Error trying to find token " << tok << " for object " << obj->GetClassName() );
    }
  if (strcmp(tok,result))
    {
    vtkGenericWarningMacro("Error trying to find token " << tok << " for object " << obj->GetClassName() << " found token " << result << "instead");
    }
}

void vtkKWSerializer::WriteSafeString(ostream& os, const char *val)
{
  int i;
  int len = strlen(val);
  
  os << '"';
  for (i = 0; i < len; i++)
    {
    if (val[i] == '"')
      {
      os << '\\';
      }
    os << val[i];
    }
  os << '"';
}

  
