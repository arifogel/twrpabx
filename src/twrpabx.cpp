/*
 * Copyright 2018 Ari Fogel <ari@fogelti.me>
 * Released under Apache 2.0 license
 */

#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>

#include "twadbstream.h"
#include "twrpabx.h"

using namespace std;



int twrpabx::Main(int argc, char** argv) {
  if (argc != 2) {
    cerr << "Expected exactly 1 argument: <input_file>" << endl;
    exit(1);
  }

  char* filename = argv[1];
  cout << "Input file: '" << filename << "'" << endl;

  FILE* input_file = fopen(filename, "rb");
  if (input_file == NULL) {
    perror("Failed to open input file");
    exit(1);
  }

  fseek(input_file, 0L, SEEK_END);
  size_t size_remaining = ftell(input_file);
  fseek(input_file, 0L, SEEK_SET);

  twrpabx::ReadBackupStreamHeader(input_file, &size_remaining);

  while (twrpabx::ReadFile(input_file, &size_remaining)) {};

  if (fclose(input_file) == EOF) {
    perror("Failed to close input file");
    exit(1);
  }

  return 0;
}

void twrpabx::ReadBackupStreamHeader(FILE* input_file, size_t* size_remaining) {
  char* buf = (char*)calloc(TWRPABX_BLOCK_SIZE, 1);
  size_t bytes_read = fread((void*)buf, 1, TWRPABX_BLOCK_SIZE, input_file);
  cout << "Read " << bytes_read << " bytes" << endl;
  if (bytes_read < TWRPABX_BLOCK_SIZE) {
    cerr << "Failed to read TW ADB Backup Header\n";
    exit(1);
  }
  *size_remaining -= bytes_read;

  char* strbuf = (char*)calloc(TWRPABX_TYPE_SIZE,1);
  if (strbuf == NULL) {
    cerr << "Failed to allocate 64-byte strbuf: " << strerror(errno) << endl;
  }

  AdbBackupStreamHeader* twAdbBackupHeader = (AdbBackupStreamHeader*)buf;
  if (strncmp(twAdbBackupHeader->start_of_header, "TWRP", 4)) {
    cerr << "Corrupt start of backup header" << endl;
    exit(1);
  }

  memcpy(strbuf, twAdbBackupHeader->type, TWRPABX_TYPE_SIZE);
  cout << "TW ADB Backup Header type: " << strbuf << endl;
  cout << "Number of partitions: " << twAdbBackupHeader->partition_count << endl;
  free(buf);
  free(strbuf);
}

void twrpabx::ReadChunk(FILE* input_file, FILE* output_file, char* buf, size_t* size_remaining) {
  size_t bytes_read = fread((void*)buf, 1, TWRPABX_BLOCK_SIZE, input_file);
  cout << "Read " << bytes_read << "  bytes" << endl;
  if (bytes_read < TWRPABX_BLOCK_SIZE) {
    cerr << "Failed to read data header" << endl;
    exit(1);
  }
  *size_remaining -= bytes_read;

  char strbuf[512] = {0};

  AdbBackupControlType* control_block = (AdbBackupControlType*)buf;
  if (strncmp(control_block->start_of_header, "TWRP", 4)) {
    cerr << "Corrupt start of data header" << endl;
    exit(1);
  }

  memcpy(strbuf, control_block->type, TWRPABX_TYPE_SIZE);
  cout << "Data header type: " << strbuf << endl;

  size_t max_data = DATA_MAX_CHUNK_SIZE - TWRPABX_BLOCK_SIZE;
  char databuf[max_data];
  size_t to_read = *size_remaining < max_data - 1024 ? *size_remaining - 1024 : max_data;
  //size_t to_read = max_data;
  bytes_read = fread((void*)databuf, 1, to_read, input_file);
  cout << "Read " << bytes_read << "  bytes" << endl;
  if (bytes_read < to_read) {
    cerr << "Failed to read. Only read " << bytes_read << " bytes of " << to_read << endl;
    exit(1);
  }
  *size_remaining -= bytes_read;
  size_t bytes_written = fwrite((void*)databuf, 1, bytes_read, output_file);
  if (bytes_written < bytes_read) {
    cerr << "Failed to write. Only wrote " << bytes_written << " bytes of " << bytes_read << endl;
    exit(1);
  }
  cout << "Wrote " << bytes_written << "  bytes" << endl;
  cout << "Remaining bytes in backup file: " << *size_remaining << endl;
}

