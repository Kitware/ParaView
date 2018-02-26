
/*------------------------------------------------------------------------------*/
/**
 *  \file  GW_Vector2D.h
 *  \brief Definition of class \c GW_Vector2D
 *  \author Gabriel Peyré 2001-09-10
 */
/*------------------------------------------------------------------------------*/

#ifndef GW_Vector2D_h
#define GW_Vector2D_h

#include "GW_MathsConfig.h"
#include "GW_VectorStatic.h"

GW_BEGIN_NAMESPACE

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Vector2D
 *  e\brief  A 3D vector, with useful operators and methods.
 *  \author Gabriel Peyré 2001-09-10
 *
 *    Just a planar vector.
 */
/*------------------------------------------------------------------------------*/

class GW_Vector2D:    public GW_VectorStatic<2,GW_Float>
{

public:

    GW_Vector2D()                                            :GW_VectorStatic<2,GW_Float>()    {}
    GW_Vector2D( const GW_VectorStatic<2,GW_Float>& v )        :GW_VectorStatic<2,GW_Float>(v)    {}
    GW_Vector2D( GW_Float a )                                :GW_VectorStatic<2,GW_Float>(a)    {}
    GW_Vector2D( GW_Float a, GW_Float b )
    {
        aCoords_[0] = a;
        aCoords_[1] = b;
    }
    void SetCoord( GW_Float a, GW_Float b )
    {
        aCoords_[0] = a;
        aCoords_[1] = b;
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_Vector2D::Rotate
    /**
    *  \param  a [GW_Float] Angle
    *  \return [GW_Vector2D] Rotated vector.
    *  \author Gabriel Peyré
    *  \date   5-26-2003
    *
    *  Return a rotated vector.
    */
    /*------------------------------------------------------------------------------*/
    GW_Vector2D Rotate( GW_Float a )
    {
        return GW_VectorStatic<2,GW_Float>::Rotate(a, 0,1 );
    }

    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_Vector2D", s);
        s << "Test for rotation of vector 2D" << endl;
        GW_Vector2D v;
        v.Randomize();
        s << "Vector v : " << v << endl;
        GW_Vector2D w = v.Rotate(GW_HALFPI);
        s << "v rotated of pi/2 : " << w << endl;
        GW_Float dot = w*v;
        GW_ASSERT( GW_ABS(dot)<1e-6 );
        TestClassFooter("GW_Vector2D", s);
    }

};




GW_END_NAMESPACE


#endif // GW_Vector2D_h

///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
