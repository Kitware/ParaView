/*=========================================================================

  Module:    vtkString.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkString.h"
#include "vtkObjectFactory.h"

#include <ctype.h>
#include <time.h>

#if defined( _WIN32 ) && !defined(__CYGWIN__)
#  if defined(__BORLANDC__)
#    define STRCASECMP stricmp
#  else
#    define STRCASECMP _stricmp
#  endif
#else
#  define STRCASECMP strcasecmp
#endif

vtkCxxRevisionMacro(vtkString, "1.26");
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
char* vtkString::ToLowerFirst(char *str)
{
  if (str)
    {
    char *ptr = str;
    while (*ptr)
      {
      if (isalpha(*ptr) && (ptr == str || isspace(*(ptr - 1))))
        {
        *ptr = (char)tolower(*ptr);
        }
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
char* vtkString::ToUpperFirst(char *str)
{
  if (str)
    {
    char *ptr = str;
    while (*ptr)
      {
      if (isalpha(*ptr) && (ptr == str || isspace(*(ptr - 1))))
        {
        *ptr = (char)toupper(*ptr);
        }
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
unsigned int vtkString::CountChar(const char* str, char c)
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
  char* src = str + str_len - (max_len - middle);
  char* dst = str + middle;
  size_t len = strlen(src);
  size_t cc;
  for ( cc = 0; cc < len; cc ++ )
    {
    dst[cc] = src[cc];
    }
  dst[cc] = 0;

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

//----------------------------------------------------------------------------
int vtkStringConvertDateMacroString(char *str, struct tm *tmt)
{
  if (!str || !tmt)
    {
    return 0;
    }

  // __DATE__
  // The compilation date of the current source file. The date is a string
  // literal of the form Mmm dd yyyy. The month name Mmm is the same as for
  // dates generated by the library function asctime declared in TIME.H. 

  // index:   012345678901
  // format:  Mmm dd yyyy
  // example: Dec 19 2003

  char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

  char buffer[12];
  strcpy(buffer, str);

  buffer[3] = 0;
  char *ptr = strstr(month_names, buffer);
  if (!ptr)
    {
    return 0;
    }

  int month = (ptr - month_names) / 3;
  int day = atoi(buffer + 4);
  int year = atoi(buffer + 7);

  tmt->tm_isdst = -1;
  tmt->tm_hour  = 0;
  tmt->tm_min   = 0;
  tmt->tm_sec   = 0;
  tmt->tm_wday  = 0;
  tmt->tm_yday  = 0;
  tmt->tm_mday  = day;
  tmt->tm_mon   = month;
  tmt->tm_year  = year - 1900;

  return 1;
}

//----------------------------------------------------------------------------
int vtkString::ConvertDateMacroString(char *str, time_t *tmt)
{
  struct tm tmt2;
  if (!vtkStringConvertDateMacroString(str, &tmt2))
    {
    return 0;
    }
  *tmt = mktime(&tmt2);
  return 1;
}

//----------------------------------------------------------------------------
int vtkStringConvertTimeStampMacroString(char *str, struct tm *tmt)
{
  if (!str || !tmt)
    {
    return 0;
    }

  // __TIMESTAMP__
  // The date and time of the last modification of the current source file, 
  // expressed as a string literal in the form Ddd Mmm Date hh:mm:ss yyyy, 
  /// where Ddd is the abbreviated day of the week and Date is an integer 
  // from 1 to 31.

  // index:   0123456789
  //                    0123456789
  //                              0123456789
  // format:  Ddd Mmm Date hh:mm:ss yyyy
  // example: Fri Dec 19 14:34:58 2003

  char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

  char buffer[27];
  strcpy(buffer, str);

  buffer[7] = 0;
  char *ptr = strstr(month_names, buffer + 4);
  if (!ptr)
    {
    return 0;
    }

  int month = (ptr - month_names) / 3;
  int day = atoi(buffer + 8);
  int hour = atoi(buffer + 11);
  int min = atoi(buffer + 14);
  int sec = atoi(buffer + 17);
  int year = atoi(buffer + 20);

  tmt->tm_isdst = -1;
  tmt->tm_hour  = hour;
  tmt->tm_min   = min;
  tmt->tm_sec   = sec;
  tmt->tm_wday  = 0;
  tmt->tm_yday  = 0;
  tmt->tm_mday  = day;
  tmt->tm_mon   = month;
  tmt->tm_year  = year - 1900;

  return 1;
}

//----------------------------------------------------------------------------
int vtkString::ConvertTimeStampMacroString(char *str, time_t *tmt)
{
  struct tm tmt2;
  if (!vtkStringConvertTimeStampMacroString(str, &tmt2))
    {
    return 0;
    }
  *tmt = mktime(&tmt2);
  return 1;
}

//----------------------------------------------------------------------------
char* vtkString::AddSpaceToUpperFirstString(const char *str, char *res)
{
  if (str && *str && res)
    {
    char *ptr = res;
    *ptr++ = *str++;
    while (*str)
      {
      if (isupper(*str) && !isupper(*(str - 1)))
        {
        *ptr++ = ' ';
        }
      *ptr++ = *str++;
      }
    *ptr = '\0';
    }

  return res;
}