bool twrpabx::ReadFile(FILE* input_file, size_t* size_remaining) {
  char* buf = (char*)calloc(TWRPABX_BLOCK_SIZE, 1);
  if (buf == NULL) {
    cerr << "Could not allocate " << TWRPABX_BLOCK_SIZE << "-byte buffer" << endl;
    exit(1);
  }
  size_t bytes_read = fread((void*)buf, 1, TWRPABX_BLOCK_SIZE, input_file);
  cout << "Read " << bytes_read << " bytes" << endl;
  if (bytes_read < TWRPABX_BLOCK_SIZE) {
    cerr << "Failed to read file header" << endl;
    exit(1);
  }
  *size_remaining -= bytes_read;

  char* strbuf = (char*)calloc(TWRPABX_BLOCK_SIZE, 1);
  if (strbuf == NULL) {
    cerr << "Could not allocate " << TWRPABX_BLOCK_SIZE << "-byte string buffer" << endl;
    exit(1);
  }

  twfilehdr* file_header = (twfilehdr*)buf;
  if (strncmp(file_header->start_of_header, "TWRP", 4)) {
    cerr << "Corrupt start of file header" << endl;
    exit(1);
  }

  memcpy(strbuf, file_header->type, TWRPABX_TYPE_SIZE);

  if (!strncmp(TWENDADB, strbuf, TWRPABX_TYPE_SIZE)) {
    cout << "End ADB type: " << strbuf << endl;
    free(buf);
    free(strbuf);
    return false;
  }

  if (strncmp(TWFN, strbuf, TWRPABX_TYPE_SIZE)) {
    cerr << "Unexpected type: " << strbuf << endl;
    exit(1);
  }

  cout << "File Header type: " << strbuf << endl;

  cout << "File size: " << *size_remaining << "\n";

  memcpy(strbuf, file_header->name, TWRPABX_NAME_SIZE);
  cout << "Raw file name: " << strbuf << endl;
  char* last_element = strtok(strbuf, "/");
  char* current_element;
  while ((current_element = strtok(NULL, "/")) != NULL) {
     last_element = current_element;
  };
  cout << "File name: %s" <<  last_element << endl;

  FILE* output_file = fopen(last_element, "wb");

  do {
   twrpabx::ReadChunk(input_file, output_file, buf, size_remaining);
  } while (*size_remaining > 1024);

  if (fclose(output_file) < 0) {
    perror("Failed to close output file");
    exit(1);
  }

  cout << "Current offset: " << ftell(input_file) << endl;

  /* trailer */

  bytes_read = fread((void*)buf, 1, TWRPABX_BLOCK_SIZE, input_file);
  cout << "Read " << bytes_read << " bytes" << endl;
  if (bytes_read < TWRPABX_BLOCK_SIZE) {
    cerr << "Failed to read file trailer" << endl;
    exit(1);
  }
  *size_remaining -= bytes_read;

  AdbBackupFileTrailer* trailer = (AdbBackupFileTrailer*)buf;
  if (strncmp(file_header->start_of_header, "TWRP", 4)) {
    cerr << "Corrupt start of file trailer" << endl;
    exit(1);
  }

  memcpy(strbuf, file_header->type, TWRPABX_TYPE_SIZE);
  cout << "File trailer type: " << strbuf << endl;

  free(buf);
  free(strbuf);
  return true;
}

