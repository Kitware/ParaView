#include "vtkTclUtil.h"
int vtkDummyRenderWindowInteractorCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkDummyRenderWindowInteractorNewCommand();
int vtkImageOutlineFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkImageOutlineFilterNewCommand();
int vtkInteractorStyleCameraCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStyleCameraNewCommand();
int vtkInteractorStyleGridExtentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStyleGridExtentNewCommand();
int vtkInteractorStyleImageExtentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStyleImageExtentNewCommand();
int vtkInteractorStylePlaneCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStylePlaneNewCommand();
int vtkInteractorStylePlaneSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStylePlaneSourceNewCommand();
int vtkInteractorStyleSphereCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStyleSphereNewCommand();
int vtkPVActorCompositeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVActorCompositeNewCommand();
int vtkPVApplicationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVApplicationNewCommand();
int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVDataNewCommand();
int vtkPVDataSetReaderInterfaceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVDataSetReaderInterfaceNewCommand();
int vtkPVEnSightReaderInterfaceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVEnSightReaderInterfaceNewCommand();
int vtkPVMethodInterfaceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVMethodInterfaceNewCommand();
int vtkPVRenderViewCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVRenderViewNewCommand();
int vtkPVSelectionListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSelectionListNewCommand();
int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceNewCommand();
int vtkPVSourceCollectionCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceCollectionNewCommand();
int vtkPVSourceInterfaceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceInterfaceNewCommand();
int vtkPVSourceListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceListNewCommand();
int vtkPVWindowCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVWindowNewCommand();
int vtkSimpleFieldDataToAttributeDataFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkSimpleFieldDataToAttributeDataFilterNewCommand();
int vtkSingleContourFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkSingleContourFilterNewCommand();
int vtkStringListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkStringListNewCommand();

extern Tcl_HashTable vtkInstanceLookup;
extern Tcl_HashTable vtkPointerLookup;
extern Tcl_HashTable vtkCommandLookup;
extern void vtkTclDeleteObjectFromHash(void *);
extern void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);


extern "C" {int VTK_EXPORT Vtkkwparaviewtcl_SafeInit(Tcl_Interp *interp);}



extern "C" {int VTK_EXPORT Vtkkwparaviewtcl_Init(Tcl_Interp *interp);}



extern void vtkTclGenericDeleteObject(ClientData cd);



int VTK_EXPORT Vtkkwparaviewtcl_SafeInit(Tcl_Interp *interp)
{
  return Vtkkwparaviewtcl_Init(interp);
}


int VTK_EXPORT Vtkkwparaviewtcl_Init(Tcl_Interp *interp)
{
  vtkTclCreateNew(interp,(char *) "vtkDummyRenderWindowInteractor", vtkDummyRenderWindowInteractorNewCommand,
                  vtkDummyRenderWindowInteractorCommand);
  vtkTclCreateNew(interp,(char *) "vtkImageOutlineFilter", vtkImageOutlineFilterNewCommand,
                  vtkImageOutlineFilterCommand);
  vtkTclCreateNew(interp,(char *) "vtkInteractorStyleCamera", vtkInteractorStyleCameraNewCommand,
                  vtkInteractorStyleCameraCommand);
  vtkTclCreateNew(interp,(char *) "vtkInteractorStyleGridExtent", vtkInteractorStyleGridExtentNewCommand,
                  vtkInteractorStyleGridExtentCommand);
  vtkTclCreateNew(interp,(char *) "vtkInteractorStyleImageExtent", vtkInteractorStyleImageExtentNewCommand,
                  vtkInteractorStyleImageExtentCommand);
  vtkTclCreateNew(interp,(char *) "vtkInteractorStylePlane", vtkInteractorStylePlaneNewCommand,
                  vtkInteractorStylePlaneCommand);
  vtkTclCreateNew(interp,(char *) "vtkInteractorStylePlaneSource", vtkInteractorStylePlaneSourceNewCommand,
                  vtkInteractorStylePlaneSourceCommand);
  vtkTclCreateNew(interp,(char *) "vtkInteractorStyleSphere", vtkInteractorStyleSphereNewCommand,
                  vtkInteractorStyleSphereCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVActorComposite", vtkPVActorCompositeNewCommand,
                  vtkPVActorCompositeCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVApplication", vtkPVApplicationNewCommand,
                  vtkPVApplicationCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVData", vtkPVDataNewCommand,
                  vtkPVDataCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVDataSetReaderInterface", vtkPVDataSetReaderInterfaceNewCommand,
                  vtkPVDataSetReaderInterfaceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVEnSightReaderInterface", vtkPVEnSightReaderInterfaceNewCommand,
                  vtkPVEnSightReaderInterfaceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVMethodInterface", vtkPVMethodInterfaceNewCommand,
                  vtkPVMethodInterfaceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVRenderView", vtkPVRenderViewNewCommand,
                  vtkPVRenderViewCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSelectionList", vtkPVSelectionListNewCommand,
                  vtkPVSelectionListCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSource", vtkPVSourceNewCommand,
                  vtkPVSourceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSourceCollection", vtkPVSourceCollectionNewCommand,
                  vtkPVSourceCollectionCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSourceInterface", vtkPVSourceInterfaceNewCommand,
                  vtkPVSourceInterfaceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSourceList", vtkPVSourceListNewCommand,
                  vtkPVSourceListCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVWindow", vtkPVWindowNewCommand,
                  vtkPVWindowCommand);
  vtkTclCreateNew(interp,(char *) "vtkSingleContourFilter", vtkSingleContourFilterNewCommand,
                  vtkSingleContourFilterCommand);
  vtkTclCreateNew(interp,(char *) "vtkSimpleFieldDataToAttributeDataFilter", vtkSimpleFieldDataToAttributeDataFilterNewCommand,
                  vtkSimpleFieldDataToAttributeDataFilterCommand);
  vtkTclCreateNew(interp,(char *) "vtkStringList", vtkStringListNewCommand,
                  vtkStringListCommand);
  return TCL_OK;
}
