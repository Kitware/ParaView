// tcl wrapper for vtkKWCallbackSpecification object
//
#include "vtkSystemIncludes.h"
#include "vtkKWCallbackSpecification.h"

#include "vtkTclUtil.h"

ClientData vtkKWCallbackSpecificationNewCommand()
{
  vtkKWCallbackSpecification *temp = vtkKWCallbackSpecification::New();
  return ((ClientData)temp);
}

int vtkKWObjectCppCommand(vtkKWObject *op, Tcl_Interp *interp,
             int argc, char *argv[]);
int VTKTCL_EXPORT vtkKWCallbackSpecificationCppCommand(vtkKWCallbackSpecification *op, Tcl_Interp *interp,
             int argc, char *argv[]);

int VTKTCL_EXPORT vtkKWCallbackSpecificationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[])
{
  if ((argc == 2)&&(!strcmp("Delete",argv[1]))&& !vtkTclInDelete())
    {
    Tcl_DeleteCommand(interp,argv[0]);
    return TCL_OK;
    }
   return vtkKWCallbackSpecificationCppCommand((vtkKWCallbackSpecification *)cd,interp, argc, argv);
}

int VTKTCL_EXPORT vtkKWCallbackSpecificationCppCommand(vtkKWCallbackSpecification *op, Tcl_Interp *interp,
             int argc, char *argv[])
{
  int    tempi;
  double tempd;
  static char temps[80];
  int    error;

  error = 0; error = error;
  tempi = 0; tempi = tempi;
  tempd = 0; tempd = tempd;
  temps[0] = 0; temps[0] = temps[0];

  if (argc < 2)
    {
    Tcl_SetResult(interp, "Could not find requested method.", TCL_VOLATILE);
    return TCL_ERROR;
    }
  if (!interp)
    {
    if (!strcmp("DoTypecasting",argv[0]))
      {
      if (!strcmp("vtkKWCallbackSpecification",argv[1]))
        {
        argv[2] = (char *)((void *)op);
        return TCL_OK;
        }
      if (vtkKWObjectCppCommand((vtkKWObject *)op,interp,argc,argv) == TCL_OK)
        {
        return TCL_OK;
        }
      }
    return TCL_ERROR;
    }

  if (!strcmp("GetSuperClassName",argv[1]))
    {
    Tcl_SetResult(interp,"vtkKWObject", TCL_VOLATILE);
    return TCL_OK;
    }

  if ((!strcmp("New",argv[1]))&&(argc == 2))
    {
    vtkKWCallbackSpecification  *temp20;
    int vtkKWCallbackSpecificationCommand(ClientData, Tcl_Interp *, int, char *[]);
    error = 0;

    if (!error)
      {
      temp20 = (op)->New();
      vtkTclGetObjectFromPointer(interp,(void *)temp20,vtkKWCallbackSpecificationCommand);
      return TCL_OK;
      }
    }
  if ((!strcmp("GetClassName",argv[1]))&&(argc == 2))
    {
    const char    *temp20;
    error = 0;

    if (!error)
      {
      temp20 = (op)->GetClassName();
      if (temp20)
        {
        Tcl_SetResult(interp, (char*)temp20, TCL_VOLATILE);
        }
      else
        {
        Tcl_ResetResult(interp);
        }
      return TCL_OK;
      }
    }
  if ((!strcmp("IsA",argv[1]))&&(argc == 3))
    {
    char    *temp0;
    int      temp20;
    error = 0;

    temp0 = argv[2];
    if (!error)
      {
      temp20 = (op)->IsA(temp0);
      char tempResult[1024];
      sprintf(tempResult,"%i",temp20);
      Tcl_SetResult(interp, tempResult, TCL_VOLATILE);
      return TCL_OK;
      }
    }
  if ((!strcmp("SetEventString",argv[1]))&&(argc == 3))
    {
    char    *temp0;
    error = 0;

    temp0 = argv[2];
    if (!error)
      {
      op->SetEventString(temp0);
      Tcl_ResetResult(interp);
      return TCL_OK;
      }
    }
  if ((!strcmp("GetEventString",argv[1]))&&(argc == 2))
    {
    char    *temp20;
    error = 0;

    if (!error)
      {
      temp20 = (op)->GetEventString();
      if (temp20)
        {
        Tcl_SetResult(interp, (char*)temp20, TCL_VOLATILE);
        }
      else
        {
        Tcl_ResetResult(interp);
        }
      return TCL_OK;
      }
    }
  if ((!strcmp("SetCommandString",argv[1]))&&(argc == 3))
    {
    char    *temp0;
    error = 0;

    temp0 = argv[2];
    if (!error)
      {
      op->SetCommandString(temp0);
      Tcl_ResetResult(interp);
      return TCL_OK;
      }
    }
  if ((!strcmp("GetCommandString",argv[1]))&&(argc == 2))
    {
    char    *temp20;
    error = 0;

    if (!error)
      {
      temp20 = (op)->GetCommandString();
      if (temp20)
        {
        Tcl_SetResult(interp, (char*)temp20, TCL_VOLATILE);
        }
      else
        {
        Tcl_ResetResult(interp);
        }
      return TCL_OK;
      }
    }
  if ((!strcmp("SetCalledObject",argv[1]))&&(argc == 3))
    {
    vtkKWObject  *temp0;
    error = 0;

    temp0 = (vtkKWObject *)(vtkTclGetPointerFromObject(argv[2],"vtkKWObject",interp,error));
    if (!error)
      {
      op->SetCalledObject(temp0);
      Tcl_ResetResult(interp);
      return TCL_OK;
      }
    }
  if ((!strcmp("GetCalledObject",argv[1]))&&(argc == 2))
    {
    vtkKWObject  *temp20;
    int vtkKWObjectCommand(ClientData, Tcl_Interp *, int, char *[]);
    error = 0;

    if (!error)
      {
      temp20 = (op)->GetCalledObject();
      vtkTclGetObjectFromPointer(interp,(void *)temp20,vtkKWObjectCommand);
      return TCL_OK;
      }
    }
  if ((!strcmp("SetWindow",argv[1]))&&(argc == 3))
    {
    vtkKWWindow  *temp0;
    error = 0;

    temp0 = (vtkKWWindow *)(vtkTclGetPointerFromObject(argv[2],"vtkKWWindow",interp,error));
    if (!error)
      {
      op->SetWindow(temp0);
      Tcl_ResetResult(interp);
      return TCL_OK;
      }
    }
  if ((!strcmp("GetWindow",argv[1]))&&(argc == 2))
    {
    vtkKWWindow  *temp20;
    int vtkKWWindowCommand(ClientData, Tcl_Interp *, int, char *[]);
    error = 0;

    if (!error)
      {
      temp20 = (op)->GetWindow();
      vtkTclGetObjectFromPointer(interp,(void *)temp20,vtkKWWindowCommand);
      return TCL_OK;
      }
    }
  if ((!strcmp("SetNextCallback",argv[1]))&&(argc == 3))
    {
    vtkKWCallbackSpecification  *temp0;
    error = 0;

    temp0 = (vtkKWCallbackSpecification *)(vtkTclGetPointerFromObject(argv[2],"vtkKWCallbackSpecification",interp,error));
    if (!error)
      {
      op->SetNextCallback(temp0);
      Tcl_ResetResult(interp);
      return TCL_OK;
      }
    }
  if ((!strcmp("GetNextCallback",argv[1]))&&(argc == 2))
    {
    vtkKWCallbackSpecification  *temp20;
    int vtkKWCallbackSpecificationCommand(ClientData, Tcl_Interp *, int, char *[]);
    error = 0;

    if (!error)
      {
      temp20 = (op)->GetNextCallback();
      vtkTclGetObjectFromPointer(interp,(void *)temp20,vtkKWCallbackSpecificationCommand);
      return TCL_OK;
      }
    }

  if (!strcmp("ListInstances",argv[1]))
    {
    vtkTclListInstances(interp,(ClientData)vtkKWCallbackSpecificationCommand);
    return TCL_OK;
    }

  if (!strcmp("ListMethods",argv[1]))
    {
    vtkKWObjectCppCommand(op,interp,argc,argv);
    Tcl_AppendResult(interp,"Methods from vtkKWCallbackSpecification:\n",NULL);
    Tcl_AppendResult(interp,"  GetSuperClassName\n",NULL);
    Tcl_AppendResult(interp,"  New\n",NULL);
    Tcl_AppendResult(interp,"  GetClassName\n",NULL);
    Tcl_AppendResult(interp,"  IsA\t with 1 arg\n",NULL);
    Tcl_AppendResult(interp,"  SetEventString\t with 1 arg\n",NULL);
    Tcl_AppendResult(interp,"  GetEventString\n",NULL);
    Tcl_AppendResult(interp,"  SetCommandString\t with 1 arg\n",NULL);
    Tcl_AppendResult(interp,"  GetCommandString\n",NULL);
    Tcl_AppendResult(interp,"  SetCalledObject\t with 1 arg\n",NULL);
    Tcl_AppendResult(interp,"  GetCalledObject\n",NULL);
    Tcl_AppendResult(interp,"  SetWindow\t with 1 arg\n",NULL);
    Tcl_AppendResult(interp,"  GetWindow\n",NULL);
    Tcl_AppendResult(interp,"  SetNextCallback\t with 1 arg\n",NULL);
    Tcl_AppendResult(interp,"  GetNextCallback\n",NULL);
    return TCL_OK;
    }

  if (vtkKWObjectCppCommand((vtkKWObject *)op,interp,argc,argv) == TCL_OK)
    {
    return TCL_OK;
    }

  if ((argc >= 2)&&(!strstr(interp->result,"Object named:")))
    {
    char temps2[256];
    sprintf(temps2,"Object named: %s, could not find requested method: %s\nor the method was called with incorrect arguments.\n",argv[0],argv[1]);
    Tcl_AppendResult(interp,temps2,NULL);
    }
  return TCL_ERROR;
}
