"""
A collection of utilities for cinema camera handling.
"""

import numpy as np
import math


def convert_pose_to_camera(iPosition, iFocalPoint, iViewUp, pose, camType):
    """Converts input eye, focal point and view up using the pose and camType
    and returns (neweye, newfp, newviewup)"""
    def VecNormalize(V1):
        return V1/np.linalg.norm(V1)

    def VecMatrixMul(V, M):
        return [
            V[0]*M[0][0] + V[1]*M[0][1] + V[2]*M[0][2],
            V[0]*M[1][0] + V[1]*M[1][1] + V[2]*M[1][2],
            V[0]*M[2][0] + V[1]*M[2][1] + V[2]*M[2][2]
            ]

    def MatrixMatrixMul(mtx_a, mtx_b):
        tpos_b = zip(*mtx_b)
        rtn = [
            [sum(ea*eb for ea, eb in zip(a, b)) for b in tpos_b]
            for a in mtx_a]
        return rtn

    iViewUp = VecNormalize(iViewUp)

    atvec = VecNormalize(np.subtract(iPosition, iFocalPoint))
    rightvec = np.cross(atvec, iViewUp)
    m = [[rightvec[0], rightvec[1], rightvec[2]],
         [iViewUp[0], iViewUp[1], iViewUp[2]],
         [atvec[0],  atvec[1],  atvec[2]]]
    # OK because orthogonal thus invert equals trans
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
    elif camType == "yaw-pitch-roll":
        pi = np.subtract(iFocalPoint, iPosition)
        pr = VecMatrixMul(pi, mf)
        pf = np.add(pr, iPosition)
        return (iPosition, pf, newUp)
    else:
        print("ERROR unexpected camera type")
        return (iPosition, iFocalPoint, iViewUp)


def nearest_camera(poses, mnext):
    """
    find index of the pose that is closest match to mnext
    """
    # TODO: profile, and then if important, optimize this
    # 1: only need to compare 2 of 3 vectors since they are orthogonal
    # 2: will be faster still if we push the matrices into numpy
    # (flatten too perhaps)

    # algorithm: reduce to distance value, and then pick the index of
    # pose with least distance to mnext
    best = -1
    bestdist = -1
    primary = 0
    secondary = 1
    tertiary = 2
    for i in range(0, len(poses)):
        cand = poses[i]
        dist1 = math.sqrt(
            math.pow(mnext[primary][0]-cand[primary][0], 2) +
            math.pow(mnext[primary][1]-cand[primary][1], 2) +
            math.pow(mnext[primary][2]-cand[primary][2], 2))
        dist2 = math.sqrt(
            math.pow(mnext[secondary][0]-cand[secondary][0], 2) +
            math.pow(mnext[secondary][1]-cand[secondary][1], 2) +
            math.pow(mnext[secondary][2]-cand[secondary][2], 2))
        dist3 = math.sqrt(
            math.pow(mnext[tertiary][0]-cand[tertiary][0], 2) +
            math.pow(mnext[tertiary][1]-cand[tertiary][1], 2) +
            math.pow(mnext[tertiary][2]-cand[tertiary][2], 2))
        dist = dist1*dist1+dist2*dist2+dist3*dist3
        if best == -1:
            bestdist = dist
            best = i
        if dist < bestdist:
            # print "NB", dist, i, dist-bestdist
            bestdist = dist
            best = i
    return best
