#include <myApplication.h>

myApplicationCore::myApplicationCore(int argc, char* argv[])
  : pqPVApplicationCore(argc, argv)
{
  this->useVersionedSettings(false);
}
