#include "vtkTclUtil.h"
int vtkColorByProcessCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkColorByProcessNewCommand();
int vtkDummyRenderWindowInteractorCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkDummyRenderWindowInteractorNewCommand();
int vtkGetRemoteGhostCellsCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkGetRemoteGhostCellsNewCommand();
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
int vtkParallelDecimateCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkParallelDecimateNewCommand();
int vtkPVActorCompositeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVActorCompositeNewCommand();
int vtkPVAnimationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVAnimationNewCommand();
int vtkPVApplicationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVApplicationNewCommand();
int vtkPVAssignmentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVAssignmentNewCommand();
int vtkPVCommandListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVCommandListNewCommand();
int vtkPVCutterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVCutterNewCommand();
int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVDataNewCommand();
int vtkPVDataSetToDataSetFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVDataSetToDataSetFilterNewCommand();
int vtkPVDataSetToPolyDataFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVDataSetToPolyDataFilterNewCommand();
int vtkPVExtentTranslatorCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVExtentTranslatorNewCommand();
int vtkPVGetRemoteGhostCellsCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVGetRemoteGhostCellsNewCommand();
int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVGlyph3DNewCommand();
int vtkPVImageDataCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageDataNewCommand();
int vtkPVImageClipCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageClipNewCommand();
int vtkPVImageSliceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageSliceNewCommand();
int vtkPVImageSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageSourceNewCommand();
int vtkPVImageTextureFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageTextureFilterNewCommand();
int vtkPVImageToImageFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageToImageFilterNewCommand();
int vtkPVMethodInterfaceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVMethodInterfaceNewCommand();
int vtkPVParallelDecimateCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVParallelDecimateNewCommand();
int vtkPVPolyDataCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVPolyDataNewCommand();
int vtkPVPolyDataSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVPolyDataSourceNewCommand();
int vtkPVPolyDataToPolyDataFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVPolyDataToPolyDataFilterNewCommand();
int vtkPVRenderViewCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVRenderViewNewCommand();
int vtkPVRunTimeContourCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVRunTimeContourNewCommand();
int vtkPVScalarBarCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVScalarBarNewCommand();
int vtkPVSelectionListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSelectionListNewCommand();
int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceNewCommand();
int vtkPVSourceCollectionCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceCollectionNewCommand();
int vtkPVSourceListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceListNewCommand();
int vtkPVWindowCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVWindowNewCommand();
int vtkRunTimeContourCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkRunTimeContourNewCommand();
int vtkSingleContourFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkSingleContourFilterNewCommand();

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
  vtkTclCreateNew(interp,(char *) "vtkColorByProcess", vtkColorByProcessNewCommand,
                  vtkColorByProcessCommand);
  vtkTclCreateNew(interp,(char *) "vtkDummyRenderWindowInteractor", vtkDummyRenderWindowInteractorNewCommand,
                  vtkDummyRenderWindowInteractorCommand);
  vtkTclCreateNew(interp,(char *) "vtkGetRemoteGhostCells", vtkGetRemoteGhostCellsNewCommand,
                  vtkGetRemoteGhostCellsCommand);
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
  vtkTclCreateNew(interp,(char *) "vtkParallelDecimate", vtkParallelDecimateNewCommand,
                  vtkParallelDecimateCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVActorComposite", vtkPVActorCompositeNewCommand,
                  vtkPVActorCompositeCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVAnimation", vtkPVAnimationNewCommand,
                  vtkPVAnimationCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVApplication", vtkPVApplicationNewCommand,
                  vtkPVApplicationCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVAssignment", vtkPVAssignmentNewCommand,
                  vtkPVAssignmentCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVCommandList", vtkPVCommandListNewCommand,
                  vtkPVCommandListCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVCutter", vtkPVCutterNewCommand,
                  vtkPVCutterCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVData", vtkPVDataNewCommand,
                  vtkPVDataCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVDataSetToDataSetFilter", vtkPVDataSetToDataSetFilterNewCommand,
                  vtkPVDataSetToDataSetFilterCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVDataSetToPolyDataFilter", vtkPVDataSetToPolyDataFilterNewCommand,
                  vtkPVDataSetToPolyDataFilterCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVExtentTranslator", vtkPVExtentTranslatorNewCommand,
                  vtkPVExtentTranslatorCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVGetRemoteGhostCells", vtkPVGetRemoteGhostCellsNewCommand,
                  vtkPVGetRemoteGhostCellsCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVGlyph3D", vtkPVGlyph3DNewCommand,
                  vtkPVGlyph3DCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVImageData", vtkPVImageDataNewCommand,
                  vtkPVImageDataCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVImageClip", vtkPVImageClipNewCommand,
                  vtkPVImageClipCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVImageSlice", vtkPVImageSliceNewCommand,
                  vtkPVImageSliceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVImageSource", vtkPVImageSourceNewCommand,
                  vtkPVImageSourceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVImageTextureFilter", vtkPVImageTextureFilterNewCommand,
                  vtkPVImageTextureFilterCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVImageToImageFilter", vtkPVImageToImageFilterNewCommand,
                  vtkPVImageToImageFilterCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVMethodInterface", vtkPVMethodInterfaceNewCommand,
                  vtkPVMethodInterfaceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVParallelDecimate", vtkPVParallelDecimateNewCommand,
                  vtkPVParallelDecimateCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVPolyData", vtkPVPolyDataNewCommand,
                  vtkPVPolyDataCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVPolyDataSource", vtkPVPolyDataSourceNewCommand,
                  vtkPVPolyDataSourceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVPolyDataToPolyDataFilter", vtkPVPolyDataToPolyDataFilterNewCommand,
                  vtkPVPolyDataToPolyDataFilterCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVRenderView", vtkPVRenderViewNewCommand,
                  vtkPVRenderViewCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVRunTimeContour", vtkPVRunTimeContourNewCommand,
                  vtkPVRunTimeContourCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVScalarBar", vtkPVScalarBarNewCommand,
                  vtkPVScalarBarCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSelectionList", vtkPVSelectionListNewCommand,
                  vtkPVSelectionListCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSource", vtkPVSourceNewCommand,
                  vtkPVSourceCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSourceCollection", vtkPVSourceCollectionNewCommand,
                  vtkPVSourceCollectionCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVSourceList", vtkPVSourceListNewCommand,
                  vtkPVSourceListCommand);
  vtkTclCreateNew(interp,(char *) "vtkPVWindow", vtkPVWindowNewCommand,
                  vtkPVWindowCommand);
  vtkTclCreateNew(interp,(char *) "vtkRunTimeContour", vtkRunTimeContourNewCommand,
                  vtkRunTimeContourCommand);
  vtkTclCreateNew(interp,(char *) "vtkSingleContourFilter", vtkSingleContourFilterNewCommand,
                  vtkSingleContourFilterCommand);
  return TCL_OK;
}
