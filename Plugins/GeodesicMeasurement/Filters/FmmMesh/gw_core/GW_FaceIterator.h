
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_FaceIterator.h
 *  \brief  Definition of class \c GW_FaceIterator
 *  \author Gabriel Peyré
 *  \date   4-1-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_FACEITERATOR_H_
#define _GW_FACEITERATOR_H_

#include "GW_Config.h"


namespace GW {

class GW_Face;
class GW_Vertex;

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_FaceIterator
 *  \brief  Iterator on a group of face surrounding a vertex.
 *  \author Gabriel Peyré
 *  \date   4-1-2003
 *
 *  Useful iterator.
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_FaceIterator
{
public:

    GW_FaceIterator(  GW_Face* pFace, GW_Vertex* pOrigin, GW_Vertex* pDirection, GW_U32 nNbrIncrement = 0 );

    /* assignment */
    //GW_FaceIterator& operator=( const GW_FaceIterator& it);

    /* evaluation */
    GW_Bool operator==( const GW_FaceIterator& it);
    GW_Bool operator!=( const GW_FaceIterator& it);

    /* indirection */
    GW_Face* operator*(  );

    /* progression : \todo take in account NULL pointer */
    void operator++();

    GW_Vertex* GetLeftVertex();
    GW_Vertex* GetRightVertex();

private:

    GW_Face* pFace_;
    GW_Vertex* pOrigin_;
    GW_Vertex* pDirection_;

    /** just for debug purpose */
    GW_U32 nNbrIncrement_;
};


inline GW_FaceIterator::GW_FaceIterator(  GW_Face* pFace, GW_Vertex* pOrigin, GW_Vertex* pDirection, GW_U32 nNbrIncrement )
:    pFace_        ( pFace),
    pOrigin_    ( pOrigin  ),
    pDirection_    ( pDirection ),
    nNbrIncrement_    ( nNbrIncrement )
{ }


/* egality */
inline GW_Bool GW_FaceIterator::operator==( const GW_FaceIterator& it)
{
    return (pFace_==it.pFace_)
        && (pOrigin_==it.pOrigin_)
        && (pDirection_==it.pDirection_);
}

/* egality */
inline GW_Bool GW_FaceIterator::operator!=( const GW_FaceIterator& it)
{
    return (pFace_!=it.pFace_)
        || (pOrigin_!=it.pOrigin_)
        || (pDirection_!=it.pDirection_);
}


/* dereference */
inline GW_Face* GW_FaceIterator::operator*(  )
{
    return pFace_;
}



} // End namespace GW



#endif // _GW_FACEITERATOR_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
