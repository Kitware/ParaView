/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_ProgressBar.h
 *  \brief  Definition of class \c GW_ProgressBar
 *  \author Gabriel Peyré
 *  \date   4-9-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_PROGRESSBAR_H_
#define _GW_PROGRESSBAR_H_

#include "../gw_core/GW_Config.h"
#include "../gw_core/GW_Vertex.h"

namespace GW {

class GW_VoronoiVertex;

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_ProgressBar
 *  \brief  An ASCII progress bar
 *  \author Gabriel Peyré
 *  \date   1-02-2004
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_ProgressBar
{

public:

    GW_ProgressBar( GW_U32 nLength = 40, char char_begin = '|', char char_empty = '-', char char_progress = '*'  )
    :nLength_( nLength ),
    rCurPos_( 0 ),
    rLastPrint_( 0 ),
    rDelta_( 1.0/nLength ),
    char_empty_( char_empty ),
    char_begin_( char_begin ),
    char_progress_( char_progress )
    {
        /* NOTHING */
    }

    void Begin()
    {
        cout << char_begin_;
        for( GW_U32 i=0; i<nLength_; ++i )
            cout << char_empty_;
        cout << char_begin_;
        for( GW_U32 i=0; i<nLength_+1; ++i )
            cout << "\b";
    }

    void Update(GW_Float rNewPos)
    {
//        GW_CLAMP_01(rNewPos);
        if( rNewPos>=rCurPos_ )
        {
            GW_U32 nToPrint = (GW_U32) floor( (rNewPos-rLastPrint_)/rDelta_ );
            if( nToPrint>0 )
            {
                for( GW_U32 i=0; i<nToPrint; ++i )
                    cout << char_progress_;
                rLastPrint_ = rLastPrint_+nToPrint*rDelta_;
            }
            rCurPos_ = rNewPos;
        }
    }

    void End()
    {
        /* flush resting time */
        this->Update(1.0+rDelta_/10);
        cout << char_begin_;
    }

private:

    GW_U32 nLength_;
    GW_Float rCurPos_;
    GW_Float rDelta_;
    GW_Float rLastPrint_;
    char char_begin_;
    char char_empty_;
    char char_progress_;
};


} // End namespace GW


#endif // _GW_GEODESICVERTEX_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
