
/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_Vector4D.h
 *  \brief Definition of class \c GW_Vector4D
 *  \author Gabriel Peyré 2001-09-10
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_Vector4D_h
#define GW_Vector4D_h

#include "GW_MathsConfig.h"
#include "GW_Maths.h"
#include "GW_VectorStatic.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Vector4D
 *  \brief  A 4D vector, with useful operators and methods.
 *  \author Gabriel Peyré 2001-09-10
 *
 *    This class is used every where ...
 *    A constructor makes the conversion GW_Float[4] -> GW_Vector4D
 *    A lot of operator have been defined to make common maths operations, so use them !
 */
/*------------------------------------------------------------------------------*/

class GW_Vector4D:    public GW_VectorStatic<4,GW_Float>
{

public:

    GW_Vector4D()                                            :GW_VectorStatic<4,GW_Float>()    {}
    GW_Vector4D( const GW_VectorStatic<4,GW_Float>&    v )        :GW_VectorStatic<4,GW_Float>(v)    {}
    GW_Vector4D( GW_Float a )                                :GW_VectorStatic<4,GW_Float>(a)    {}
    GW_Vector4D( GW_Float a, GW_Float b, GW_Float c, GW_Float d )
    {
        aCoords_[0] = a;
        aCoords_[1] = b;
        aCoords_[2] = c;
        aCoords_[2] = d;
    }
    void SetCoord( GW_Float a, GW_Float b, GW_Float c, GW_Float d )
    {
        aCoords_[0] = a;
        aCoords_[1] = b;
        aCoords_[2] = c;
        aCoords_[2] = d;
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_Vector4D", s);
        TestClassFooter("GW_Vector4D", s);
    }

};


} // End namespace GW


#endif // GW_Vector4D_h

///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2009 Gabriel Peyre
