/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkString.h"
#include "vtkObjectFactory.h"
#include <ctype.h>

#if defined( _WIN32 ) && !defined(__CYGWIN__)
#  if defined(__BORLANDC__)
#    define STRCASECMP stricmp
#  else
#    define STRCASECMP _stricmp
#  endif
#else
#  define STRCASECMP strcasecmp
#endif

vtkCxxRevisionMacro(vtkString, "1.18");
vtkStandardNewMacro(vtkString);
 
//----------------------------------------------------------------------------
vtkIdType vtkString::Length(const char* str)
{
  if ( !str )
    {
    return 0;
    }
  return static_cast<vtkIdType>(strlen(str));
}

  
//----------------------------------------------------------------------------
void vtkString::Copy(char* dest, const char* src)
{
  if ( !dest )
    {
    return;
    }
  if ( !src )
    {
    *dest = 0;
    return;
    }
  strcpy(dest, src);
}

//----------------------------------------------------------------------------
char* vtkString::Duplicate(const char* str)
{    
  if ( str )
    {
    char *newstr = new char [ vtkString::Length(str) + 1 ];
    vtkString::Copy(newstr, str);
    return newstr;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkString::Compare(const char* str1, const char* str2)
{
  if ( !str1 )
    {
    return -1;
    }
  if ( !str2 )
    {
    return 1;
    }
  return strcmp(str1, str2);
}

//----------------------------------------------------------------------------
int vtkString::StartsWith(const char* str1, const char* str2)
{
  if ( !str1 || !str2 || strlen(str1) < strlen(str2) )
    {
    return 0;
    }
  return !strncmp(str1, str2, strlen(str2));  
}

//----------------------------------------------------------------------------
int vtkString::EndsWith(const char* str1, const char* str2)
{
  if ( !str1 || !str2 || strlen(str1) < strlen(str2) )
    {
    return 0;
    }
  return !strncmp(str1 + (strlen(str1)-strlen(str2)), str2, strlen(str2));
}

//----------------------------------------------------------------------------
const char* vtkString::FindLastString(const char* str1, const char* str2)
{
  if (!str1 || !str2)
    {
    return NULL;
    }
  
  size_t len1 = strlen(str1);
  size_t len2 = strlen(str2);

  if (len1 < len2)
    {
    return NULL;
    }

  const char *ptr = str1 + len1 - len2;
  do
    {
    if (!strncmp(ptr, str2, len2))
      {
      return ptr;
      }
    } while (ptr-- != str1);

  return NULL;
}

//----------------------------------------------------------------------------
char* vtkString::Append(const char* str1, const char* str2)
{
  if ( !str1 && !str2 )
    {
    return 0;
    }
  char *newstr = 
    new char[ vtkString::Length(str1) + vtkString::Length(str2)+1];
  if ( !newstr )
    {
    return 0;
    }
  newstr[0] = 0;
  if ( str1 )
    {
    strcat(newstr, str1);
    }
  if ( str2 )
    {
    strcat(newstr, str2);
    }
  return newstr;
}

//----------------------------------------------------------------------------
char* vtkString::Append(const char* str1, const char* str2, const char* str3)
{
  if ( !str1 && !str2 && !str3 )
    {
    return 0;
    }
  char *newstr = 
    new char[ vtkString::Length(str1) + vtkString::Length(str2) + vtkString::Length(str3)+1];
  if ( !newstr )
    {
    return 0;
    }
  newstr[0] = 0;
  if ( str1 )
    {
    strcat(newstr, str1);
    }
  if ( str2 )
    {
    strcat(newstr, str2);
    }
  if ( str3 )
    {
    strcat(newstr, str3);
    }
  return newstr;
}

//----------------------------------------------------------------------------
int vtkString::CompareCase(const char* str1, const char* str2)
{
  if ( !str1 )
    {
    return -1;
    }
  if ( !str2 )
    {
    return 1;
    }
  return STRCASECMP(str1, str2);
}

//----------------------------------------------------------------------------
char* vtkString::ToLower(char *str)
{
  if (str)
    {
    char *ptr = str;
    while (*ptr)
      {
      *ptr = (char)tolower(*ptr);
      ++ptr;
      }
    }
  return str;
}

//----------------------------------------------------------------------------
char* vtkString::ToUpper(char *str)
{
  if (str)
    {
    char *ptr = str;
    while (*ptr)
      {
      *ptr = (char)toupper(*ptr);
      ++ptr;
      }
    }
  return str;
}

//----------------------------------------------------------------------------
char* vtkString::ReplaceChar(char* str, char toreplace, char replacement)
{
  if (str)
    {
    char *ptr = str;
    while (*ptr)
      {
      if (*ptr == toreplace)
        {
        *ptr = replacement;
        }
      ++ptr;
      }
    }
  return str;
}

//----------------------------------------------------------------------------
char* vtkString::ReplaceChars(char* str, const char *toreplace, char replacement)
{
  if (str)
    {
    char *ptr = str;
    while (*ptr)
      {
      const char *ptr2 = toreplace;
      while (*ptr2)
        {
        if (*ptr == *ptr2)
          {
          *ptr = replacement;
          }
        ++ptr2;
        }
      ++ptr;
      }
    }
  return str;
}

//----------------------------------------------------------------------------
char* vtkString::RemoveChars(const char* str, const char *toremove)
{
  if (!str)
    {
    return 0;
    }

  char *clean_str = new char [strlen(str) + 1];
  char *ptr = clean_str;

  while (*str)
    {
    const char *str2 = toremove;
    while (*str2 && *str != *str2)
      {
      ++str2;
      }
    if (!*str2)
      {
      *ptr++ = *str;
      }
    ++str;
    }
  *ptr = '\0';
  return clean_str;
}

//----------------------------------------------------------------------------
char* vtkString::RemoveAllButChars(const char* str, const char *tokeep)
{
  if (!str)
    {
    return 0;
    }

  char *clean_str = new char [strlen(str) + 1];
  char *ptr = clean_str;

  while (*str)
    {
    const char *str2 = tokeep;
    while (*str2 && *str != *str2)
      {
      ++str2;
      }
    if (*str2)
      {
      *ptr++ = *str;
      }
    ++str;
    }
  *ptr = '\0';
  return clean_str;
}

//----------------------------------------------------------------------------
char* vtkString::RemoveAllButUpperHex(const char* str)
{
  if (!str)
    {
    return 0;
    }

  char *clean_str = new char [strlen(str) + 1];
  char *ptr = clean_str;

  while (*str)
    {
    if ((*str >= '0' && *str <= '9') || (*str >= 'A' && *str <= 'H'))
      {
      *ptr++ = *str;
      }
    ++str;
    }
  *ptr = '\0';
  return clean_str;
}

//----------------------------------------------------------------------------
unsigned int vtkString::CountChar(const char* str, const char c)
{
  int count = 0;

  if (str)
    {
    const char *ptr = str;
    while (*ptr)
      {
      if (*ptr == c)
        {
        ++count;
        }
      ++ptr;
      }
    }
  return count;
}

//----------------------------------------------------------------------------
char* vtkString::FillString(char* str, char c, size_t len)
{
  if (str)
    {
    memset(str, c, len);
    str[len] = '\0';
    }

  return str;
}

//----------------------------------------------------------------------------
char* vtkString::CropString(char* str, size_t max_len)
{
  if (!str || max_len == 0)
    {
    return str;
    }

  size_t str_len = strlen(str);
  if (max_len >= str_len)
    {
    return str;
    }

  // Crop the string

  size_t middle = max_len / 2;
  strcpy(str + middle, str + str_len - (max_len - middle));

  // Add the ellipsis

  if (max_len > 2)
    {
    str[middle] = '.';
    if (max_len > 3)
      {
      str[middle - 1] = '.';
      if (max_len > 4)
        {
        str[middle + 1] = '.';
        }
      }
    }

  return str;
}
