#ifndef sgs_hpp
#define sgs_hpp

// mpi hack
#include "mpi.h"

// system headers
#include <assert.h>
#include <vector>

// user headers
//#include "mpi.h"


// SGS == Sparse Gather Scatter
class SGS
{
public:
    SGS(const int* elms, int num_elms,
        const int* part, int num_part,
        const int* dist,
        MPI_Comm comm);
    SGS(const std::vector<int>& elms,
        const std::vector<int>& part,
        const std::vector<int>& dist,
        MPI_Comm comm);
    ~SGS() {}

    void post_recv();

    void plus(double* val);
    void fabs_max(double* val);

    int rank() const { return _rank; }
    int size() const { return _size; }
    int comm_size() const { return _comm_size; }

private:
    std::vector<int> _elms;
    std::vector<int> _part;
    std::vector<int> _dist;
    std::vector<MPI_Request> _recv_req;
    std::vector<MPI_Request> _send_req;
    std::vector<MPI_Status> _status;
    std::vector<std::vector<int> > _map;
    std::vector<std::vector<double> > _send_buf;
    std::vector<std::vector<double> > _recv_buf;
    std::vector<int> _map_sizes;
    MPI_Comm _comm;
    int _size;
    int _part_size;
    int _rank;
    int _comm_size;
    bool _posted_recv;

    void init_int_list();
    void reset_requests();
    void load_send_buf(const double* val);

    void recv(int& data, const int src, const int tag = 666)
    {
        assert(src >= 0 && src < _part_size);
        assert(_recv_req.size() == _part.size());

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Irecv(&data, 1, MPI_INT, _part[src], tag, _comm, &_recv_req[src]);
        assert(MPI_SUCCESS == status);
    }

    void recv(std::vector<int>& data, const int src, const int tag = 777)
    {
        assert(src >= 0 && src < _part_size);
        assert(_recv_req.size() == _part.size());

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Irecv(&data[0], data.size(), MPI_INT, _part[src], tag, _comm,
            &_recv_req[src]);
        assert(MPI_SUCCESS == status);
    }

    void recv(std::vector<double>& data, const int src, const int tag = 888)
    {
        assert(src >= 0 && src < _part_size);
        assert(_recv_req.size() == _part.size());

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Irecv(&data[0], data.size(), MPI_DOUBLE, _part[src],
            tag, _comm, &_recv_req[src]);
        assert(MPI_SUCCESS == status);
    }

    void send(int& data, const int tgt, const int tag = 666)
    {
        assert(tgt >= 0 && tgt < _part_size);
        assert(_send_req.size() == _part.size());

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Isend(&data, 1, MPI_INT, _part[tgt], tag,
            _comm, &_send_req[tgt]);
        assert(MPI_SUCCESS == status);
    }

    void send(std::vector<int>& data, const int tgt, const int tag = 777)
    {
        assert(tgt >= 0 && tgt < _part_size);
        assert(_send_req.size() == _part.size());

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Isend(&data[0], data.size(), MPI_INT, _part[tgt],
            tag, _comm, &_send_req[tgt]);
        assert(MPI_SUCCESS == status);
    }

    void send(std::vector<double>& data, const int tgt, const int tag = 888)
    {
        assert(tgt >= 0 && tgt < _part_size);
        assert(_send_req.size() == _part.size());

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Isend(&data[0], data.size(), MPI_DOUBLE, _part[tgt],
            tag, _comm, &_send_req[tgt]);
        assert(MPI_SUCCESS == status);
    }

    void waitall_recv()
    {
        assert(_recv_req.size() == _part.size());
        assert(_status.size() == _part.size());
        assert(static_cast<int>(_part.size()) == _part_size);

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Waitall(_part_size, &_recv_req[0], &_status[0]);
        assert(MPI_SUCCESS == status);
    }

    int waitany_recv()
    {
        assert(_recv_req.size() == _part.size());
        assert(_status.size() == _part.size());
        assert(static_cast<int>(_part.size()) == _part_size);

        int index;

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Waitany(_part_size, &_recv_req[0], &index, MPI_STATUS_IGNORE);
        assert(MPI_SUCCESS == status);

        return index;
    }

    void waitall_send()
    {
        assert(_send_req.size() == _part.size());
        assert(_status.size() == _part.size());
        assert(static_cast<int>(_part.size()) == _part_size);

        #ifndef NDEBUG
        int status =
        #endif
        MPI_Waitall(_part_size, &_send_req[0], &_status[0]);
        assert(MPI_SUCCESS == status);
    }

    void barrier()
    {
        #ifndef NDEBUG
        int status =
        #endif
        MPI_Barrier(_comm);
        assert(MPI_SUCCESS == status);
    }
};

#endif
