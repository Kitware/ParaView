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
// .NAME vtkXMLObjectReader - XML Object Reader.
// .SECTION Description
// vtkXMLObjectReader provides base functionalities for all XML readers.
// .SECTION See Also
// vtkXMLObjectWriter

#ifndef __vtkXMLObjectReader_h
#define __vtkXMLObjectReader_h

#include "vtkXMLIOBase.h"

class vtkXMLDataElement;
class vtkXMLDataParser;

class VTK_EXPORT vtkXMLObjectReader : public vtkXMLIOBase
{
public:
  vtkTypeRevisionMacro(vtkXMLObjectReader,vtkXMLIOBase);

  // Description:
  // Parse an XML tree and modify the object accordingly.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Parse an XML stream.
  // Return 1 on success, 0 on error.
  virtual int ParseStream(istream&);

  // Description:
  // Parse an XML string.
  // Return 1 on success, 0 on error.
  virtual int ParseString(const char*);

  // Description:
  // Parse an XML file.
  // Return 1 on success, 0 on error.
  virtual int ParseFile(const char*);

  // Description:
  // Convenience method to look in 'parent' for an XML element matching 
  // the current root name and parse that element accordingly (see Parse()).
  // IsInElement() does not actually parse, but checks if the
  // element can be found on the path described previously.
  // Return 1 on success, 0 on error.
  virtual int ParseInElement(vtkXMLDataElement *parent);
  virtual int IsInElement(vtkXMLDataElement *parent);

  // Description:
  // Convenience method to look in 'grandparent' for an XML 'parent' element
  // matching the 'name', then look inside 'parent' for an XML element matching
  // the current root name and parse that element accordingly 
  // (see ParseInElement()).
  // IsInNestedElement() does not actually parse, but checks if the
  // element can be found on the path described previously.
  // Return 1 on success, 0 on error.
  virtual int ParseInNestedElement(vtkXMLDataElement *grandparent,
                                   const char *name);
  virtual int IsInNestedElement(vtkXMLDataElement *grandparent,
                                const char *name);

protected:
  vtkXMLObjectReader();
  ~vtkXMLObjectReader();  

  // Description:
  // Create and destroy the XML parser.
  virtual void CreateXMLParser();
  virtual void DestroyXMLParser();
  
  // Description:
  // InitializeParsing is called before each call to Parse(element).
  virtual int InitializeParsing();

  // A vtkXMLDataParser instance used to hide XML reading details
  // and create an XML tree from an XML stream.

  vtkXMLDataParser* XMLParser;

  // Store the element that was parsed by Parse()

  vtkXMLDataElement *LastParsedElement;

private:
  vtkXMLObjectReader(const vtkXMLObjectReader&);  // Not implemented.
  void operator=(const vtkXMLObjectReader&);  // Not implemented.
};

#endif


