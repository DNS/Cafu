cmake_minimum_required(VERSION 3.9)

project("minizip")

add_library(minizip STATIC unzip.c ioapi.c)
