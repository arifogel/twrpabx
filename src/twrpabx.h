/*
 * Copyright 2018 Ari Fogel <ari@fogelti.me>
 * Released under Apache 2.0 license
 */

#ifndef __TWRPABX_H__
#define __TWRPABX_H__

#include <stdio.h>

size_t BLOCK_SIZE = 512;
size_t NAME_SIZE = 468;
size_t TYPE_SIZE = 16;

class twrpabx {
public:
  static void ReadBackupStreamHeader(FILE* input_file, size_t* size_remaining);
  static bool ReadFile(FILE* input_file, size_t* size_remaining);
  static void ReadChunk(FILE* input_file, FILE* output_file, char* buf, size_t* size_remaining);
};


#endif
