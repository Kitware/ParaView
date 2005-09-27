/*=========================================================================

  Module:    vtkKWSerializer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSerializer.h"

#include "vtkObjectFactory.h"

#include <ctype.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSerializer );
vtkCxxRevisionMacro(vtkKWSerializer, "1.8");

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkKWSerializer::GetNextToken(istream *is, char *result)
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
        if (count >= VTK_KWSERIALIZER_MAX_TOKEN_LENGTH)
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
      if (count == VTK_KWSERIALIZER_MAX_TOKEN_LENGTH)
        {
        result[count] ='\0';
        vtkGenericWarningMacro("A token exceeding the maximum token size was found! The token was: " << result);
        }
      }
    }
  
  result[count] = '\0';
  return success;
}

//----------------------------------------------------------------------------
void vtkKWSerializer::FindClosingBrace(istream *is, vtkObject *obj)
{
  char token[VTK_KWSERIALIZER_MAX_TOKEN_LENGTH];
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

//----------------------------------------------------------------------------
void vtkKWSerializer::ReadNextToken(istream *is, const char *tok,
                                    vtkObject *obj)
{
  char result[VTK_KWSERIALIZER_MAX_TOKEN_LENGTH];
  if (!vtkKWSerializer::GetNextToken(is,result))
    {
    vtkGenericWarningMacro("Error trying to find token " << tok << " for object " << obj->GetClassName() );
    }
  if (strcmp(tok,result))
    {
    vtkGenericWarningMacro("Error trying to find token " << tok << " for object " << obj->GetClassName() << " found token " << result << "instead");
    }
}

//----------------------------------------------------------------------------
void vtkKWSerializer::WriteSafeString(ostream& os, const char *val)
{
  int i;
  int len = val ? strlen(val) : 0;
  
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

//----------------------------------------------------------------------------
void vtkKWSerializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}



