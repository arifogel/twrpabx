/*
 * Copyright 2018 Ari Fogel <ari@fogelti.me>
 * Released under Apache 2.0 license
 */

#ifndef __TWRPABX_H__
#define __TWRPABX_H__

#include <stdio.h>

#define TWRPABX_BLOCK_SIZE (512)
#define TWRPABX_NAME_SIZE (468)
#define TWRPABX_TYPE_SIZE (16)

class twrpabx {
public:
  static int Main(int argc, char** argv);
  static void ReadBackupStreamHeader(FILE* input_file, size_t* size_remaining);
  static bool ReadFile(FILE* input_file, size_t* size_remaining);
  static bool ReadChunk(FILE* input_file, FILE* output_file, char* buf, size_t* size_remaining);
};


#endif
