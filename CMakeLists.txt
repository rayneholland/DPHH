cmake_minimum_required(VERSION 3.29)
project(DPHH)

set(CMAKE_CXX_STANDARD 20)

add_executable(DPHH main.cpp
        hash/Murmurhash.h
        sketch/Sketch.h
        sketch/CMS.h
        sketch/CS.h
        heavy/sketchHH.h
        heap/IndexMinHeap.h
        sketch/CMSSO.h
        heavy/CMSSOHH.h
        heavy/SpaceSaving.h
        heavy/MisraGries.h
        help/nodes.h
        heavy/SSSO.h
        heavy/MGSO.h
        sketch/CSSO.h
        heavy/CSSOHH.h
)
