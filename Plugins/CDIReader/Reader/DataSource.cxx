//#include "cdi_tools.h"
#include "DataSource.h"
#include "cdi.h"

namespace DataSource
{

CDIObject::CDIObject(std::string newURI)
{
  this->openURI(newURI);
}

int CDIObject::openURI(std::string newURI)
{
  this->setVoid();
  this->URI = newURI;

  // check if we got either *.Grib or *.nc data
  std::string check = this->URI.substr((URI.size() - 4), this->URI.size());
  if (check == "grib" || check == ".grb")
  {
    this->type = GRIB;
  }
  else
  {
    this->type = NC;
  }

  this->StreamID = streamOpenRead(this->URI.c_str());
  if (this->StreamID < 0)
  {
    this->setVoid();
    return 0;
  }

  this->VListID = streamInqVlist(this->StreamID);
  return 1;
}

void CDIObject::setVoid()
{
  if (this->StreamID > -1)
    streamClose(this->StreamID);

  StreamID = -1;
  VListID = -1;
  type = VOID;
}

CDIObject::~CDIObject()
{
  URI = "";
  this->setVoid();
}

};
