#!/usr/bin/python
# -*- coding: utf-8 -*-
import os, shutil, subprocess, glob, fnmatch


def material_filter_copy(from_cmat, to_cmat, MatNames):
    relPath=to_cmat.replace("\\", "/").rpartition("/")[0]+"/"
    outFile=open(to_cmat, 'w')
    isInMat=False
    LineCount=0
    DoneMatNames=[]

    for line in open(from_cmat):
        for MatName in MatNames:
            if line.startswith(MatName) or line.startswith('"'+MatName):
                assert(not isInMat)
                # print MatName, " ### ", line
                isInMat=True
                DoneMatNames+=[MatName]
                break

        # If there were comments in the file, we would unintentionally and involuntarily drop them...
        assert(not line.startswith("//"))

        if isInMat:
            line=line.replace("map "+relPath, "map ")
            line=line.replace("map  "+relPath, "map  ")
            line=line.replace("map   "+relPath, "map   ")
            line=line.replace("map    "+relPath, "map    ")
            line=line.replace("map     "+relPath, "map     ")
            outFile.write(line)
            LineCount+=1

            if line.strip()=="}":
                outFile.write("\n")
                isInMat=False

    if LineCount==0:
        print "######################";
        # Make a full, unfiltered copy of the file:
        # for line in open(from_cmat):
        #     print line.strip()

    for MatName in DoneMatNames:
        if MatName in MatNames:
            MatNames.remove(MatName)

    for MatName in MatNames:
        print "            #### ", MatName, " not found in ", from_cmat

    outFile.close()


def find_files(directory, pattern):
    for root, dirs, files in os.walk(directory):
        for basename in files:
            if fnmatch.fnmatch(basename, pattern):
                filename = os.path.join(root, basename)
                yield filename


for filename in find_files('Materials\\Models', '*.cmat'):
    dirname = filename.replace("Materials\\", "").replace(".cmat", "")

    if dirname=="Models\\Vegetation-Bushes":    # Special-case...
        dirname="Models\\Vegetation"

    print "\n", filename

    if os.path.isdir(dirname):
        dirname+="\\*"
    elif dirname=="Models\\TechDemo\\Lamps":    # Special-case...
        dirname="Models\\TechDemo\\lamp*"
    else:
        raise Exception("Unexpected dirname "+dirname)

    for mdl in glob.glob(dirname):      # glob is non-recursive
        if mdl.endswith(".png"): continue
        if mdl.endswith(".jpg"): continue
        if mdl.endswith(".tga"): continue
        if mdl.endswith(".cmap"): continue
        if mdl.endswith(".dlod"): continue
        if mdl.endswith(".txt"): continue
        if mdl.endswith(".cmat"): continue  # The files that we generate here.

        if mdl.endswith("T.mdl"):
            if os.path.exists(mdl.replace("T.mdl", ".mdl")):
                continue

        if not (mdl.endswith(".mdl") or mdl.endswith(".ase")):
            raise Exception("Unexpected file "+mdl)

        new_cmat=mdl.replace(".ase", ".cmat").replace(".mdl", ".cmat")
        print "    --> ", new_cmat

        if mdl.endswith(".mdl"):
            MatNames=[new_cmat.replace(".cmat", "").replace("\\", "/")]   # For mdl, it's really just the significant "prefix" of the material name
        else:   # endswith(".ase")
            MatNames=[]
            for line in open(mdl):
                if line.find("*MATERIAL_NAME")!=-1:
                    mat=line.replace("*MATERIAL_NAME", "").strip().strip('"')
                    assert(mat!="")
                    MatNames+=[mat]

        material_filter_copy(filename, new_cmat, MatNames)

