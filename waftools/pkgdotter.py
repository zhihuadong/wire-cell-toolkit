#!/usr/bin/env python
'''
This a dirty hack
'''

import sys

from collections import defaultdict

class Faker(object):
    def __init__(self):
        
        self.deps = dict()
        for typ in 'lib app test'.split():
            self.deps[typ] = defaultdict(set)

    def register(self, typ, name, uselst):
        if type(uselst) == type(""):
            uselst = uselst.split()
        for one in uselst:
            self.deps[typ][name].add(one)
                

    def smplpkg(self, name, use='', test_use='', app_use=''):
        '''
        Fake being the waf bld method of the same name.  The 'use'
        args can be litteral lists or space separated strings and hold
        names of packages on which the 'name' pacakge depends.
        '''
        self.register('lib', name, use)

        self.register('app', name, use)
        self.register('app', name, app_use)

        self.register('test', name, use)
        self.register('test', name, test_use)

    def dot(self, typ):
        preamble=[
            'node[shape=box];',
            'label="Wire Cell Package Dependencies for view: %s";' % typ.upper(),
        ]

        edges = list()
        d = self.deps[typ]
        exts = set()
        for n,deps in d.items():
            for dep in deps:
                edges.append('%s -> %s;' % (n, dep))
                if not dep.startswith("WireCell"):
                    exts.add(dep)
        
        exts = ','.join(list(exts))
        preamble.append('{rank=same; %s}' % exts)
            
        preamble = '\n\t'.join(preamble)
        body = '\n\t'.join(edges)
        return 'digraph %s {\n\t%s\n\t%s\n}\n' % (typ, preamble, body)


bld = Faker()

which = sys.argv[1]
wscrip_builds = sys.argv[2:]

for wb in wscrip_builds:
    execfile (wb)

print (bld.dot(which))

