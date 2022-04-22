#ifndef CDI_DATA_SOURCE
#define CDI_DATA_SOURCE

#include <string>

namespace DataSource
{
class CDIObject
{
  enum stype
  {
    VOID,
    GRIB,
    NC
  };
  std::string URI;
  int StreamID;
  int VListID;
  stype type;

public:
  void setVoid();
  CDIObject(std::string URI);
  CDIObject()
  {
    this->StreamID = -1;
    this->setVoid();
  }
  ~CDIObject();
  int openURI(std::string URI);
  std::string getURI() const { return URI; }
  int getStreamID() const { return StreamID; }
  int getVListID() const { return VListID; }
  stype getType() const { return type; }
  bool isVoid() const { return this->type == VOID; }
};

};
#endif // CDI_DATA_SOURCE
