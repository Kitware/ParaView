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
// .NAME vtkString - performs common string operations
// .SECTION Description
// vtkString is a collection of methods to perform common string operations. 
// The strings in this case are not some reference counted objects but
// the traditional c-strings (char*). This class provides platform 
// independent methods for creating, copying, ... of strings.

#ifndef __vtkString_h
#define __vtkString_h

#include "vtkObject.h"

class VTK_EXPORT vtkString : public vtkObject
{
public:
  static vtkString *New();
  vtkTypeRevisionMacro(vtkString,vtkObject);

  // Description:
  // This method returns the size of string. If the string is empty,
  // it returns 0. It can handle null pointers.
  static vtkIdType Length(const char* str);

  // Description:
  // Copy string to the other string.
  static void Copy(char* dest, const char* src);

  // Description:
  // This method makes a duplicate of the string similar to C function
  // strdup but it uses new to create new string, so you can use
  // delete to remove it. It returns 0 if the input is empty.
  static char* Duplicate(const char* str);

  // Description: 
  // This method compare two strings. It is similar to strcmp, but it
  // can handle null pointers.
  static int Compare(const char* str1, const char* str2);

  // Description:
  // This method compare two strings. It is similar to Compare, but it
  // ignores case.
  static int CompareCase(const char* str1, const char* str2);

  // Description:
  // This method compare two strings. It is similar to strcmp, but it
  // can handle null pointers. Also it only returns C style true or
  // false versus compare which returns also which one is greater.
  static int Equals(const char* str1, const char* str2)
    { return vtkString::Compare(str1, str2) == 0; }

  // Description:
  // This method compare two strings. It is similar to Equals, but it
  // ignores case.
  static int EqualsCase(const char* str1, const char* str2)
    { return vtkString::CompareCase(str1, str2) == 0; }

  // Description:
  // Check if the first string starts with the second one.
  static int StartsWith(const char* str1, const char* str2);

  // Description:
  // Check if the first string ends with the second one.
  static int EndsWith(const char* str1, const char* str2);

  // Description:
  // Returns a pointer to the last occurence of str2 in str1.
  static const char* FindLastString(const char* str1, const char* str2);

  // Description:
  // Append two strings and prroduce new one.  Programmer must delete
  // the resulting string. The method returns 0 if inputs are empty or
  // if there was an error.
  static char* Append(const char* str1, const char* str2);
  static char* Append(const char* str1, const char* str2, const char* str3);
  
  // Description:
  // Transform the string to lowercase (inplace).
  // Return a pointer to the string (i.e. 'str').
  static char* ToLower(char* str);

  // Description:
  // Transform the string to uppercase (inplace).
  // Return a pointer to the string (i.e. 'str').
  static char* ToUpper(char* str);

  // Description:
  // Replace a character or some characters in the string (inplace).
  // Return a pointer to the string (i.e. 'str').
  static char* ReplaceChar(char* str, char toreplace, char replacement);
  static char* ReplaceChars(char* str, const char *toreplace,char replacement);

  // Description:
  // Remove some characters from a string.
  // Return a pointer to a new allocated string.
  static char* RemoveChars(const char* str, const char *toremove);

  // Description:
  // Return the number of occurence of a char.
  static unsigned int CountChar(const char* str, char c);

  // Description:
  // Fill 'str' with 'len' times 'c', add NULL terminator
  // Return 'str'.
  static char* FillString(char* str, char c, size_t len);

  // Description:
  // Crop string to a given length by removing chars in the center of the
  // string and replacing them with an ellipsis (...). This is done in-place.
  static char* CropString(char* str, size_t max_len);

protected:
  vtkString() {};
  ~vtkString() {};

private:
  vtkString(const vtkString&);  // Not implemented.
  void operator=(const vtkString&);  // Not implemented.
};

#endif



