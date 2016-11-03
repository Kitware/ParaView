# A collection of utils useful for cinema importing/exporting.
# Please don't import paraview.simple or paraview.server in this module to
# ensure that it can be used in reader (programmable source) code.

import numpy as np

def convert_pose_to_camera(iPosition, iFocalPoint, iViewUp, pose, camType):
    """Converts input eye, focal point and view up using the pose and camType
    and returns (neweye, newfp, newviewup)"""
    def VecNormalize( V1):
        return V1/np.linalg.norm(V1)
    def VecMatrixMul( V, M):
        return [
            V[0]*M[0][0] + V[1]*M[0][1] + V[2]*M[0][2],
            V[0]*M[1][0] + V[1]*M[1][1] + V[2]*M[1][2],
            V[0]*M[2][0] + V[1]*M[2][1] + V[2]*M[2][2]
            ]
    def MatrixMatrixMul( mtx_a, mtx_b):
        tpos_b = zip( *mtx_b)
        rtn = [[ sum( ea*eb for ea,eb in zip(a,b)) for b in tpos_b] for a in mtx_a]
        return rtn

    iViewUp = VecNormalize(iViewUp)

    atvec = VecNormalize(np.subtract(iPosition, iFocalPoint))
    rightvec = np.cross(atvec, iViewUp)
    m = [[rightvec[0], rightvec[1], rightvec[2]],
         [iViewUp[0], iViewUp[1], iViewUp[2]],
         [atvec[0],  atvec[1],  atvec[2]]]
    #OK because orthogonal thus invert equals trans
    mInv = [[m[0][0], m[1][0], m[2][0]],
            [m[0][1], m[1][1], m[2][1]],
            [m[0][2], m[1][2], m[2][2]]]
    mi = MatrixMatrixMul(mInv, pose)
    mf = MatrixMatrixMul(mi, m)

    newUp = VecMatrixMul(iViewUp, mf)

    if camType == "azimuth-elevation-roll":
        pi = np.subtract(iPosition, iFocalPoint)
        pr = VecMatrixMul(pi, mf)
        pf = np.add(pr, iFocalPoint)
        return (pf, iFocalPoint, newUp)
    elif self.camType == "yaw-pitch-roll":
        pi = np.subtract(iFocalPoint, iPosition)
        pr = VecMatrixMul(pi, mf)
        pf = np.add(pr, iPosition)
        return (iPosition, pf, newUp)
    else:
        print ("ERROR unexpected camera type")
        return (iPosition, iFocalPoint, iViewUp)
