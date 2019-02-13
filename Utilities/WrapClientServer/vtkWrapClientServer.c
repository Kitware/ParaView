/*=========================================================================

  Program:   ParaView
  Module:    vtkWrapClientServer.c

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWrap.h"
#include "vtkParse.h"
#include "vtkParseHierarchy.h"
#include "vtkParseMain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int numberOfWrappedFunctions = 0;
FunctionInfo* wrappedFunctions[1000];
extern FunctionInfo* currentFunction;
HierarchyInfo* hierarchyInfo = NULL;

/* make a guess about whether a class is wrapped */
static int class_is_wrapped(const char* classname)
{
  HierarchyEntry* entry;

  if (hierarchyInfo)
  {
    entry = vtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry)
    {
      /* only allow non-excluded vtkObjects as args */
      if (vtkParseHierarchy_GetProperty(entry, "WRAPEXCLUDE") ||
        !vtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "vtkObjectBase"))
      {
        return 0;
      }

      /* the primary class is the one the header is named after */
      return vtkParseHierarchy_IsPrimary(entry);
    }
  }

  return 1;
}

int arg_is_pointer_to_data(unsigned int argType, int count)
{
  return (count == 0 && (argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER && /* T*   */
    (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_STRING &&                     /* vtkStdString* */
    (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VOID &&                       /* void*    */
    (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_CHAR &&                       /* char*    */
    (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_BOOL &&                       /* bool*    */
    (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_UNKNOWN &&                    /* Foo*  */
    (argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VTK_OBJECT /* vtkFoo*  */);
}

void output_temp(FILE* fp, int i, unsigned int argType, const char* Id, int count)
{
  /* Store whether this is pointer to data.  */
  int isPointerToData = i != MAX_ARGS && arg_is_pointer_to_data(argType, count);

  /* ignore void */
  if (((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID) && ((argType & VTK_PARSE_INDIRECT) == 0))
  {
    return;
  }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) && ((argType & VTK_PARSE_CONST) != 0))
  {
    fprintf(fp, "    const ");
  }
  else
  {
    fprintf(fp, "    ");
  }

  /* Handle some objects of known type.  */
  if (((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT) &&
    (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) ||
        ((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)) &&
    strcmp(Id, "vtkClientServerStream") == 0)
  {
    fprintf(fp, "vtkClientServerStream temp%i_inst, *temp%i = &temp%i_inst;\n", i, i, i);
    return;
  }

  /* Start pointer-to-data arguments.  */
  if (isPointerToData)
  {
    fprintf(fp, "vtkClientServerStreamDataArg<");
  }

  if (argType & VTK_PARSE_UNSIGNED)
  {
    fprintf(fp, "unsigned ");
  }

  switch ((argType & VTK_PARSE_BASE_TYPE) & ~VTK_PARSE_UNSIGNED)
  {
    case VTK_PARSE_FLOAT:
      fprintf(fp, "float  ");
      break;
    case VTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case VTK_PARSE_INT:
      fprintf(fp, "int    ");
      break;
    case VTK_PARSE_SHORT:
      fprintf(fp, "short  ");
      break;
    case VTK_PARSE_LONG:
      fprintf(fp, "long   ");
      break;
    case VTK_PARSE_VOID:
      fprintf(fp, "void   ");
      break;
    case VTK_PARSE_CHAR:
      fprintf(fp, "char   ");
      break;
    case VTK_PARSE_ID_TYPE:
      fprintf(fp, "vtkIdType ");
      break;
    case VTK_PARSE_LONG_LONG:
      fprintf(fp, "long long ");
      break;
    case VTK_PARSE___INT64:
      fprintf(fp, "__int64 ");
      break;
    case VTK_PARSE_SIGNED_CHAR:
      fprintf(fp, "signed char ");
      break;
    case VTK_PARSE_BOOL:
      fprintf(fp, "bool ");
      break;
    case VTK_PARSE_VTK_OBJECT:
      fprintf(fp, "%s ", Id);
      break;
    case VTK_PARSE_STRING:
      if (i == MAX_ARGS)
      {
        fprintf(fp, "%s ", Id);
        break;
      }
      else
      {
        fprintf(fp, "char    *");
      }
      break;

    case VTK_PARSE_UNKNOWN:
      return;
  }

  /* Finish pointer-to-data arguments.  */
  if (isPointerToData)
  {
    fprintf(fp, "> temp%i(msg, 0, %i);\n", i, i + 2);
    return;
  }

  /* handle array arguments */
  if (count > 1)
  {
    fprintf(fp, "temp%i[%i];\n", i, count);
    return;
  }

  switch (argType & VTK_PARSE_INDIRECT)
  {
    case VTK_PARSE_REF:
      if (i == MAX_ARGS)
      {
        fprintf(fp, " *"); /* act " &" */
      }
      break;
    case VTK_PARSE_POINTER:
      fprintf(fp, " *");
      break;
    case VTK_PARSE_POINTER_REF:
      fprintf(fp, "*&");
      break;
    case VTK_PARSE_POINTER_POINTER:
      fprintf(fp, "**");
      break;
    default:
      fprintf(fp, "  ");
      break;
  }

  fprintf(fp, "temp%i", i);
  fprintf(fp, ";\n");
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE* fp)
{
  unsigned int rType = currentFunction->ReturnType;

  /* use the hint */
  if ((rType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)
  {
    switch (rType & VTK_PARSE_BASE_TYPE)
    {
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
      case VTK_PARSE_INT:
      case VTK_PARSE_SHORT:
      case VTK_PARSE_LONG:
      case VTK_PARSE_ID_TYPE:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE___INT64:
      case VTK_PARSE_UNSIGNED_CHAR:
      case VTK_PARSE_SIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_INT:
      case VTK_PARSE_UNSIGNED_SHORT:
      case VTK_PARSE_UNSIGNED_LONG:
      case VTK_PARSE_UNSIGNED_ID_TYPE:
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
        fprintf(fp,
          "      resultStream.Reset();\n"
          "      resultStream << vtkClientServerStream::Reply << "
          "vtkClientServerStream::InsertArray(temp%i,%i) << vtkClientServerStream::End;\n",
          MAX_ARGS, currentFunction->HintSize);
        break;
    }
  }
}

void return_result(FILE* fp)
{
  unsigned int rType = currentFunction->ReturnType;
  const char* rClass = currentFunction->ReturnClass;

  switch (rType & VTK_PARSE_BASE_TYPE)
  {
    case VTK_PARSE_VOID:
      if ((rType & VTK_PARSE_INDIRECT) == 0)
      {
        return;
      }
      break;

    case VTK_PARSE_FLOAT:
    case VTK_PARSE_CHAR:
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_DOUBLE:
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_BOOL:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
    case VTK_PARSE_STRING:
      if ((rType & VTK_PARSE_INDIRECT) == 0 ||
        (((rType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_CHAR) &&
            ((rType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)))
      {
        fprintf(fp, "      resultStream.Reset();\n"
                    "      resultStream << vtkClientServerStream::Reply << temp%i << "
                    "vtkClientServerStream::End;\n",
          MAX_ARGS);
        return;
      }
      else if ((rType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
      {
        fprintf(fp, "      resultStream.Reset();\n"
                    "      resultStream << vtkClientServerStream::Reply << *temp%i << "
                    "vtkClientServerStream::End;\n",
          MAX_ARGS);
        return;
      }
      else if ((rType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)
      {
        /* handle functions returning vectors */
        /* this is done by looking them up in a hint file */
        use_hints(fp);
        return;
      }
      break;

    case VTK_PARSE_VTK_OBJECT:
      /* Handle some objects of known type.  */
      if (strcmp(rClass, "vtkClientServerStream") == 0)
      {
        fprintf(fp, "      resultStream.Reset();\n"
                    "      resultStream << vtkClientServerStream::Reply << *temp%i << "
                    "vtkClientServerStream::End;\n",
          MAX_ARGS);
        return;
      }
      else if ((rType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)
      {
        fprintf(fp, "      resultStream.Reset();\n"
                    "      resultStream << vtkClientServerStream::Reply << (vtkObjectBase *)temp%i "
                    "<< vtkClientServerStream::End;\n",
          MAX_ARGS);
        return;
      }
      break;
  }

  /* if we get to here, then the type was not recognized */

  fprintf(fp, "      resultStream.Reset();\n"
              "      resultStream << vtkClientServerStream::Reply\n"
              "                   << \"unable to return result of type(%#x).\"\n"
              "                   << vtkClientServerStream::End;\n",
    (rType & VTK_PARSE_UNQUALIFIED_TYPE));
}

void get_args(FILE* fp, int i)
{
  unsigned int argType = currentFunction->ArgTypes[i];
  int argCount = currentFunction->ArgCounts[i];
  const char* argClass = currentFunction->ArgClasses[i];
  int j;
  int start_arg = 2;

  /* what arg do we start with */
  for (j = 0; j < i; j++)
  {
    start_arg = start_arg + (currentFunction->ArgCounts[j] ? currentFunction->ArgCounts[j] : 1);
  }

  /* ignore void */
  if (((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID) && ((argType & VTK_PARSE_INDIRECT) == 0))
  {
    return;
  }

  switch (argType & VTK_PARSE_BASE_TYPE)
  {
    case VTK_PARSE_FLOAT:
    case VTK_PARSE_DOUBLE:
    case VTK_PARSE_INT:
    case VTK_PARSE_SHORT:
    case VTK_PARSE_LONG:
    case VTK_PARSE_ID_TYPE:
    case VTK_PARSE_LONG_LONG:
    case VTK_PARSE___INT64:
    case VTK_PARSE_SIGNED_CHAR:
    case VTK_PARSE_BOOL:
    case VTK_PARSE_CHAR:
    case VTK_PARSE_UNSIGNED_CHAR:
    case VTK_PARSE_UNSIGNED_INT:
    case VTK_PARSE_UNSIGNED_SHORT:
    case VTK_PARSE_UNSIGNED_LONG:
    case VTK_PARSE_UNSIGNED_ID_TYPE:
    case VTK_PARSE_UNSIGNED_LONG_LONG:
    case VTK_PARSE_UNSIGNED___INT64:
    case VTK_PARSE_STRING:
      if (((argType & VTK_PARSE_INDIRECT) == 0) ||
        ((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF) ||
        (((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_CHAR) &&
            ((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)))
      {
        fprintf(fp, "msg.GetArgument(0, %i, &temp%i)", i + 2, i);
      }
      else if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) && (argCount > 1))
      {
        fprintf(fp, "msg.GetArgument(0, %i, temp%i, %i)", i + 2, i, argCount);
      }
      else if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) &&
        (arg_is_pointer_to_data(argType, argCount)))
      {
        /* Pointer-to-data arguments are handled by an object
           convertible to bool.  */
        fprintf(fp, "temp%i", i);
      }

      break;

    case VTK_PARSE_VTK_OBJECT:
      /* Handle some objects of known type.  */
      if ((((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF) ||
            ((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)) &&
        strcmp(argClass, "vtkClientServerStream") == 0)
      {
        fprintf(fp, "msg.GetArgument(0, %i, temp%i)", i + 2, i);
      }
      else if ((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)
      {
        fprintf(fp, "vtkClientServerStreamGetArgumentObject(msg, 0, %i, &temp%i, \"%s\")", i + 2, i,
          argClass);
      }
      break;

    case VTK_PARSE_VOID:
    case VTK_PARSE_UNKNOWN:
      break;
  }
}

/* declare these so they can be used in outputFunction */
int managableArguments(FunctionInfo* curFunction);
int notWrappable(FunctionInfo* curFunction);

void outputFunction(FILE* fp, ClassInfo* data)
{
  int i;

  if (notWrappable(currentFunction))
  {
    return;
  }

  /* if the args are OK and it is not a constructor or destructor */
  if (managableArguments(currentFunction) && strcmp(data->Name, currentFunction->Name) &&
    strcmp(data->Name, currentFunction->Name + 1))
  {
    if (currentFunction->IsLegacy)
    {
      fprintf(fp, "#if !defined(VTK_LEGACY_REMOVE)\n");
    }
    fprintf(fp, "  if (!strcmp(\"%s\",method) && msg.GetNumberOfArguments(0) == %i)\n",
      currentFunction->Name, currentFunction->NumberOfArguments + 2);
    fprintf(fp, "    {\n");

    /* process the args */
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
      output_temp(fp, i, currentFunction->ArgTypes[i], currentFunction->ArgClasses[i],
        currentFunction->ArgCounts[i]);
    }
    output_temp(fp, MAX_ARGS, currentFunction->ReturnType, currentFunction->ReturnClass, 0);

    if (currentFunction->NumberOfArguments > 0)
    {
      const char* amps = "    if(";
      /* now get the required args from the stack */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        fprintf(fp, "%s", amps);
        amps = " &&\n      ";
        get_args(fp, i);
      }
      fprintf(fp, ")\n");
    }
    fprintf(fp, "      {\n");

    if ((currentFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_VOID)
    {
      fprintf(fp, "      op->%s(", currentFunction->Name);
    }
    else if ((currentFunction->ReturnType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
    {
      fprintf(fp, "      temp%i = &(op)->%s(", MAX_ARGS, currentFunction->Name);
    }
    else
    {
      fprintf(fp, "      temp%i = (op)->%s(", MAX_ARGS, currentFunction->Name);
    }

    for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
      if (i)
      {
        fprintf(fp, ",");
      }
      if (((currentFunction->ArgTypes[i] & VTK_PARSE_INDIRECT) == VTK_PARSE_REF) &&
        ((currentFunction->ArgTypes[i] & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT))
      {
        fprintf(fp, "*(temp%i)", i);
      }
      else if ((((currentFunction->ArgTypes[i] & VTK_PARSE_INDIRECT) == 0) ||
                 ((currentFunction->ArgTypes[i] & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)) &&
        ((currentFunction->ArgTypes[i] & VTK_PARSE_BASE_TYPE) == VTK_PARSE_STRING))
      {
        /* explicit construction avoids possible ambiguity */
        fprintf(fp, "vtkStdString(temp%i)", i);
      }
      else
      {
        fprintf(fp, "temp%i", i);
      }
    }
    fprintf(fp, ");\n");
    return_result(fp);
    fprintf(fp, "      return 1;\n");
    fprintf(fp, "      }\n");
    fprintf(fp, "    }\n");
    if (currentFunction->IsLegacy)
    {
      fprintf(fp, "#endif\n");
    }

    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
  }

#if 0
  if (!strcmp("vtkObject",data->Name))
    {
    fprintf(fp,"  if (!strcmp(\"AddProgressObserver\",method) && msg.NumberOfArguments == 3 &&\n");
    fprintf(fp,"      msg.ArgumentTypes[2] == vtkClietnServerStream::string_value)\n");
    fprintf(fp,"    {\n");
    fprintf(fp,"    vtkClientServerProgressObserver *apo = vtkClientServerProgressObserver::New();\n");
    fprintf(fp,"    vtkObject* obj = arlu->GetObjectFromMessage(msg, 0, 1);\n");
    fprintf(fp,"    apo->SetFilterID(arlu->GetIDFromObject(obj));\n");
    fprintf(fp,"    apo->SetClientServerUtil(arlu);\n");
    fprintf(fp,"    char *temp0 = vtkClientServerInterpreter::GetString(msg,2);\n");
    fprintf(fp,"    op->AddObserver(temp0,apo);\n");
    fprintf(fp,"    apo->Delete();\n");
    fprintf(fp,"    delete [] temp0;\n");
    fprintf(fp,"    return 1;\n");
    fprintf(fp,"    }\n");
    }
#endif
}

//--------------------------------------------------------------------------nix
/*
 * This structure is used internally to sort+collect individual functions.
 * Polymorphed functions will be combined and can be handeled together.
 *
 */
typedef struct _UniqueFunctionInfo
{
  const char* Name;
  int TotalPolymorphTypes;
  FunctionInfo* Function[20];
} UniqueFunctionInfo;

//--------------------------------------------------------------------------nix
/*
 * This structure is used to collect and hold class information. It is a
 * modified version of FileInfo
 *
 */
typedef struct _NewClassInfo
{
  int HasDelete;
  int IsAbstract;
  int IsConcrete;
  const char* ClassName;
  const char* FileName;
  const char* OutputFileName;
  const char* SuperClasses[10];
  int NumberOfSuperClasses;
  int NumberOfFunctions;
  UniqueFunctionInfo Functions[1000];
  const char* NameComment;
  const char* Description;
  const char* Caveats;
  const char* SeeAlso;
} NewClassInfo;

//--------------------------------------------------------------------------nix
/*
 * notWrappable returns true if the current-function is not wrappable.
 *
 * @param curFunction function-info being worked on
 *
 * @return true if the function is not wrappable
 */
int notWrappable(FunctionInfo* curFunction)
{
  return (curFunction->IsOperator || curFunction->ArrayFailure || !curFunction->IsPublic ||
    !curFunction->Name || curFunction->Template || curFunction->IsExcluded);
}

//--------------------------------------------------------------------------nix
/*
 * managableArguments check if the functions arguments are in a form
 * which can easily be used for automatic wrapper generation.
 *
 * @param curFunction the function beign worked on
 *
 * @return true if the arguments are okay for code generation
 */
int managableArguments(FunctionInfo* curFunction)
{
  static unsigned int supported_types[] = { VTK_PARSE_VOID, VTK_PARSE_BOOL, VTK_PARSE_FLOAT,
    VTK_PARSE_DOUBLE, VTK_PARSE_CHAR, VTK_PARSE_UNSIGNED_CHAR, VTK_PARSE_SIGNED_CHAR, VTK_PARSE_INT,
    VTK_PARSE_UNSIGNED_INT, VTK_PARSE_SHORT, VTK_PARSE_UNSIGNED_SHORT, VTK_PARSE_LONG,
    VTK_PARSE_UNSIGNED_LONG, VTK_PARSE_ID_TYPE, VTK_PARSE_UNSIGNED_ID_TYPE, VTK_PARSE_LONG_LONG,
    VTK_PARSE_UNSIGNED_LONG_LONG, VTK_PARSE___INT64, VTK_PARSE_UNSIGNED___INT64,
    VTK_PARSE_VTK_OBJECT, VTK_PARSE_STRING, 0 };

  int i, j;
  int args_ok = 1;
  unsigned int returnType = 0;
  unsigned int argType = 0;
  unsigned int baseType = 0;

  /* check to see if we can handle the args */
  for (i = 0; i < curFunction->NumberOfArguments; i++)
  {
    int isPointerToData =
      arg_is_pointer_to_data(curFunction->ArgTypes[i], curFunction->ArgCounts[i]);
    argType = (curFunction->ArgTypes[i] & VTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (argType & VTK_PARSE_BASE_TYPE);

    if (curFunction->ArgTypes[i] != VTK_PARSE_FUNCTION)
    {
      for (j = 0; supported_types[j] != 0; j++)
      {
        if (baseType == supported_types[j])
        {
          break;
        }
      }
      if (supported_types[j] == 0)
      {
        args_ok = 0;
      }
    }

    /* if its a pointer arg make sure we have the ArgCount */
    if (((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER) && !isPointerToData &&
      ((argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_CHAR) &&
      ((argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VTK_OBJECT))
    {
      if (curFunction->NumberOfArguments > 1 || curFunction->ArgCounts[i] == 0 ||
        ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_STRING))
      {
        args_ok = 0;
      }
    }

    /* if it has a reference arg, don't wrap it */
    if ((argType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
    {
      /* make exception for "vtkClientServerStream&" */
      if (((argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VTK_OBJECT) ||
        strcmp(curFunction->ArgClasses[i], "vtkClientServerStream"))
      {
        /* also make exception for "const vtkStdString&" */
        if ((argType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_STRING ||
          (curFunction->ArgTypes[i] & VTK_PARSE_CONST) == 0)
        {
          args_ok = 0;
        }
      }
    }

    /* if it is a vtk object that isn't a pointer or ref, don't wrap it */
    if (((argType & VTK_PARSE_INDIRECT) == 0) &&
      ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT))
    {
      args_ok = 0;
    }

    /* if arg is a vtk object, make sure it is a wrapped object */
    if ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT &&
      !class_is_wrapped(curFunction->ArgClasses[i]))
    {
      args_ok = 0;
    }

    /* if it is "**" or "*&" then don't wrap it */
    if (((argType & VTK_PARSE_INDIRECT) != 0) &&
      ((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
      ((argType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF))
    {
      args_ok = 0;
    }

    /* if it is an array of chars, then don't wrap it */
    if ((argType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_CHAR && curFunction->ArgCounts[i] != 0)
    {
      args_ok = 0;
    }

    if ((argType & VTK_PARSE_UNSIGNED) && (argType != VTK_PARSE_UNSIGNED_CHAR) &&
      (argType != VTK_PARSE_UNSIGNED_INT) && (argType != VTK_PARSE_UNSIGNED_SHORT) &&
      (argType != VTK_PARSE_UNSIGNED_LONG) && (argType != VTK_PARSE_UNSIGNED_ID_TYPE) &&
      !isPointerToData)
    {
      args_ok = 0;
    }
  }

  /* check the return type */
  returnType = (curFunction->ReturnType & VTK_PARSE_UNQUALIFIED_TYPE);
  baseType = (returnType & VTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
  {
    if (baseType == supported_types[j])
    {
      break;
    }
  }
  if (supported_types[j] == 0)
  {
    args_ok = 0;
  }

  /* if it is a reference, then don't wrap it */
  if ((returnType & VTK_PARSE_INDIRECT) == VTK_PARSE_REF)
  {
    /* make exception for "vtkClientServerStream&" */
    if (((returnType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_VTK_OBJECT) ||
      strcmp(curFunction->ReturnClass, "vtkClientServerStream"))
    {
      /* also make exception for "vtkStdString" */
      if ((returnType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_STRING)
      {
        args_ok = 0;
      }
    }
  }

  /* if it is a vtk object that isn't a pointer, don't wrap it */
  if (((returnType & VTK_PARSE_INDIRECT) == 0) &&
    ((returnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT))
  {
    args_ok = 0;
  }

  /* if arg is a vtk object, make sure it is a wrapped object */
  if ((returnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VTK_OBJECT &&
    !class_is_wrapped(curFunction->ReturnClass))
  {
    args_ok = 0;
  }

  /* if it is "**" or "*&" then don't wrap it */
  if (((returnType & VTK_PARSE_INDIRECT) != 0) &&
    ((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER) &&
    ((returnType & VTK_PARSE_INDIRECT) != VTK_PARSE_REF))
  {
    args_ok = 0;
  }

  /* cannot wrap function pointers */
  if (curFunction->NumberOfArguments && (curFunction->ArgTypes[0] == VTK_PARSE_FUNCTION))
  {
    args_ok = 0;
  }

  /* we can't handle void * return types */
  if (((returnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_VOID) &&
    ((returnType & VTK_PARSE_INDIRECT) != 0))
  {
    args_ok = 0;
  }

  /* watch out for functions that don't have enough info */
  if ((returnType & VTK_PARSE_INDIRECT) == VTK_PARSE_POINTER)
  {
    switch (returnType & VTK_PARSE_BASE_TYPE)
    {
      case VTK_PARSE_FLOAT:
      case VTK_PARSE_DOUBLE:
      case VTK_PARSE_INT:
      case VTK_PARSE_SHORT:
      case VTK_PARSE_LONG:
      case VTK_PARSE_ID_TYPE:
      case VTK_PARSE_LONG_LONG:
      case VTK_PARSE___INT64:
      case VTK_PARSE_SIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_CHAR:
      case VTK_PARSE_UNSIGNED_INT:
      case VTK_PARSE_UNSIGNED_SHORT:
      case VTK_PARSE_UNSIGNED_LONG:
      case VTK_PARSE_UNSIGNED_ID_TYPE:
      case VTK_PARSE_UNSIGNED_LONG_LONG:
      case VTK_PARSE_UNSIGNED___INT64:
        args_ok = curFunction->HaveHint;
        break;

      case VTK_PARSE_CHAR:
      case VTK_PARSE_VTK_OBJECT:
        break;

      default:
        args_ok = 0;
        break;
    }
  }

  return args_ok;
}

//--------------------------------------------------------------------------nix
/*
 * funCmp is used to compare the function names of two FunInfo data.
 * This is used as a utility to the sort function.
 *
 * @param fun1 first function data which is compared
 * @param fun2 second function data which is compared
 *
 * @return values returned by strcmp
 */

int funCmp(const void* fun1, const void* fun2)
{
  FunctionInfo* a = (FunctionInfo*)fun1;
  FunctionInfo* b = (FunctionInfo*)fun2;
  return strcmp(a->Name, b->Name);
}

//--------------------------------------------------------------------------nix
/*
 * copy copies data from the source array to the destination
 *
 * @param from the source array from which we want to copy data
 * @param fromSize the size of the source array
 * @param to the destination to which we wanto to copy the data
 *
 * @return the size of the destination array
 */
/*
int copy(FunctionInfo* from, int fromSize, FunctionInfo* to)
{
  int i;
  for (i=0 ; i<fromSize;i++)
    {
    to[i] =  from[i];
    }
  return i;
}
*/

//--------------------------------------------------------------------------nix
/*
 * extractWrappable copies data from the source array to the destination
 * array and removes unwrappable functions whenever encountered.
 * It returns the count of the destination array.
 *
 * @param from the source array from which we want to extract data
 * @param fromSize the size of the source array
 * @param to the destination to which we wanto to copy extracted data
 *
 * @return the size of the destination array (this may not be the size of the source)
 */
int extractWrappable(FunctionInfo* from[], int fromSize, FunctionInfo* to[], const char* ClassName)
{
  int i, j;
  for (i = 0, j = 0; i < fromSize; i++)
  {
    /* if the function is wrappable and
       args are OK and
       it is not a constructor or destructor */
    if (!notWrappable(from[i]) && managableArguments(from[i]) && strcmp(ClassName, from[i]->Name) &&
      strcmp(ClassName, from[i]->Name + 1))
    {
      to[j] = from[i];
      ++j;
    }
  }
  return j;
}

//--------------------------------------------------------------------------nix
/*
 * collectUniqueFunctionInfo collects unique function. Polymorphed functions
 * are grouped in the UniqueFunctionInfo structure.
 *
 * @param src source data array with all applicable function info
 * @param srcSize the size of the array holding this info
 * @param dest the destnation array to store the extracted info
 *
 * @return the total number of elements in the dest array
 */
int collectUniqueFunctionInfo(FunctionInfo* src[], int srcSize, UniqueFunctionInfo dest[])
{
  int i, j, k;

  for (i = 0; i < srcSize; i++)
  {
    dest[i].Name = src[i]->Name;
    dest[i].TotalPolymorphTypes = 1;
    dest[i].Function[0] = src[i];
    for (j = i + 1; j < srcSize; j++)
    {
      if (strcmp(dest[i].Name, src[j]->Name) == 0)
      {
        // printf("%d [%d]> dest = %s :: src = %s\n",i,srcSize, dest[i].Name,src[j].Name);
        // printf("    > [SAME]\n");
        dest[i].Function[dest[i].TotalPolymorphTypes] = src[j];
        dest[i].TotalPolymorphTypes++;

        for (k = j; k < srcSize - 1; k++)
        {
          // printf("remaining = %s\n",src[k]->Name);
          src[k] = src[k + 1];
        }
        j--;
        srcSize--;
      }
    }
  }

  return srcSize;
}

//--------------------------------------------------------------------------nix
/*
 * This is a format converter function which converts from traditional
 * FileInfo format to NewClassInfo format. The NewClassInfo format combines
 * polymorphed functions into UniqueFunctionInfo internal format.
 *
 * @param data hold the collected file data form the parser
 * @param classData the form into which we want to convert
 */
void getClassInfo(FileInfo* fileInfo, ClassInfo* data, NewClassInfo* classData)
{
  int i;
  int TotalUniqueFunctions = 0;
  int TotalFunctions;
  FunctionInfo** tempFun = (FunctionInfo**)malloc(sizeof(FunctionInfo*) * data->NumberOfFunctions);

  /* Collect all the information */
  classData->HasDelete = data->HasDelete;
  classData->IsAbstract = data->IsAbstract;
  classData->IsConcrete = !data->IsAbstract;
  classData->ClassName = data->Name;
  classData->FileName = fileInfo->FileName;
  classData->OutputFileName = "";
  classData->NumberOfSuperClasses = data->NumberOfSuperClasses;
  for (i = 0; i < data->NumberOfSuperClasses; ++i)
  {
    classData->SuperClasses[i] = data->SuperClasses[i];
  }
  classData->NameComment = fileInfo->NameComment;
  classData->Description = fileInfo->Description;
  classData->Caveats = fileInfo->Caveats;
  classData->SeeAlso = fileInfo->SeeAlso;

  /* Collect only wrappable functions*/
  TotalFunctions = extractWrappable(data->Functions, data->NumberOfFunctions, tempFun, data->Name);
  /* Sort the function data. */
  // qsort(tempFun,TotalFunctions,sizeof(FunctionInfo),funCmp);

  /*   for(i=0;i<TotalFunctions;i++) */
  /*     { */
  /*     printf("(funargs %s %d)\n",tempFun[i].Name,tempFun[i].NumberOfArguments); */
  /*     } */

  //  printf("START\n");
  /* Collect unique and group polymorphed functions in UniqueFunctionInfo */
  TotalUniqueFunctions = collectUniqueFunctionInfo(tempFun, TotalFunctions, classData->Functions);
  //  printf("END\n");

  /*  for(i=0;i<TotalUniqueFunctions;i++) */
  /*     { */
  /*     printf("(funargs %s %d)\n",tempFun[i].Name,tempFun[i].NumberOfArguments); */
  /*     } */

  classData->NumberOfFunctions = TotalUniqueFunctions;
  free(tempFun);
}

//--------------------------------------------------------------------------nix
/*
 * Checks if the given string in unique in a list of strings
 *
 * @param main The main class name which is compared with others
 * @param list The list which is compared with
 * @param total total number of elements in the list
 *
 * @return 0 if found and 1 if not
 */
int isUniqueString(const char* main, const char* list[], int total)
{
  int i;
  for (i = 0; i < total; ++i)
  {
    if (strcmp(main, list[i]) == 0)
      return 0;
  }
  return 1;
}

//--------------------------------------------------------------------------nix
/*
 * This function takes a list of class names and replaces with a unique list
 * The total unique elements is returned
 *
 * @param classes IN/OUT List to hold the classes
 * @param total IN the total number of classes
 *
 * @return the total list of unique classes
 */
int uniqueClasses(const char* classes[], int total, const char* classSelfName)
{
  int i, j = 0;
  const char* temp[1000];
  const char* current_class_name;
  for (i = total - 1; i >= 0; --i)
  {
    current_class_name = classes[i];
    if (strcmp(current_class_name, classSelfName) != 0 &&
      // hack
      strcmp(current_class_name, "vtkClientServerStream") != 0 &&
      isUniqueString(current_class_name, &temp[0], j)
      // && strcmp(classes[i],"vtkObjectBase")!=0)
      )
    {
      temp[j] = current_class_name;
      ++j;
    }
  }
  for (i = 0; i < j; ++i)
  {
    classes[i] = temp[i];
  }
  return j;
}

//--------------------------------------------------------------------------nix
/* Some classes refer to classes that are external to their libraries,
 * these used to be handled in a messy way with VTK_WRAP_EXTERN.  In the
 * future, they will be handled with a vtkWrapHierarchy tool that
 * can automatically detect external objects.
 *
 * @param classname IN -> the referrer class
 * @param argclass IN -> the class being referred to
 *
 * @return true if argclass is external to classname's kit
 */
static int isExternalObject(const char* classname, const char* argclass)
{
  static const char* wrapExtern[] = { "vtkInformation", "vtkDataObject", "vtkPolyDataSilhouette",
    "vtkProp3D", "vtkPolyDataSilhouette", "vtkCamera", 0, 0 };
  const char** externCheck;

  for (externCheck = wrapExtern; *externCheck != 0; externCheck += 2)
  {
    if (strcmp(externCheck[0], classname) == 0 && strcmp(externCheck[1], argclass) == 0)
    {
      return 1;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------nix
/*
 * This is used to extract all the other classes used. This includes
 * superclasses, class data type passed as parameters and class
 * data-types returned by the methods.
 *
 * @param data IN -> to extract a list of other classes used
 * @param classes OUT <- the structure to hold the extracted classes
 *
 * @return the count of the extracted unique classes
 */
int extractOtherClassesUsed(NewClassInfo* data, const char* classes[])
{
  int i, j, k;
  int count = 0;
  // Collect all the superclasses
  for (i = 0; i < data->NumberOfSuperClasses; ++i)
    classes[i] = data->SuperClasses[i];
  count = data->NumberOfSuperClasses;
  // return count; // HACK only returns the super classes

  // Collect all the functions params and return types
  for (i = 0; i < data->NumberOfFunctions; ++i)
  {
    for (j = 0; j < data->Functions[i].TotalPolymorphTypes; ++j)
    {
      for (k = 0; k < data->Functions[i].Function[j]->NumberOfArguments; ++k)
      {
        if ((data->Functions[i].Function[j]->ArgTypes[k] & VTK_PARSE_BASE_TYPE) ==
          VTK_PARSE_VTK_OBJECT)
        {
          if (!isExternalObject(data->ClassName, data->Functions[i].Function[j]->ArgClasses[k]))
          {
            classes[count] = data->Functions[i].Function[j]->ArgClasses[k];
            ++count;
          }
        }
      }
      if ((data->Functions[i].Function[j]->ReturnType & VTK_PARSE_BASE_TYPE) ==
        VTK_PARSE_VTK_OBJECT)
      {
        if (!isExternalObject(data->ClassName, data->Functions[i].Function[j]->ReturnClass))
        {
          classes[count] = data->Functions[i].Function[j]->ReturnClass;
          ++count;
        }
      }
    }
  }
  return uniqueClasses(classes, count, data->ClassName);
}

//--------------------------------------------------------------------------nix
/*
 * outputs the "Object"_Init(vtkClientServerInterpreter*csi) file. This is
 * used to register and initialize the classes and their dependent objects
 * to the ClientServerInterpreter. Dependent objects include superclasses
 * Objects passed as parameters and object types returned.
 *
 * @param fp file to write into
 * @param data data which will be used to write into file
 */
void output_DummyInitFunction(FILE* fp, const char* filename)
{
  char* basename = strrchr(filename, '/');
  char* basename_dup = strdup(basename + 1);
  *strchr(basename_dup, '.') = '\0';
  fprintf(fp, "#include \"vtkSystemIncludes.h\"\n"
              "#include \"vtkClientServerInterpreter.h\"\n"
              "void VTK_EXPORT %s_Init(vtkClientServerInterpreter* /*csi*/)\n"
              "{\n"
              "}\n",
    basename_dup);
  free(basename_dup);
}

//--------------------------------------------------------------------------nix
/*
 * outputs the "Object"_Init(vtkClientServerInterpreter*csi) file. This is
 * used to register and initialize the classes and their dependent objects
 * to the ClientServerInterpreter. Dependent objects include superclasses
 * Objects passed as parameters and object types returned.
 *
 * @param fp file to write into
 * @param data data which will be used to write into file
 */
void output_InitFunction(FILE* fp, NewClassInfo* data)
{
  fprintf(fp, "\n");
  fprintf(fp, "\n"
              "//-------------------------------------------------------------------------auto\n"
              "void VTK_EXPORT %s_Init(vtkClientServerInterpreter* csi)\n"
              "{\n"
              "  static vtkClientServerInterpreter* last = NULL;\n"
              "  if(last != csi)\n"
              "    {\n"
              "    last = csi;\n",
    data->ClassName);
  if (!data->IsAbstract)
    fprintf(fp, "    csi->AddNewInstanceFunction(\"%s\", %sClientServerNewCommand);\n",
      data->ClassName, data->ClassName);
  fprintf(
    fp, "    csi->AddCommandFunction(\"%s\", %sCommand);\n", data->ClassName, data->ClassName);
  fprintf(fp, "    }\n}\n");
}

/* check all methods for use of vtkStdString */
int classUsesStdString(ClassInfo* data)
{
  int i, j;
  FunctionInfo* info;

  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    info = data->Functions[i];
    if ((info->ReturnType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_STRING)
    {
      return 1;
    }
    for (j = 0; j < info->NumberOfArguments; j++)
    {
      if ((info->ArgTypes[j] & VTK_PARSE_BASE_TYPE) == VTK_PARSE_STRING)
      {
        return 1;
      }
    }
  }
  return 0;
}

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

/* print the parsed structures */
int main(int argc, char* argv[])
{
  OptionInfo* options;
  FileInfo* fileInfo;
  ClassInfo* data;
  NamespaceInfo* ns;
  char nsname[1024];
  struct
  {
    NamespaceInfo* ns;
    size_t nsnamepos;
    int n;
  } nsstack[32];
  size_t nspos;
  FILE* fp;
  NewClassInfo* classData;
  int i, j;

  /* pre-define a macro to identify the language */
  vtkParse_DefineMacro("__VTK_WRAP_CLIENTSERVER__", 0);

  /* get command-line args and parse the header file */
  fileInfo = vtkParse_Main(argc, argv);

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hierarchyInfo =
      vtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the output file */
  fp = fopen(options->OutputFileName, "w");

  if (!fp)
  {
    fprintf(stderr, "Error opening output file %s\n", options->OutputFileName);
    exit(1);
  }

  data = fileInfo->MainClass;

  /* Set up the stack. */
  nspos = 0;
  nsstack[nspos].ns = fileInfo->Contents;
  nsstack[nspos].nsnamepos = 0;
  nsstack[nspos].n = 0;
  nsname[nsstack[nspos].nsnamepos] = '\0';
  ns = nsstack[nspos].ns;

  while (!data && ns)
  {
    if (ns->Name)
    {
      size_t namelen = strlen(nsname);
      snprintf(nsname + namelen, sizeof(nsname) - namelen, "::%s", ns->Name);
    }

    if (ns->NumberOfClasses > 0)
    {
      data = ns->Classes[0];
      break;
    }

    if (ns->NumberOfNamespaces > nsstack[nspos].n)
    {
      /* Use the next namespace. */
      ns = ns->Namespaces[nsstack[nspos].n];
      nsstack[nspos].n++;

      /* Go deeper. */
      nspos++;
      nsstack[nspos].ns = ns;
      nsstack[nspos].nsnamepos = strlen(nsname);
      nsstack[nspos].n = 0;
    }
    else
    {
      if (nspos)
      {
        --nspos;
        /* Reset the namespace name. */
        nsname[nsstack[nspos].nsnamepos] = '\0';
        /* Go back up the stack. */
        ns = nsstack[nspos].ns;
      }
      else
      {
        /* Nothing left to search. */
        ns = NULL;
      }
    }
  }

  if (!data)
  {
    output_DummyInitFunction(fp, fileInfo->FileName);
    fclose(fp);
    exit(0);
  }

  if (data->Template)
  {
    output_DummyInitFunction(fp, fileInfo->FileName);
    fclose(fp);
    exit(0);
  }

  for (i = 0; i < data->NumberOfSuperClasses; ++i)
  {
    if (strncmp(data->SuperClasses[i], "vtk", 3) == 0 && strchr(data->SuperClasses[i], '<'))
    {
      fprintf(fp, "// This automatically generated file contains only a stub,\n");
      fprintf(fp, "// bacause the class %s is based on a templated VTK class.\n", data->Name);
      fprintf(fp, "// Wrapping such classes is not currently supported.\n");
      fprintf(fp, "// Here follows the list of detected superclasses "
                  "(first offending one marked by !):\n");

      for (j = 0; j < data->NumberOfSuperClasses; ++j)
      {
        fprintf(fp, "// %c %s\n", i == j ? '!' : ' ', data->SuperClasses[j]);
      }

      output_DummyInitFunction(fp, fileInfo->FileName);
      fclose(fp);
      exit(0);
    }
  }

  if (hierarchyInfo)
  {
    /* resolve using declarations within the header files */
    vtkWrap_ApplyUsingDeclarations(data, fileInfo, hierarchyInfo);

    /* expand typedefs */
    vtkWrap_ExpandTypedefs(data, fileInfo, hierarchyInfo);

    if (!vtkWrap_IsTypeOf(hierarchyInfo, data->Name, "vtkObjectBase"))
    {
      output_DummyInitFunction(fp, fileInfo->FileName);
      fclose(fp);
      exit(0);
    }
  }

  fprintf(fp, "// ClientServer wrapper for %s object\n//\n", data->Name);
  fprintf(fp, "#define VTK_WRAPPING_CXX\n");
  if (strcmp("vtkObjectBase", data->Name) != 0)
  {
    /* Block inclusion of full streams. */
    fprintf(fp, "#define VTK_STREAMS_FWD_ONLY\n");
  }
  fprintf(fp, "#include \"%s.h\"\n", data->Name);
  fprintf(fp, "#include \"vtkSystemIncludes.h\"\n");
  if (classUsesStdString(data))
  {
    fprintf(fp, "#include \"vtkStdString.h\"\n");
  }
  fprintf(fp, "#include \"vtkClientServerInterpreter.h\"\n");
  fprintf(fp, "#include \"vtkClientServerStream.h\"\n\n");
#if 0
  if (!strcmp("vtkObject",data->Name))
    {
    fprintf(fp,"#include \"vtkClientServerProgressObserver.h\"\n\n");
    }
#endif
  if (!strcmp("vtkObjectBase", data->Name))
  {
    fprintf(fp, "#include <sstream>\n");
  }
  if (*nsname)
  {
    fprintf(fp, "using namespace %s;\n", nsname);
  }
  if (!data->IsAbstract)
  {
    fprintf(fp, "\nvtkObjectBase *%sClientServerNewCommand(void* /*ctx*/)\n{\n", data->Name);
    fprintf(fp, "  return %s::New();\n}\n\n", data->Name);
  }

  fprintf(fp, "\n"
              "int VTK_EXPORT"
              " %sCommand(vtkClientServerInterpreter *arlu, vtkObjectBase *ob,"
              " const char *method, const vtkClientServerStream& msg,"
              " vtkClientServerStream& resultStream, void* /*ctx*/)\n"
              "{\n",
    data->Name);

  if (strcmp(data->Name, "vtkObjectBase") == 0)
  {
    fprintf(fp, "  %s *op = ob;\n", data->Name);
  }
  else
  {
    fprintf(fp, "  %s *op = %s::SafeDownCast(ob);\n", data->Name, data->Name);
    fprintf(fp, "  if(!op)\n"
                "    {\n"
                "    vtkOStrStreamWrapper vtkmsg;\n"
                "    vtkmsg << \"Cannot cast \" << ob->GetClassName() << \" object to %s.  \"\n"
                "           << \"This probably means the class specifies the incorrect superclass "
                "in vtkTypeMacro.\";\n"
                "    resultStream.Reset();\n"
                "    resultStream << vtkClientServerStream::Error\n"
                "                 << vtkmsg.str() << 0 << vtkClientServerStream::End;\n"
                "    return 0;\n"
                "    }\n",
      data->Name);
  }

  fprintf(fp, "  (void)arlu;\n");

  /*fprintf(fp,"  vtkClientServerStream resultStream;\n");*/

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    currentFunction = data->Functions[i];
    outputFunction(fp, data);
  }

  /* try superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    fprintf(fp, "\n"
                "  {\n"
                "    const char* commandName = \"%s\";\n"
                "    if (arlu->HasCommandFunction(commandName) &&\n"
                "        arlu->CallCommandFunction(commandName, op, method, msg, resultStream)) { "
                "return 1; }\n"
                "  }\n",
      data->SuperClasses[i]);
  }
  /* Add the Print method to vtkObjectBase. */
  if (!strcmp("vtkObjectBase", data->Name))
  {
    fprintf(fp, "  if (!strcmp(\"Print\",method) && msg.GetNumberOfArguments(0) == 2)\n"
                "    {\n"
                "    std::ostringstream buf_with_warning_C4701;\n"
                "    op->Print(buf_with_warning_C4701);\n"
                "    resultStream.Reset();\n"
                "    resultStream << vtkClientServerStream::Reply\n"
                "                 << buf_with_warning_C4701.str().c_str()\n"
                "                 << vtkClientServerStream::End;\n"
                "    return 1;\n"
                "    }\n");
  }
  /* Add the special form of AddObserver to vtkObject. */
  if (!strcmp("vtkObject", data->Name))
  {
    fprintf(fp, "  if (!strcmp(\"AddObserver\",method) && msg.GetNumberOfArguments(0) == 4)\n"
                "    {\n"
                "    const char* event;\n"
                "    vtkClientServerStream css;\n"
                "    if(msg.GetArgument(0, 2, &event) && msg.GetArgument(0, 3, &css))\n"
                "      {\n"
                "      return arlu->NewObserver(op, event, css);\n"
                "      }\n"
                "    }\n");
  }
  fprintf(fp,
    "  if(resultStream.GetNumberOfMessages() > 0 &&\n"
    "     resultStream.GetCommand(0) == vtkClientServerStream::Error &&\n"
    "     resultStream.GetNumberOfArguments(0) > 1)\n"
    "    {\n"
    "    /* A superclass wrapper prepared a special message. */\n"
    "    return 0;\n"
    "    }\n"
    "  vtkOStrStreamWrapper vtkmsg;\n"
    "  vtkmsg << \"Object type: %s, could not find requested method: \\\"\"\n"
    "         << method << \"\\\"\\nor the method was called with incorrect arguments.\\n\";\n"
    "  resultStream.Reset();\n"
    "  resultStream << vtkClientServerStream::Error\n"
    "               << vtkmsg.str() << vtkClientServerStream::End;\n"
    "  vtkmsg.rdbuf()->freeze(0);\n",
    data->Name);
  fprintf(fp, "  return 0;\n"
              "}\n");

  classData = (NewClassInfo*)malloc(sizeof(NewClassInfo));
  getClassInfo(fileInfo, data, classData);
  output_InitFunction(fp, classData);
  free(classData);

  vtkParse_Free(fileInfo);
  fclose(fp);
  return 0;
}
