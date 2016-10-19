#include "vtkSMTransferFunctionProxy.h"

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    cerr << "Usage:\n"
         << "    " << argv[0] << " [path to colormap xml to convert] [path to output json file]"
         << endl;
    return -1;
  }

  return vtkSMTransferFunctionProxy::ConvertLegacyColorMapsToJSON(argv[1], argv[2]) ? 0 : -1;
}
