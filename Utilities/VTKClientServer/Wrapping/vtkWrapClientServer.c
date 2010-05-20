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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vtkParse.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;

int arg_is_pointer_to_data(int aType, int count)
{
  return
    (count == 0 &&
     (aType % 0x1000)/0x100 == 3 && /* T*   */
     (aType % 0x100) != 0x02 && /* void*    */
     (aType % 0x100) != 0x03 && /* char*    */
     (aType % 0x100) != 0x08 && /* Foo*  */
     (aType % 0x100) != 0x09 /* vtkFoo*  */);
}

void output_temp(FILE *fp, int i, int aType, char *Id, int count)
{
  /* Store whether this is pointer to data.  */
  int isPointerToData = i != MAX_ARGS && arg_is_pointer_to_data(aType, count);

  /* ignore void */
  if (((aType % 0x10) == 0x2)&&(!((aType % 0x1000)/0x100)))
    {
    return;
    }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) && (aType % 0x2000 >= 0x1000))
    {
    fprintf(fp,"    const ");
    }
  else
    {
    fprintf(fp,"    ");
    }

  /* Handle some objects of known type.  */
  if(((aType % 0x1000) == 0x109 || (aType % 0x1000) == 0x309) &&
     strcmp(Id, "vtkClientServerStream") == 0)
    {
    fprintf(fp, "vtkClientServerStream temp%i_inst, *temp%i = &temp%i_inst;\n",
            i, i, i);
    return;
    }

  /* Start pointer-to-data arguments.  */
  if(isPointerToData)
    {
    fprintf(fp, "vtkClientServerStreamDataArg<");
    }

  if ((aType % 0x100)/0x10 == 1)
    {
    fprintf(fp,"unsigned ");
    }

  switch (aType % 0x10)
    {
    case 0x1:   fprintf(fp,"float  "); break;
    case 0x7:   fprintf(fp,"double "); break;
    case 0x4:   fprintf(fp,"int    "); break;
    case 0x5:   fprintf(fp,"short  "); break;
    case 0x6:   fprintf(fp,"long   "); break;
    case 0x2:     fprintf(fp,"void   "); break;
    case 0x3:     fprintf(fp,"char   "); break;
    case 0xA:     fprintf(fp,"vtkIdType "); break;
    case 0xB:     fprintf(fp,"long long "); break;
    case 0xC:     fprintf(fp,"__int64 "); break;
    case 0xD:     fprintf(fp,"signed char "); break;
    case 0xE:     fprintf(fp,"bool "); break;
    case 0x9:     fprintf(fp,"%s ",Id); break;
    case 0x8: return;
    }

  /* Finish pointer-to-data arguments.  */
  if(isPointerToData)
    {
    fprintf(fp, "> temp%i(msg, 0, %i);\n", i, i+2);
    return;
    }

  /* handle array arguements */
  if (count > 1)
    {
    fprintf(fp,"temp%i[%i];\n",i,count);
    return;
    }

  switch ((aType % 0x1000)/0x100)
    {
    case 0x1: fprintf(fp, " *"); break; /* act " &" */
    case 0x2: fprintf(fp, "&&"); break;
    case 0x3: fprintf(fp, " *"); break;
    case 0x4: fprintf(fp, "&*"); break;
    case 0x5: fprintf(fp, "*&"); break;
    case 0x7: fprintf(fp, "**"); break;
    default: fprintf(fp,"  "); break;
    }

  fprintf(fp,"temp%i",i);
  fprintf(fp,";\n");
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  /* use the hint */
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301: case 0x307:
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B: case 0x30C:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A: case 0x31B:
    case 0x31C: case 0x30D:
      fprintf(fp,
              "      resultStream.Reset();\n"
              "      resultStream << vtkClientServerStream::Reply << vtkClientServerStream::InsertArray(temp%i,%i) << vtkClientServerStream::End;\n", MAX_ARGS, currentFunction->HintSize);
      break;
    }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x2:
      break;
    case 0x1: case 0x3: case 0x4: case 0x5: case 0x6: case 0x7: case 0xA:
    case 0xB: case 0xC: case 0xD: case 0xE: case 0x13: case 0x14: case 0x15: case 0x16:
    case 0x1A: case 0x1B: case 0x1C:
    case 0x303:
      fprintf(fp,
              "      resultStream.Reset();\n"
              "      resultStream << vtkClientServerStream::Reply << temp%i << vtkClientServerStream::End;\n",
              MAX_ARGS);
      break;
    case 0x109:
    case 0x309:
      /* Handle some objects of known type.  */
      if(strcmp(currentFunction->ReturnClass, "vtkClientServerStream") == 0)
        {
        fprintf(fp,
                "      resultStream.Reset();\n"
                "      resultStream << vtkClientServerStream::Reply << *temp%i << vtkClientServerStream::End;\n",
                MAX_ARGS);
        }
      else
        {
        fprintf(fp,
                "      resultStream.Reset();\n"
                "      resultStream << vtkClientServerStream::Reply << (vtkObjectBase *)temp%i << vtkClientServerStream::End;\n",MAX_ARGS);
        }
      break;

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 0x301: case 0x307:
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B: case 0x30C: case 0x30D:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A: case 0x31B:
    case 0x31C:
      use_hints(fp);
      break;
    case 0x9:
      {
      /* Handle some objects of known type.  */
      if(strcmp(currentFunction->ReturnClass, "vtkClientServerStream") == 0)
        {
        fprintf(fp,
                "      resultStream.Reset();\n"
                "      resultStream << vtkClientServerStream::Reply << temp%i << vtkClientServerStream::End;\n",
                MAX_ARGS);
        break;
        }
      }
    default:
      fprintf(fp,
              "      resultStream.Reset();\n"
              "      resultStream << vtkClientServerStream::Reply\n"
              "                   << \"unable to return result of type(%d %% 0x1000 = %d).\"\n"
              "                   << vtkClientServerStream::End;\n",
              currentFunction->ReturnType, currentFunction->ReturnType % 0x1000);
      break;
    }
}

