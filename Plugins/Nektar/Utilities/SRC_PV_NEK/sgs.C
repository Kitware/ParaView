
// ----------------------------------- //
// headers and misc decls, macros, etc //
// ----------------------------------- //

// mpi hack
#include "mpi.h"

//
// system headers
//
#include <algorithm>
#include <cassert>
#include <cmath>

//
// module headers
//
#include "sgs.hpp"
#include "csgs.h"

//
// user headers
//


//
// misc
//
using std::vector;

struct csgs_impl {
    SGS* sgs;
};

// TODO refactor this out later with a more comprehensive error module.
// check is like assert except it doesn't get turned off by NDEBUG.
#ifndef check
#include <cstdio>
#include <cstdlib>
#define check(expr) \
    do { \
        if ( !(expr) ) { \
            std::fprintf(stderr, "%s:%d check failed '%s'\n", \
                __FILE__, __LINE__, #expr); \
            std::abort(); \
        } \
    } while (0)
#endif

namespace {
    // the following value was picked at random ... feel free to play with it
    vector<int>::size_type small_vector_size = 1000;
    bool check_uniqueness(const vector<int>& v);
    bool check_dist(const vector<int>& v);
}

// --------------------------- //
// public function definitions //
// --------------------------- //

// Constructor with C style arrays as arguments instead of vectors.
// Currently the only style of initialization that is supported is the
// integer list version, vs the bitvector of the original gs library by
// Tufo.  The argument dist is currently a dummy argument but it must be
// a valid array with the same length as part. elms must not have duplicates.
// part must not contain duplicates
// but this thread's rank is allowed, b/c it get's imediately pruned.
// comm is the MPI communicator group over the global ops are to be
// performed.
SGS::SGS(const int* elms, const int num_elms, const int* part,
    const int num_part, const int* dist, MPI_Comm comm)
: _elms(num_elms), _part(num_part), _dist(num_part),
    _recv_req(0), _send_req(0), _status(0),
    _map(0), _send_buf(0), _recv_buf(0), _map_sizes(0),
    _comm(comm),
    _size(num_elms), _part_size(num_part), _rank(-1), _comm_size(-1),
    _posted_recv(false)
{
    // checks input and then copies C arrays into vectors ... then the
    // rest is the same as the other constructor
    check(elms != 0);
    check(num_elms >= 0);
    check(part != 0);
    check(num_part >= 0);
    check(dist != 0);

    std::copy(elms, elms + num_elms, _elms.begin());
    std::copy(part, part + num_part, _part.begin());
    std::copy(dist, dist + num_part, _dist.begin());

    check(check_uniqueness(_elms));
    check(check_uniqueness(_part));
    check(check_dist(_dist));
    check(_part.size() == _dist.size());

    // the rest is the same as the other constructor
    int status = MPI_Comm_rank(comm, &_rank);
    check(MPI_SUCCESS == status);

    status = MPI_Comm_size(comm, &_comm_size);
    check(MPI_SUCCESS == status);

    // if part contains itself prune the list
    vector<int>::iterator part_iter = _part.begin();
    const vector<int>::iterator part_end = _part.end();

    while ( part_iter != part_end )
    {
        if ( *part_iter == _rank )
        {
            _part.erase(part_iter);
            _part_size = _part.size();
            break;
        }

        ++part_iter;
    }

    init_int_list();
}

// Basically the same as above constructor except for the fact that it takes
// vectors as arguments.
SGS::SGS(const vector<int>& elms, const vector<int>& part,
    const vector<int>& dist, MPI_Comm comm)
: _elms(elms), _part(part), _dist(dist),
    _recv_req(0), _send_req(0), _status(0),
    _map(0), _send_buf(0), _recv_buf(0), _map_sizes(0),
    _comm(comm),
    _size(elms.size()), _part_size(part.size()), _rank(-1), _comm_size(-1),
    _posted_recv(false)
{
    check(check_uniqueness(elms));
    check(check_uniqueness(part));
    check(check_dist(dist));
    check(part.size() == dist.size());

    int status = MPI_Comm_rank(comm, &_rank);
    check(MPI_SUCCESS == status);

    status = MPI_Comm_size(comm, &_comm_size);
    check(MPI_SUCCESS == status);

    // if part contains itself prune the list
    vector<int>::iterator part_iter = _part.begin();
    const vector<int>::iterator part_end = _part.end();

    while ( part_iter != part_end )
    {
        if ( *part_iter == _rank )
        {
            _part.erase(part_iter);
            _part_size = _part.size();
            break;
        }

        ++part_iter;
    }

    init_int_list();
}

void SGS::post_recv()
{
    for ( int i = 0; i < _part_size; ++i )
        recv(_recv_buf[i], i);

    _posted_recv = true;
}

void SGS::plus(double* val)
{
    assert(val != 0);

    if (!_part_size) return;

    if ( !_posted_recv )
        post_recv();

    _posted_recv = false;

    load_send_buf(val);

    for ( int i = 0; i < _part_size; ++i )
        send(_send_buf[i], i);

#if 0

    for ( int i = 0; i < _part_size; ++i )
    {
        const int k = waitany_recv();

        for ( int j = 0; j < _map_sizes[k]; ++j )
            val[_map[k][j]] += _recv_buf[k][j];
    }

#else

    const int p_size = _part_size;
    int counter = 0;

    do
    {
        const int k = waitany_recv();

        vector<double>::const_iterator       buf     = _recv_buf[k].begin();
        const vector<double>::const_iterator buf_end = _recv_buf[k].end();
        vector<int>::const_iterator          map     = _map[k].begin();

        do
        {
            val[*map++] += *buf++;
        }
        while ( buf != buf_end );

        ++counter;
    }
    while ( counter < p_size );

#endif

    waitall_send();
}

void SGS::fabs_max(double* val)
{
    assert(val != 0);

    if (!_part_size) return;


    if ( !_posted_recv )
        post_recv();

    _posted_recv = false;

    load_send_buf(val);

    for ( int i = 0; i < _part_size; ++i )
        send(_send_buf[i], i);

#if 0

    for ( int i = 0; i < _part_size; ++i )
    {
        const int k = waitany_recv();

        for ( int j = 0; j < _map_sizes[k]; ++j )
        {
            const double t1 = std::fabs(val[_map[k][j]]);
            const double t2 = std::fabs(_recv_buf[k][j]);

            if ( t2 > t1 )
                val[_map[k][j]] = _recv_buf[k][j];
        }
    }

#else

    const int p_size = _part_size;
    int counter = 0;

    do
    {
        const int k = waitany_recv();

        vector<double>::const_iterator       buf     = _recv_buf[k].begin();
        const vector<double>::const_iterator buf_end = _recv_buf[k].end();
        vector<int>::const_iterator          map     = _map[k].begin();

        do
        {
            const double t1 = std::fabs(val[*map]);
            const double t2 = std::fabs(*buf);

            if ( t2 > t1 )
            {
                val[*map++] = *buf++;
            }
            else
            {
                ++map;
                ++buf;
            }
        }
        while ( buf != buf_end );

        ++counter;
    }
    while ( counter < p_size );

#endif

    waitall_send();
}

CSGS CSGS_init(const int* elms, int num_elms, const int* part, int num_part,
    const int* dist, MPI_Comm comm)
{
    CSGS csgs = new csgs_impl();
    csgs->sgs = new SGS(elms, num_elms, part, num_part, dist, comm);

    return csgs;
}

void CSGS_free(CSGS csgs)
{
    delete csgs->sgs;
    delete csgs;
}

void CSGS_post_recv(CSGS csgs)
{
    csgs->sgs->post_recv();
}

void CSGS_plus(CSGS csgs, double* val)
{
    csgs->sgs->plus(val);
}

void CSGS_fabs_max(CSGS csgs, double* val)
{
    csgs->sgs->fabs_max(val);
}

int CSGS_rank(CSGS csgs)
{
    return csgs->sgs->rank();
}

int CSGS_size(CSGS csgs)
{
    return csgs->sgs->size();
}

int CSGS_comm_size(CSGS csgs)
{
    return csgs->sgs->comm_size();
}

// ---------------------------- //
// private function definitions //
// ---------------------------- //

// Initializes object via the integer lists.  This initialization should be
// used when each rank has only a few elements compared to the total number
// of distinct elements involved in the global ops, ie when the total number
// of threads is large.
void SGS::init_int_list()
{
    //
    // first find out the sizes of elms on each rank of part
    //
    const int p_size = _part_size;
    vector<int> sizes(p_size, 0);

    // adjust request and status vectors b/c their sizes are zero
    _send_req.resize(p_size);
    _recv_req.resize(p_size);
    _status.resize(p_size);

    reset_requests();

    // 10/16/2008 - Leo suggested that splitting the sends and recvs into
    // separate loops should perform better
    for ( int i = 0; i < p_size; ++i )
        recv(sizes[i], i);

    for ( int i = 0; i < p_size; ++i )
        send(_size, i);

    waitall_recv();

    // holds the input elms of ranks from part
    vector<vector<int> > other_elms(p_size);

    for ( int i = 0; i < p_size; ++i )
        other_elms[i].resize(sizes[i]);

    waitall_send();

    //
    // get the other ranks elms, ie fill other_elms
    //
    reset_requests();

    for ( int i = 0; i < p_size; ++i )
        recv(other_elms[i], i);

    for ( int i = 0; i < p_size; ++i )
        send(_elms, i);

    waitall_recv();
    waitall_send();

    //
    // prune part of threads which do not share elements with this thread
    //
    const int e_size = _elms.size();
    vector<bool> prune(p_size, true);

    for ( int i = 0; i < p_size; ++i )
    {
        vector<int>::iterator other_begin = other_elms[i].begin();
        vector<int>::iterator other_end   = other_elms[i].end();
        vector<int>::iterator find_res;

        for ( int j = 0; j < e_size; ++j )
        {
            find_res = std::find(other_begin, other_end, _elms[j]);

            // if _elms[j] is in other_elms[i] do not prune other_elms[i]
            if ( find_res != other_end )
            {
                prune[i] = false;
                break;
            }
        }
    }

    vector<int>::iterator          part_iter  = _part.begin();
    vector<int>::iterator          dist_iter  = _dist.begin();
    vector<vector<int> >::iterator other_iter = other_elms.begin();
    vector<bool>::iterator         prune_iter = prune.begin();
    const vector<bool>::iterator   prune_end  = prune.end();

    while ( prune_iter != prune_end )
    {
        if ( *prune_iter )
        {
            part_iter = _part.erase(part_iter);
            dist_iter = _dist.erase(dist_iter);
            other_iter = other_elms.erase(other_iter);
            ++prune_iter;
        }
        else
        {
            ++prune_iter;
            ++part_iter;
            ++dist_iter;
            ++other_iter;
        }
    }

    if ( _part_size != static_cast<int>(_part.size()) )
        _part_size = _part.size();

    _recv_req.resize(_part_size);
    _send_req.resize(_part_size);
    _status.resize(_part_size);
    _map.resize(_part_size);
    _send_buf.resize(_part_size);
    _recv_buf.resize(_part_size);
    _map_sizes.resize(_part_size);

    // 10/16/2008 - this should be the last reset according to Leo
    reset_requests();

    //
    // set up mappings and buffers
    //
    const vector<int>::iterator elms_end = _elms.end();
    const vector<int>::iterator elms_begin = _elms.begin();

    // this version handles unsorted but unique arrays but it's an
    // O(n^2) algo ... I can sort and then unsort for an O(n log n) algo
    // but it's a lot of work and considering this is a "sparse" gather
    // scatter I think this naive/simpler O(n^2) is faster in practise
    for ( int i = 0; i < _part_size; ++i )
    {
        vector<int>::iterator other_head = other_elms[i].begin();
        const vector<int>::iterator other_end = other_elms[i].end();

        while ( other_head != other_end )
        {
            vector<int>::iterator iter = std::find(elms_begin, elms_end,
                *other_head);

            if ( iter != elms_end )
                _map[i].push_back(iter - elms_begin);

            ++other_head;
        }

        _send_buf[i].resize(_map[i].size());
        _recv_buf[i].resize(_map[i].size());
        _map_sizes[i] = _map[i].size();
    }
}

// TODO how about MPI_Status?
void SGS::reset_requests()
{
    for ( int i = 0; i < _part_size; ++i )
    {
        _send_req[i] = MPI_REQUEST_NULL;
        _recv_req[i] = MPI_REQUEST_NULL;
    }
}

void SGS::load_send_buf(const double* val)
{
#if 0

    for ( int i = 0; i < _part_size; ++i )
        for ( int j = 0; j < _map_sizes[i]; ++j )
            _send_buf[i][j] = val[_map[i][j]];

#else

    vector<vector<double> >::iterator       send_iter = _send_buf.begin();
    const vector<vector<double> >::iterator send_end = _send_buf.end();
    vector<vector<int> >::const_iterator    map_iter = _map.begin();

    do
    {
        vector<double>::iterator       buf = (*send_iter).begin();
        const vector<double>::iterator buf_end = (*send_iter).end();
        vector<int>::const_iterator    map = (*map_iter).begin();

        do
        {
            *buf++ = val[*map++];
        }
        while ( buf != buf_end );

        ++send_iter;
        ++map_iter;
    }
    while ( send_iter != send_end );

#endif
}

namespace {
    bool check_uniqueness(const vector<int>& v)
    {
        const vector<int>::size_type v_size = v.size();

        if ( v_size <= 1 )
            return true;

        // if input "small" use naive o/w use asymptotically better one
        // value defined above in anon-namespace decls
        if ( v_size < small_vector_size )
        {
            // naive O(n^2) algo
            vector<int>::size_type i, j;
            for ( i = 0; i < v_size - 1; ++i )
                for ( j = i + 1; j < v_size; ++j )
                    if ( v[i] == v[j] )
                        return false;
        }
        else
        {
            // make a copy: O(n)
            // sort copy: O(n log n)
            // check uniqueness: O(n)
            // so O(n log n)
            vector<int> temp(v);

            std::sort(temp.begin(), temp.end());

            vector<int>::iterator orig_end, unique_end;
            orig_end = temp.end();
            unique_end = std::unique(temp.begin(), temp.end());

            if ( orig_end > unique_end )
                return false;
        }

        return true;
    }

    // TODO
    bool check_dist(const vector<int>& v)
    {
        return v.size() || 1;
    }
} // end anonymous namespace
