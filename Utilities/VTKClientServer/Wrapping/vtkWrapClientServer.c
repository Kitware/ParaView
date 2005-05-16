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
#include "vtkParse.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;


void output_temp(FILE *fp, int i, int aType, char *Id, int count)
{
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
    case 0x9:     fprintf(fp,"%s ",Id); break;
    case 0x8: return;
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
    case 0x304: case 0x305: case 0x306: case 0x30A:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A:
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
    case 0x13: case 0x14: case 0x15: case 0x16: case 0x1A:
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
    case 0x304: case 0x305: case 0x306: case 0x30A:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A:
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
      fprintf(fp,
              "vtkClientServerStreamGetArgumentObject(msg, 0, %i, &temp%i, \"%s\")",
              i+2, i, currentFunction->ArgClasses[i]);
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
          case 0x4: case 0x5: case 0x6: case 0xA:
          case 0x13: case 0x14: case 0x15: case 0x16: case 0x1A:
            fprintf(fp, "msg.GetArgument(0, %i, temp%i, %i)",
                    i+2, i, currentFunction->ArgCounts[i]);
            break;
          }
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
    if ((currentFunction->ArgTypes[i] % 0x10) == 0x8) args_ok = 0;
    /* if its a pointer arg make sure we have the ArgCount */
    if ((currentFunction->ArgTypes[i] % 0x1000 >= 0x100) &&
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
        (currentFunction->ArgTypes[i] != 0x1A))
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
    case 0x304: case 0x305: case 0x306: case 0x30A:
    case 0x313: case 0x314: case 0x315: case 0x316: case 0x31A:
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
      if (currentFunction->ArgTypes[i] == 0x109)
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

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
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
            "           << \"This probably means the class specifies the incorrect superclass in vtkTypeRevisionMacro.\";\n"
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
            "    ostrstream buf_with_warning_C4701;\n"
            "    op->Print(buf_with_warning_C4701);\n"
            "    buf_with_warning_C4701.put('\\0');\n"
            "    resultStream.Reset();\n"
            "    resultStream << vtkClientServerStream::Reply\n"
            "                 << buf_with_warning_C4701.str()\n"
            "                 << vtkClientServerStream::End;\n"
            "    buf_with_warning_C4701.rdbuf()->freeze(0);\n"
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
}