void get_args(FILE *fp, int i)
{
  int j;
  int start_arg = 2;

  /* what arg do we start with */
  for (j = 0; j < i; j++)
    {
    start_arg = start_arg +
      (currentFunction->ArgCounts[j] ? currentFunction->ArgCounts[j] : 1);
    }

  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 0x10) == 0x2)&&
      (!((currentFunction->ArgTypes[i] % 0x1000)/0x100)))
    {
    return;
    }

  switch (currentFunction->ArgTypes[i] % 0x1000)
    {
    case 0x1:
    case 0x7:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0x3:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x1A:
    case 0x303:
      fprintf(fp, "msg.GetArgument(0, %i, &temp%i)", i+2, i);
      break;
    case 0x109:
    case 0x309:
      /* Handle some objects of known type.  */
      if(strcmp(currentFunction->ArgClasses[i], "vtkClientServerStream") == 0)
        {
        fprintf(fp,
                "msg.GetArgument(0, %i, temp%i)", i+2, i);
        }
      else
        {
        fprintf(fp,
                "vtkClientServerStreamGetArgumentObject(msg, 0, %i, &temp%i, \"%s\")",
                i+2, i, currentFunction->ArgClasses[i]);
        }
      break;
    case 0x2:
    case 0x9:
      break;
    default:
      if (currentFunction->ArgCounts[i] > 1)
        {
        switch (currentFunction->ArgTypes[i] % 0x100)
          {
          case 0x1: case 0x7:
          case 0x4: case 0x5: case 0x6: case 0xA: case 0xB: case 0xC: case 0xD: case 0xE:
          case 0x13: case 0x14: case 0x15: case 0x16:
          case 0x1A: case 0x1B: case 0x1C:
            fprintf(fp, "msg.GetArgument(0, %i, temp%i, %i)",
                    i+2, i, currentFunction->ArgCounts[i]);
            break;
          }
        }
      else if(arg_is_pointer_to_data(currentFunction->ArgTypes[i],
                                     currentFunction->ArgCounts[i]))
        {
        /* Pointer-to-data arguments are handled by an object
           convertible to bool.  */
        fprintf(fp, "temp%i", i);
        }

    }
}

