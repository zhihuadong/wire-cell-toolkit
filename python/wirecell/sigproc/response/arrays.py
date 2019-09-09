#!/usr/bin/env python3
'''
Deal with responses as numpy arrays
'''

import numpy
from wirecell import units

def pr2array(pr, nimperwire = 6, nbinsperwire = 10):
    '''
    Convert a schema.PlaneResponse to a numpy array
    '''
    nwires = len(pr.paths) // nimperwire
    midwire = nwires//2

    nticks = pr.paths[0].current.size
    nimps = int(nwires*nbinsperwire)
    res = numpy.zeros((nimps, nticks))
    pitches = numpy.zeros(nimps)

    for iwire in range(nwires):
        ibin0 = iwire * nimperwire
        for ind in range(nimperwire-1):
            a = pr.paths[ibin0 + ind+0].current
            b = pr.paths[ibin0 + ind+1].current
            m = 0.5 * (a+b)

            p1 = pr.paths[ibin0 + ind+0].pitchpos
            p2 = pr.paths[ibin0 + ind+1].pitchpos
            pm = 0.5*(p1+p2)
            
            obin = iwire * nbinsperwire + ind;

            res[obin] = m
            pitches[obin] = pm

    res = res + numpy.flipud(res)
    pitches = pitches - numpy.flip(pitches)
        
    # for path in pr.paths:
    #     print ("%.3f mm"%(path.pitchpos/units.mm))
    return res,pitches

def savez(fr, npz_filename):
    '''
    Save a schema.FieldResponse to an npzfile
    '''
    nplanes = len(fr.planes)
    planeid = numpy.zeros(nplanes)

    dat = dict(otps=numpy.array([fr.origin,fr.tstart,fr.period,fr.speed]),
               locations=numpy.zeros(nplanes),
               pitches=numpy.zeros(nplanes))

    for iplane, pr in enumerate(fr.planes):
        r,p = pr2array(pr)
        dat['resp%d' % pr.planeid] = r
        dat['bincenters%d' % pr.planeid] = p
        dat['locations'][iplane] = pr.location
        dat['pitches'][iplane] = pr.pitch
    
    numpy.savez(npz_filename, **dat)
