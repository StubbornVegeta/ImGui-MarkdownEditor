#pragma once

#ifndef _LOADIMAGE_H
#define _LOADIMAGE_H


typedef unsigned int GLuint;

#define GL_CLAMP_TO_EDGE 0x812F


bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);

#endif
