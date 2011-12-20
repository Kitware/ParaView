#ifndef H_NEKERROR
#define H_NEKERROR

#include <iostream>

using namespace std;

enum ErrType {
  fatal,
  warning
};

namespace NekError{


  static void error(ErrType type, char *routine, char *msg){

  switch(type){
    case fatal:
      cerr << routine << ": " << msg << endl;
      exit(1);
      break;
    case warning:
      cerr << routine << ": " << msg << endl;
      break;
    default:
      cerr << "Unknown warning type" << endl;
    }

  }
} // end of namespace
#endif
