#include "stdafx.h"
#include "GW_Config.h"

using namespace GW;

string GW::gw_alinea = "  * ";
string GW::gw_endl = "\n";
FILE* GW::GW_OutputStream = NULL;

void GW::GW_OutputComment( const char* str )
{
    if( GW_OutputStream!=NULL )
    {
        string str2 = gw_alinea + str + gw_endl;
        fprintf( GW_OutputStream, "%s", str2.c_str() );
    }
}
