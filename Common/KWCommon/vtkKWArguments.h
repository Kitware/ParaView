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


  //BTX

  // Description:
  // These are different argument types.
  enum ArgumentTypeEnum { 
    NO_ARGUMENT,    // The option takes no argument             --foo
    CONCAT_ARGUMENT,// The option takes argument after no space --foobar
    SPACE_ARGUMENT, // The option takes argument after space    --foo bar
    EQUAL_ARGUMENT  // The option takes argument after equal    --foo=bar
  };

  enum VariableTypeEnum {
    NO_VARIABLE_TYPE = 0, // The variable is not specified
    INT_TYPE,             // The variable is integer (int)
    DOUBLE_TYPE,          // The variable is float (double)
    STRING_TYPE           // The variable is string (char*)
  };

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
    void* Variable;
    int VariableType;
    const char* Help;
    };
  
  // Description:
  // Initialize internal data structures. This should be called before parsing.
  void Initialize(int argc, char* argv[]);

  //ETX
  
  // Description:
  // Initialize internal data structure and pass arguments one by one. This is
  // convinience method for use from scripting languages where argc and argv
  // are not available.
  void Initialize();
  void AddArgument(const char* arg);

  // Description:
  // This method will parse arguments and call apropriate methods. 
  int Parse();

  // Description:
  // This method will add a callback for a specific argument. The arguments to
  // it are argument, argument type, callback method, and call data. The
  // argument help specifies the help string used with this option. The
  // callback and call_data can be skipped.
  void AddCallback(const char* argument, ArgumentTypeEnum type, CallbackType callback, 
                   void* call_data, const char* help);
  void AddCallback(const char* argument, ArgumentTypeEnum type, const char* help)
    {
    this->AddCallback(argument, type, 0, 0, help);
    }

  // Description:
  // Add handler for argument which is going to set the variable to the
  // specified value.
  void AddHandler(const char* argument, ArgumentTypeEnum type, VariableTypeEnum vtype, void* variable, const char* help);
  void AddHandler(const char* argument, ArgumentTypeEnum type, int* variable, const char* help);
  void AddHandler(const char* argument, ArgumentTypeEnum type, double* variable, const char* help);
  void AddHandler(const char* argument, ArgumentTypeEnum type, char** variable, const char* help);
  void AddBooleanHandler(const char* argument, int* variable, const char* help);

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
  // Return string containing help. If the argument is specified, only return
  // help for that argument.
  vtkGetStringMacro(Help);
  const char* GetHelp(const char* arg);

  // Description:
  // Get / Set the line length. Default length is 80.
  vtkSetMacro(LineLength, unsigned int);
  vtkGetMacro(LineLength, unsigned int);

  // Description:
  // This are methods for map interface. After calling ->Parse(), the program
  // can ask the map for its entries.
  int IsSpecified(const char* arg);
  const char* GetValue(const char* arg);

protected:
  vtkKWArguments();
  ~vtkKWArguments();

  vtkSetStringMacro(Help);
  void GenerateHelp();

  typedef vtkKWArgumentsInternal Internal;
  Internal* Internals;
  char* Help;

  unsigned int LineLength;

private:
  vtkKWArguments(const vtkKWArguments&); // Not implemented
  void operator=(const vtkKWArguments&); // Not implemented
};


#endif