void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator ||
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name)
    {
    return;
    }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    int isPointerToData =
      arg_is_pointer_to_data(currentFunction->ArgTypes[i],
                             currentFunction->ArgCounts[i]);
    if ((currentFunction->ArgTypes[i] % 0x10) == 0x8) args_ok = 0;
    /* if its a pointer arg make sure we have the ArgCount */
    if ((currentFunction->ArgTypes[i] % 0x1000 >= 0x100) &&
        !isPointerToData &&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x303)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x309)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x109))
      {
      if (currentFunction->NumberOfArguments > 1 ||
          !currentFunction->ArgCounts[i])
        {
        args_ok = 0;
        }
      }
    if ((currentFunction->ArgTypes[i] % 0x100 >= 0x10)&&
        (currentFunction->ArgTypes[i] != 0x13)&&
        (currentFunction->ArgTypes[i] != 0x14)&&
        (currentFunction->ArgTypes[i] != 0x15)&&
        (currentFunction->ArgTypes[i] != 0x16)&&
        (currentFunction->ArgTypes[i] != 0x1A)&&
        !isPointerToData)
      {
      args_ok = 0;
      }
    }

  /* if it returns an unknown class we cannot wrap it */
  if ((currentFunction->ReturnType % 0x10) == 0x8)
    {
    args_ok = 0;
    }

  if (((currentFunction->ReturnType % 0x1000)/0x100 != 0x3)&&
      ((currentFunction->ReturnType % 0x1000)/0x100 != 0x1)&&
      ((currentFunction->ReturnType % 0x1000)/0x100))
    {
    args_ok = 0;
    }
  if (currentFunction->NumberOfArguments &&
      (currentFunction->ArgTypes[0] == 0x5000))
    {
    args_ok = 0;
    }

  /* we can't handle void * return types */
  if ((currentFunction->ReturnType % 0x1000) == 0x302)
    {
    args_ok = 0;
    }

  /* watch out for functions that dont have enough info */
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301: case 0x307:
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B:
    case 0x30C: case 0x30D:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A:
    case 0x31B: case 0x31C:
      args_ok = currentFunction->HaveHint;
      break;
    }

  /* if the args are OK and it is not a constructor or destructor */
  if (args_ok &&
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName,currentFunction->Name + 1))
    {
    if(currentFunction->IsLegacy)
      {
      fprintf(fp,"#if !defined(VTK_LEGACY_REMOVE)\n");
      }
    fprintf(fp,"  if (!strcmp(\"%s\",method) && msg.GetNumberOfArguments(0) == %i)\n",
            currentFunction->Name, currentFunction->NumberOfArguments+2);
    fprintf(fp, "    {\n");

    /* process the args */
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      output_temp(fp, i, currentFunction->ArgTypes[i],
                  currentFunction->ArgClasses[i],
                  currentFunction->ArgCounts[i]);
      }
    output_temp(fp, MAX_ARGS,currentFunction->ReturnType,
                currentFunction->ReturnClass, 0);

    if(currentFunction->NumberOfArguments > 0)
      {
      const char* amps = "    if(";
      /* now get the required args from the stack */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
        {
        fprintf(fp, "%s", amps);
        amps = " &&\n      ";
        get_args(fp,i);
        }
      fprintf(fp, ")\n");
      }
    fprintf(fp, "      {\n");

    switch (currentFunction->ReturnType % 0x1000)
      {
      case 0x2:
        fprintf(fp,"      op->%s(",currentFunction->Name);
        break;
      case 0x109:
        fprintf(fp,"      temp%i = &(op)->%s(",MAX_ARGS,currentFunction->Name);
        break;
      default:
        fprintf(fp,"      temp%i = (op)->%s(",MAX_ARGS,currentFunction->Name);
      }
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      if (i)
        {
        fprintf(fp,",");
        }
      if ((currentFunction->ArgTypes[i] % 0x1000) == 0x109)
        {
        fprintf(fp,"*(temp%i)",i);
        }
      else
        {
        fprintf(fp,"temp%i",i);
        }
      }
    fprintf(fp,");\n");
    return_result(fp);
    fprintf(fp,"      return 1;\n");
    fprintf(fp,"      }\n");
    fprintf(fp,"    }\n");
    if(currentFunction->IsLegacy)
      {
      fprintf(fp,"#endif\n");
      }

    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
    }

