/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSerializer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWSerializer.h"
#include <ctype.h>
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"


//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSerializer );




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
  char token[1024];
  int balance = 1;
  
  while (balance && vtkKWSerializer::GetNextToken(is,token))
    {
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

  
