/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUtilities.cxx
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
#include "vtkXMLUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLUtilities);
vtkCxxRevisionMacro(vtkXMLUtilities, "1.1");

//----------------------------------------------------------------------------
void vtkXMLUtilities::ConvertSpecialChars(const char *str, ostream &os)
{
  if (!str)
    {
    return;
    }

  while (*str)
    {
    switch (*str)
      {
      case '&':
        os << "&amp;";
        break;

      case '"':
        os << "&quot;";
        break;

      case '\'':
        os << "&apos;";
        break;

      case '<':
        os << "&lt;";
        break;

      case '>':
        os << "&gt;";
        break;

      case '\n':
        os << "&#xA;";
        break;

      default:
        os << *str;
      }
    str++;
    }
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::CollateAttributes(vtkXMLDataElement *elem, 
                                        ostream &os,
                                        const char *sep)
{
  if (!elem)
    {
    return;
    }

  int i, nb = elem->GetNumberOfAttributes();
  for (i = 0; i < nb; i++)
    {
    const char *name = elem->GetAttributeName(i);
    if (name)
      {
      const char *value = elem->GetAttribute(name);
      if (value)
        {
        if (i)
          {
          os << (sep ? sep : " ");
          }
        os << name << "=\"";
        vtkXMLUtilities::ConvertSpecialChars(value, os);
        os << '\"';
        }
      }
    }

  return;
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::FlattenElement(vtkXMLDataElement *elem, 
                                     ostream &os,
                                     vtkIndent *indent)
{
  if (!elem)
    {
    return;
    }

  unsigned long pos = os.tellp();

  // Name

  if (indent)
    {
    os << *indent;
    }

  os << '<' << elem->GetName();

  // Attributes

  if (elem->GetNumberOfAttributes())
    {
    os << ' ';
    unsigned long len = (unsigned long)os.tellp() - pos;
    char *sep = new char [1 + len + 1];
    sep[0] = '\n';
    vtkString::FillString(sep + 1, ' ', len);
    vtkXMLUtilities::CollateAttributes(elem, os, sep);
    delete [] sep;
    }

  // Nested elements and close

  int nb_nested = elem->GetNumberOfNestedElements();
  if (!nb_nested)
    {
    os << "/>\n";
    }
  else
    {
    os << ">\n";
    for (int i = 0; i < nb_nested; i++)
      {
      if (indent)
        {
        vtkIndent next_indent = indent->GetNextIndent();
        vtkXMLUtilities::FlattenElement(elem->GetNestedElement(i), 
                                        os, &next_indent);
        }
      else
        {
        vtkXMLUtilities::FlattenElement(elem->GetNestedElement(i), os);
        }
      }
    if (indent)
      {
      os << *indent;
      }
    os << "</" << elem->GetName() << ">\n";
    }
}