#if 0
  if (!strcmp("vtkObject",data->ClassName))
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
  char *Name;
  int TotalPolymorphTypes;
  FunctionInfo Function[20];
} UniqueFunctionInfo;


//--------------------------------------------------------------------------nix
/*
 * This structure is used to collect and hold class information. It is a
 * modified version of FileInfo
 *
 */
typedef struct _ClassInfo
{
  int   HasDelete;
  int   IsAbstract;
  int   IsConcrete;
  char *ClassName;
  char *FileName;
  char *OutputFileName;
  char *SuperClasses[10];
  int   NumberOfSuperClasses;
  int   NumberOfFunctions;
  UniqueFunctionInfo Functions[1000];
  char *NameComment;
  char *Description;
  char *Caveats;
  char *SeeAlso;
} ClassInfo;

//--------------------------------------------------------------------------nix
/* 
 * This function makes writes a function which makes its own meta-infomation
 * available to the caller.
 * 
 * @param fp Wrapper file to be written
 * @param data The data which is written
 */
void outputMetaInfoExtractFunction(FILE *fp, ClassInfo *data)
{
  int i,j,k;
  fprintf(fp,
          "//-------------------------------------------------------------------------auto\n"
          "/*\n"
          " * %sMetaInfo function creates meta-information each time\n"
          " * it gets called. This can be used for any Meta Object Protocol needs.\n"
          " *\n"
          " * @return fileinfo structure with class metadata\n"
          " */\n"
          "ClassInfo& %sMetaInfo()\n"
          "{\n"
          "  static bool once = 1;\n"
          "  static ClassInfo data;\n"
          "  if(once)\n"
          "    {\n"
          "    once = 0;\n"
          "\n"
          "    data.HasDelete  = %d;\n"
          "    data.IsAbstract = %d;\n"
          "    data.IsConcrete = %d;\n"
          "    data.ClassName      = \"%s\";\n"
          "    data.FileName       = \"%s\";\n"
          "    data.OutputFileName = \"%s\";\n"
          "      {\n",
          data->ClassName,
          data->ClassName,
          data->HasDelete,
          data->IsAbstract,
          data->IsConcrete,
          data->ClassName,
          data->FileName,
          data->OutputFileName
          );
  for(i=0; i < data->NumberOfSuperClasses;i++)
    {
    fprintf(fp,
            "            data.SuperClasses[%d]= \"%s\";\n",
            i,data->SuperClasses[i]
            );
        
    }
  fprintf(fp,
          "      }\n"
          "    data.NumberOfSuperClasses = %d;\n"
          "    data.NumberOfFunctions = %d;\n"
          "      {\n",
          data->NumberOfSuperClasses,
          data->NumberOfFunctions
          );

  for(i=0; i< data->NumberOfFunctions;i++)
    {
    fprintf(fp,
            "      // %s\n"
            "      data.Functions[%d].Name = \"%s\";\n"
            "      data.Functions[%d].TotalPolymorphTypes = %d;\n"
            "        {\n",
            data->Functions[i].Name,
            i,data->Functions[i].Name,
            i,data->Functions[i].TotalPolymorphTypes
            );
    for(j=0;j<data->Functions[i].TotalPolymorphTypes;++j)
      {
      fprintf(fp,
              "        //____________________________\n"
              "        data.Functions[%d].Function[%d].Name = \"%s\";\n"
              "        data.Functions[%d].Function[%d].NumberOfArguments = %d ;\n"
              "        data.Functions[%d].Function[%d].ArrayFailure = %d;\n"
              "        data.Functions[%d].Function[%d].IsPureVirtual = %d;\n"
              "        data.Functions[%d].Function[%d].IsPublic = %d;\n"
              "        data.Functions[%d].Function[%d].IsProtected = %d;\n"
              "        data.Functions[%d].Function[%d].IsOperator = %d;\n"
              "        data.Functions[%d].Function[%d].HaveHint = %d;\n"
              "        data.Functions[%d].Function[%d].HintSize = %d;\n"
              "          {\n",

              i,j,data->Functions[i].Function[j].Name,
              i,j,data->Functions[i].Function[j].NumberOfArguments,
              i,j,data->Functions[i].Function[j].ArrayFailure,
              i,j,data->Functions[i].Function[j].IsPureVirtual,
              i,j,data->Functions[i].Function[j].IsPublic,
              i,j,data->Functions[i].Function[j].IsProtected,
              i,j,data->Functions[i].Function[j].IsOperator,
              i,j,data->Functions[i].Function[j].HaveHint,
              i,j,data->Functions[i].Function[j].HintSize
              );
      for(k=0;k<data->Functions[i].Function[j].NumberOfArguments;k++)
        {
        fprintf(fp,
                "          data.Functions[%d].Function[%d].ArgTypes[%d]   = 0x%x;\n"
                "          data.Functions[%d].Function[%d].ArgCounts[%d]  = %d;\n"
                "          data.Functions[%d].Function[%d].ArgClasses[%d] = \"%s\";\n",
                i,j,k,data->Functions[i].Function[j].ArgTypes[k],
                i,j,k,data->Functions[i].Function[j].ArgCounts[k],
                i,j,k,data->Functions[i].Function[j].ArgClasses[k]
                );
        }
      fprintf(fp,
              "          }\n"
              "        data.Functions[%d].Function[%d].ReturnType  = 0x%x;\n"
              "        data.Functions[%d].Function[%d].ReturnClass = \"%s\";\n"
              //"              data.Functions[%d].Function[%d].Comment     = \"%s\";\n"
              "        data.Functions[%d].Function[%d].Signature   = \"%s\";\n"
              "        data.Functions[%d].Function[%d].IsLegacy    = %d;\n",
              i,j,data->Functions[i].Function[j].ReturnType,
              i,j,data->Functions[i].Function[j].ReturnClass,
              //                i,data->Functions[i].Function[j].Comment,
              i,j,data->Functions[i].Function[j].Signature,
              i,j,data->Functions[i].Function[j].IsLegacy
              );
      }
    fprintf(fp,
            "        }\n"
            );
    }
  fprintf(fp,
          "      }\n"
          //"        data.NameComment = \"%s\";\n"
          //"        data.Description = \"%s\";\n"
          //"        data.Caveats     = \"%s\";\n"
          //"        data.SeeAlso     = \"%s\";\n"
          "    }\n"
          "  return data;\n"
          "}\n\n"//,
          //            data->NameComment,
          //            data->Description,
          //            data->Caveats,
          //            data->SeeAlso
          );
}

