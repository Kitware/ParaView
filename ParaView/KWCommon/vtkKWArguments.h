/*=========================================================================

  Module:    vtkKWArguments.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWArguments - simple wrapper for icons
// .SECTION Description
// A simple icon wrapper. It can either be used with file icons.h to 
// provide a unified interface for internal icons or a wrapper for 
// custom icons. The icons are defined with width, height, pixel_size, and array
// of unsigned char values.

#ifndef __vtkKWArguments_h
#define __vtkKWArguments_h

#include "vtkObject.h"

class vtkKWArgumentsInternal;

class VTK_EXPORT vtkKWArguments : public vtkObject
{
public:
  static vtkKWArguments* New();
  vtkTypeRevisionMacro(vtkKWArguments,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  typedef vtkKWArgumentsInternal Internal;

  //BTX
  // These are prototypes for callbacks.
  typedef int(*CallbackType)(const char* argument, const char* value, 
    void* call_data);
  typedef int(*ErrorCallbackType)(const char* argument, void* client_data);

  struct CallbackStructure
    {
    const char* Argument;
    int ArgumentType;
    CallbackType Callback;
    void* CallData;
    const char* Help;
    };
  
  // Description:
  // These are different argument types.
  enum { 
    NO_ARGUMENT,    // The option takes no argument             --foo
    CONCAT_ARGUMENT,// The option takes argument after no space --foobar
    SPACE_ARGUMENT, // The option takes argument after space    --foo bar
    EQUAL_ARGUMENT  // The option takes argument after equal    --foo=bar
  };
  //ETX
  
  // Description:
  // Initialize internal data structures. This should be called before parsing.
  void Initialize(int argc, char* argv[]);

  // Description:
  // This method will parse arguments and call apropriate methods. 
  int Parse();

  // Description:
  // This method will add a callback for a specific argument. The arguments to
  // it are argument, argument type, callback method, and call data. The
  // argument help specifies the help string used with this option.
  void AddCallback(const char* argument, int type, CallbackType callback, 
                   void* call_data, const char* help);

  // Description:
  // This method registers callbacks for argument types from array of
  // structures. It stops when an entry has all zeros. 
  void AddCallbacks(CallbackStructure* callbacks);

  // Description:
  // Set the callbacks for error handling.
  void SetClientData(void* client_data);
  void SetUnknownArgumentCallback(ErrorCallbackType callback);

  // Description:
  // Get remaining arguments. It allocates space for argv, so you have to call
  // delete[] on it.
  void GetRemainingArguments(int* argc, char*** argv);

  // Description:
  // Return string containing help.
  vtkGetStringMacro(Help);

  // Description:
  // Get / Set the line length. Default length is 80.
  vtkSetMacro(LineLength, unsigned int);
  vtkGetMacro(LineLength, unsigned int);

protected:
  vtkKWArguments();
  ~vtkKWArguments();

  vtkSetStringMacro(Help);
  void GenerateHelp();

  Internal* Internals;
  char* Help;

  unsigned int LineLength;

private:
  vtkKWArguments(const vtkKWArguments&); // Not implemented
  void operator=(const vtkKWArguments&); // Not implemented
};


#endif





