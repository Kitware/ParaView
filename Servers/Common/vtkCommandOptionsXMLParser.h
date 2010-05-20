/*=========================================================================
  
  Program:   ParaView
  Module:    vtkCommandOptionsXMLParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCommandOptionsXMLParser - ParaView options storage
// .SECTION Description
// An object of this class represents a storage for ParaView options
// 
// These options can be retrieved during run-time, set using configuration file
// or using Command Line Arguments.

#ifndef __vtkCommandOptionsXMLParser_h
#define __vtkCommandOptionsXMLParser_h

#include "vtkXMLParser.h"
#include "vtkCommandOptions.h" // for enum
class vtkCommandOptionsXMLParserInternal;
class vtkCommandOptions;

class VTK_EXPORT vtkCommandOptionsXMLParser : public vtkXMLParser
{
public:
  static vtkCommandOptionsXMLParser* New();
  vtkTypeMacro(vtkCommandOptionsXMLParser,vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add arguments to the xml parser.  These should be the 
  // long arguments from the vtkCommandOptions class of the form
  // --foo, and pass in the variable that needs to be set with the value.
  void AddBooleanArgument(const char* longarg, int* var, int type=0);
  void AddArgument(const char* longarg, int* var, int type=0);
  void AddArgument(const char* longarg, char** var, int type=0);
  void SetPVOptions(vtkCommandOptions* o) 
    {
      this->PVOptions = o;
    }
protected:
  // Description:
  // Default constructor.
  vtkCommandOptionsXMLParser();

  // Description:
  // Destructor.
  virtual ~vtkCommandOptionsXMLParser();

  // Called when a new element is opened in the XML source.  Should be
  // replaced by subclasses to handle each element.
  //  name = Name of new element.
  //  atts = Null-terminated array of attribute name/value pairs.
  //         Even indices are attribute names, and odd indices are values.
  virtual void StartElement(const char* name, const char** atts);

  // Called at the end of an element in the XML source opened when
  // StartElement was called.
  virtual void EndElement(const char* name);  
  // Call to process the .. of  <Option>...</>
  void HandleOption(const char** atts);
  // Call to process the .. of  <Option>...</>
  void HandleProcessType(const char** atts);

  virtual void SetProcessType(const char* ptype);
  void SetProcessTypeInt(int ptype);

private:
  vtkCommandOptionsXMLParser(const vtkCommandOptionsXMLParser&); // Not implemented
  void operator=(const vtkCommandOptionsXMLParser&); // Not implemented
  int InPVXTag;
  vtkCommandOptions* PVOptions;
  vtkCommandOptionsXMLParserInternal* Internals;
};

#endif // #ifndef __vtkCommandOptionsXMLParser_h