//--------------------------------------------------------------------------nix
/*
 * notWrappable returns true if the current-function is not wrappable.
 * 
 * @param currentFunction function-info being worked on
 * 
 * @return true if the function is not wrappable
 */
int notWrappable(FunctionInfo *currentFunction)
{
  return (currentFunction->IsOperator ||
          currentFunction->ArrayFailure ||
          !currentFunction->IsPublic ||
          !currentFunction->Name);
}

//--------------------------------------------------------------------------nix
/*
 * managableArguments check if the functions arguments are in a form
 * which can easily be used for automatic wrapper generation.
 * 
 * @param currentFunction the function beign worked on
 * 
 * @return true if the arguments are okay for code generation
 */
int managableArguments(FunctionInfo *currentFunction)
{
  int args_ok = 1;
  int i;
  
  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    int isPointerToData = arg_is_pointer_to_data(currentFunction->ArgTypes[i],
                                                 currentFunction->ArgCounts[i]);
    if ((currentFunction->ArgTypes[i] % 0x10) == 0x8)
      args_ok = 0;

    /* if its a pointer arg make sure we have the ArgCount */
    if ((currentFunction->ArgTypes[i] % 0x1000 >= 0x100) &&
        !isPointerToData &&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x303)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x309)&&
        (currentFunction->ArgTypes[i] % 0x1000 != 0x109))
      {
      if (currentFunction->NumberOfArguments > 1 ||
          !currentFunction->ArgCounts[i])
        {
        args_ok = 0;
        }
      }
    if ((currentFunction->ArgTypes[i] % 0x100 >= 0x10)&&
        (currentFunction->ArgTypes[i] != 0x13)&&
        (currentFunction->ArgTypes[i] != 0x14)&&
        (currentFunction->ArgTypes[i] != 0x15)&&
        (currentFunction->ArgTypes[i] != 0x16)&&
        (currentFunction->ArgTypes[i] != 0x1A)&&
        !isPointerToData)
      {
      args_ok = 0;
      }
    }
  
  /* if it returns an unknown class we cannot wrap it */
  if ((currentFunction->ReturnType % 0x10) == 0x8)
    {
    args_ok = 0;
    }
  
  if (((currentFunction->ReturnType % 0x1000)/0x100 != 0x3)&&
      ((currentFunction->ReturnType % 0x1000)/0x100 != 0x1)&&
      ((currentFunction->ReturnType % 0x1000)/0x100))
    {
    args_ok = 0;
    }
  if (currentFunction->NumberOfArguments &&
      (currentFunction->ArgTypes[0] == 0x5000))
    {
    args_ok = 0;
    }

  /* we can't handle void * return types */
  if ((currentFunction->ReturnType % 0x1000) == 0x302)
    {
    args_ok = 0;
    }

  /* watch out for functions that dont have enough info */
  switch (currentFunction->ReturnType % 0x1000)
    {
    case 0x301: case 0x307:
    case 0x304: case 0x305: case 0x306: case 0x30A: case 0x30B:
    case 0x30C: case 0x30D:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A:
    case 0x31B: case 0x31C:
      args_ok = currentFunction->HaveHint;
      break;
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

static int funCmp(const void  *fun1, const void *fun2)
{
  FunctionInfo *a = (FunctionInfo*)fun1;
  FunctionInfo *b = (FunctionInfo*)fun2;
  return strcmp(a->Name,b->Name);
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
int extractWrappable(FunctionInfo* from,
                     int fromSize,
                     FunctionInfo* to,
                     char* ClassName)
{
  int i,j;
  for (i=0,j=0 ; i<fromSize ;i++)
    {
    /* if the function is wrappable and
       args are OK and
       it is not a constructor or destructor */
    if(!notWrappable(&(from[i])) &&
       managableArguments(&(from[i])) &&
       strcmp(ClassName,from[i].Name) && strcmp(ClassName,from[i].Name + 1))
      {
      to[j] =  from[i];
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
int collectUniqueFunctionInfo(FunctionInfo *src, int srcSize, UniqueFunctionInfo *dest)
{
  int i,j,k;
  i=0;
  j=0;
  for (i=0; i < srcSize; ++i)
    {
    dest[i].Name = src[i].Name;
    dest[i].TotalPolymorphTypes = 1;
    dest[i].Function[0]= src[i];
    for(j=i+1; j<srcSize;j++)
      {
      if(funCmp(&dest[i],&src[j])==0)
        {
        //printf("%d [%d]> dest = %s :: src = %s\n",i,srcSize, dest[i].Name,src[j].Name);
        //printf("    > [SAME]\n");
        dest[i].Function[dest[i].TotalPolymorphTypes]=src[j];
        ++dest[i].TotalPolymorphTypes;
        for(k=j; k<srcSize-1;k++)
          {
          //printf("remaining = %s\n",src[k].Name);
          src[k]=src[k+1];
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
 * FileInfo format to ClassInfo format. The classInfo format combines
 * polymorphed functions into UniqueFunctionInfo internal format.
 * 
 * @param data hold the collected file data form the parser
 * @param classData the form into which we want to convert
 */
void getClassInfo(FileInfo *data, ClassInfo* classData)
{
  int i;
  int TotalUniqueFunctions=0;
  int TotalFunctions;
  FunctionInfo *tempFun =(FunctionInfo*) malloc
    (sizeof(FunctionInfo)*data->NumberOfFunctions);

  /* Collect all the information */
  classData->HasDelete = data->HasDelete;
  classData->IsAbstract = data->IsAbstract;
  classData->IsConcrete = data->IsConcrete;
  classData->ClassName = data->ClassName;
  classData->FileName = data->FileName;
  classData->OutputFileName = data->OutputFileName;
  classData->NumberOfSuperClasses = data->NumberOfSuperClasses;
  for (i = 0; i < data->NumberOfSuperClasses; ++i)
    {
    classData->SuperClasses[i] = data->SuperClasses[i];      
    }
  classData->NameComment = data->NameComment;
  classData->Description = data->Description;
  classData->Caveats = data->Caveats;
  classData->SeeAlso = data->SeeAlso;

  /* Collect only wrappable functions*/
  TotalFunctions = extractWrappable(&data->Functions[0],
                                    data->NumberOfFunctions,
                                    tempFun,
                                    data->ClassName);
  /* Sort the function data. */
  //qsort(tempFun,TotalFunctions,sizeof(FunctionInfo),funCmp);
  
/*   for(i=0;i<TotalFunctions;i++) */
/*     { */
/*     printf("(funargs %s %d)\n",tempFun[i].Name,tempFun[i].NumberOfArguments); */
/*     } */

//  printf("START\n");  
  /* Collect unique and group polymorphed functions in UniqueFunctionInfo */
  TotalUniqueFunctions = collectUniqueFunctionInfo(tempFun,
                                                   TotalFunctions,
                                                   classData->Functions);
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
int isUniqueString(char* main, char *list[], int total)
{
  int i;
  for (i = 0; i < total; ++i)
    {
    if(strcmp(main,list[i])==0)
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
int uniqueClasses(char *classes[],int total,char *classSelfName)
{
  int i,j=0;
  char *temp[1000];
  char* current_class_name;
  for (i = total-1; i>=0; --i)
  {
    current_class_name = classes[i];
    if (strcmp(current_class_name,classSelfName)!=0  &&
       // hack
       strcmp(current_class_name,"vtkClientServerStream")!=0 &&
       isUniqueString(current_class_name,&temp[0],j)
       // && strcmp(classes[i],"vtkObjectBase")!=0)
       )
      {
      temp[j]= current_class_name;
      ++j;
      }
  }
  for(i=0;i<j;++i)
    {
    classes[i]=temp[i];
    }
  return j;
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
int extractOtherClassesUsed(ClassInfo *data, char * classes[])
{
  int i,j,k;
  int count=0;
  // Collect all the superclasses
  for ( i = 0; i < data->NumberOfSuperClasses; ++i)
    classes[i]=data->SuperClasses[i];
  count = data->NumberOfSuperClasses;
  // return count; // HACK only returns the super classes
  
  // Collect all the functions params and return types
  for ( i = 0; i < data->NumberOfFunctions; ++i)
    {
    for (j = 0; j < data->Functions[i].TotalPolymorphTypes; ++j)
      {
      for (k=0; k < data->Functions[i].Function[j].NumberOfArguments; ++k)
        {
        if(data->Functions[i].Function[j].ArgTypes[k] % 16 ==9)
          {
          if (data->Functions[i].Function[j].ArgExternals[k] == 0)
            {
            classes[count]=data->Functions[i].Function[j].ArgClasses[k];
            ++count;
            }
          }
        }
      if(data->Functions[i].Function[j].ReturnType % 16 == 9)
        {
        if (data->Functions[i].Function[j].ReturnExternal == 0)
          {
          classes[count]=data->Functions[i].Function[j].ReturnClass;
          ++count;
          }
        }
      }
    }
  return uniqueClasses(classes,count,data->ClassName);
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
void output_InitFunction(FILE *fp, ClassInfo *data)
{
  char* classes[1000];
  int totalClasses,i;
  totalClasses=  extractOtherClassesUsed(data,classes);
  fprintf(fp,"\n");
  for (i=0; i < totalClasses; ++i)
    {
    fprintf(fp,"void %s_Init(vtkClientServerInterpreter* csi);\n",classes[i]);
    }
  fprintf(fp,
          "\n"
          "//-------------------------------------------------------------------------auto\n"
          "void VTK_EXPORT %s_Init(vtkClientServerInterpreter* csi)\n"
          "{\n"
          "  static bool once;\n"
          "  if(!once)\n"
          "    {\n"
          "    once = true;\n", data->ClassName);
  for (i=0; i < totalClasses; ++i)
    fprintf(fp,"    %s_Init(csi);\n",classes[i]); 
  if(data->IsConcrete)
    fprintf(fp,"    csi->AddNewInstanceFunction(\"%s\", %sClientServerNewCommand);\n",
            data->ClassName,data->ClassName);
  fprintf(fp,"    csi->AddCommandFunction(\"%s\", %sCommand);\n",
          data->ClassName,data->ClassName);
  fprintf(fp, "    }\n}\n");
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  ClassInfo *classData;
  int i;

  fprintf(fp,"// ClientServer wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#define VTK_WRAPPING_CXX\n");
  if(strcmp("vtkObjectBase", data->ClassName) != 0)
    {
    /* Block inclusion of full streams. */
    fprintf(fp,"#define VTK_STREAMS_FWD_ONLY\n");
    }
  fprintf(fp,"#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp,"#include \"%s.h\"\n",data->ClassName);
  fprintf(fp,"#include \"vtkClientServerInterpreter.h\"\n");
  fprintf(fp,"#include \"vtkClientServerStream.h\"\n\n");
#if 0
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"#include \"vtkClientServerProgressObserver.h\"\n\n");
    }
#endif
  if (!strcmp("vtkObjectBase",data->ClassName))
    {
    fprintf(fp,"#include <vtksys/ios/sstream>\n");
    }
  if (data->IsConcrete)
    {
    fprintf(fp,"\nvtkObjectBase *%sClientServerNewCommand()\n{\n",data->ClassName);
    fprintf(fp,"  return %s::New();\n}\n\n",data->ClassName);
    }

  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
      {
      fprintf(fp,
              "int %sCommand(vtkClientServerInterpreter*, vtkObjectBase*,"
              " const char*, const vtkClientServerStream&,"
              " vtkClientServerStream& resultStream);\n",
              data->SuperClasses[i]);
      }
    }

  fprintf(fp,
          "\n"
          "int VTK_EXPORT"
          " %sCommand(vtkClientServerInterpreter *arlu, vtkObjectBase *ob,"
          " const char *method, const vtkClientServerStream& msg,"
          " vtkClientServerStream& resultStream)\n"
          "{\n",
          data->ClassName);

  if(strcmp(data->ClassName, "vtkObjectBase") == 0)
    {
    fprintf(fp,"  %s *op = ob;\n",
            data->ClassName);
    }
  else
    {
    fprintf(fp,"  %s *op = %s::SafeDownCast(ob);\n",
            data->ClassName, data->ClassName);
    fprintf(fp,
            "  if(!op)\n"
            "    {\n"
            "    vtkOStrStreamWrapper vtkmsg;\n"
            "    vtkmsg << \"Cannot cast \" << ob->GetClassName() << \" object to %s.  \"\n"
            "           << \"This probably means the class specifies the incorrect superclass in vtkTypeMacro.\";\n"
            "    resultStream.Reset();\n"
            "    resultStream << vtkClientServerStream::Error\n"
            "                 << vtkmsg.str() << 0 << vtkClientServerStream::End;\n"
            "    return 0;\n"
            "    }\n", data->ClassName);
    }

  fprintf(fp, "  (void)arlu;\n");


  /*fprintf(fp,"  vtkClientServerStream resultStream;\n");*/

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }

  /* try superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"\n  if (%sCommand(arlu, op,method,msg,resultStream))\n",
              data->SuperClasses[i]);
    fprintf(fp,"    {\n    return 1;\n    }\n");
    }
  /* Add the Print method to vtkObjectBase. */
  if (!strcmp("vtkObjectBase",data->ClassName))
    {
    fprintf(fp,
            "  if (!strcmp(\"Print\",method) && msg.GetNumberOfArguments(0) == 2)\n"
            "    {\n"
            "    vtksys_ios::ostringstream buf_with_warning_C4701;\n"
            "    op->Print(buf_with_warning_C4701);\n"
            "    resultStream.Reset();\n"
            "    resultStream << vtkClientServerStream::Reply\n"
            "                 << buf_with_warning_C4701.str().c_str()\n"
            "                 << vtkClientServerStream::End;\n"
            "    return 1;\n"
            "    }\n");
    }
  /* Add the special form of AddObserver to vtkObject. */
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,
            "  if (!strcmp(\"AddObserver\",method) && msg.GetNumberOfArguments(0) == 4)\n"
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
          data->ClassName);
  fprintf(fp,
          "  return 0;\n"
          "}\n");
  
  classData = (ClassInfo*)malloc(sizeof(ClassInfo));
  getClassInfo(data,classData);
  output_InitFunction(fp,classData);
  free(classData);
}

