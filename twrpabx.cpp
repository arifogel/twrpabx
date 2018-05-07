/*
 * Copyright 2018 Ari Fogel <ari@fogelti.me>
 * Released under Apache 2.0 license
 */

#include <string>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include "twadbstream.h"
//#include "libtwadbbu.hpp"
#include "twrpabx.h"

void twrpabx::ReadBackupStreamHeader(FILE* input_file, size_t* size_remaining) {
  char buf[BLOCK_SIZE] = {0};
  size_t bytes_read = fread((void*)buf, 1, BLOCK_SIZE, input_file);
  printf("Read %zu bytes\n", bytes_read);
  if (bytes_read < BLOCK_SIZE) {
    fprintf(stderr, "Failed to read TW ADB Backup Header\n");
    exit(1);
  }
  *size_remaining -= bytes_read;

  char strbuf[64] = {0};

  AdbBackupStreamHeader* twAdbBackupHeader = (AdbBackupStreamHeader*)buf;
  if (strncmp(twAdbBackupHeader->start_of_header, "TWRP", 4)) {
    fprintf(stderr, "Corrupt start of backup header");
    exit(1);
  }

  memcpy(strbuf, twAdbBackupHeader->type, TYPE_SIZE);
  printf("TW ADB Backup Header type: %s\n", strbuf);

  printf("Number of partitions: %" PRIu64 "\n", twAdbBackupHeader->partition_count);

}

void twrpabx::ReadChunk(FILE* input_file, FILE* output_file, char* buf, size_t* size_remaining) {
  size_t bytes_read = fread((void*)buf, 1, BLOCK_SIZE, input_file);
  printf("Read %zu bytes\n", bytes_read);
  if (bytes_read < BLOCK_SIZE) {
    fprintf(stderr, "Failed to read data header\n");
    exit(1);
  }
  *size_remaining -= bytes_read;

  char strbuf[512] = {0};

  AdbBackupControlType* control_block = (AdbBackupControlType*)buf;
  if (strncmp(control_block->start_of_header, "TWRP", 4)) {
    fprintf(stderr, "Corrupt start of data header\n");
    exit(1);
  }

  memcpy(strbuf, control_block->type, TYPE_SIZE);
  printf("Data header type: %s\n", strbuf);

  size_t max_data = DATA_MAX_CHUNK_SIZE - BLOCK_SIZE;
  char databuf[max_data];
  size_t to_read = *size_remaining < max_data - 1024 ? *size_remaining - 1024 : max_data;
  //size_t to_read = max_data;
  bytes_read = fread((void*)databuf, 1, to_read, input_file);
  printf("Read %zu bytes\n", bytes_read);
  if (bytes_read < to_read) {
    fprintf(stderr, "Failed to read. Only read %zu bytes of %zu\n", bytes_read, to_read);
    exit(1);
  }
  *size_remaining -= bytes_read;
  size_t bytes_written = fwrite((void*)databuf, 1, bytes_read, output_file);
  if (bytes_written < bytes_read) {
    fprintf(stderr, "Failed to write. Only wrote %zu bytes of %zu\n", bytes_written, bytes_read);
    exit(1);
  }
  printf("Wrote %zu bytes\n", bytes_written);
  printf("Remaining bytes in backup file: %zu\n", *size_remaining);
}

bool twrpabx::ReadFile(FILE* input_file, size_t* size_remaining) {
  char buf[BLOCK_SIZE] = {0};
  size_t bytes_read = fread((void*)buf, 1, BLOCK_SIZE, input_file);
  printf("Read %zu bytes\n", bytes_read);
  if (bytes_read < BLOCK_SIZE) {
    fprintf(stderr, "Failed to read file header\n");
    exit(1);
  }
  *size_remaining -= bytes_read;

  char strbuf[512] = {0};

  twfilehdr* file_header = (twfilehdr*)buf;
  if (strncmp(file_header->start_of_header, "TWRP", 4)) {
    fprintf(stderr, "Corrupt start of file header\n");
    exit(1);
  }

  memcpy(strbuf, file_header->type, TYPE_SIZE);

  if (!strncmp(TWENDADB, strbuf, TYPE_SIZE)) {
  printf("End ADB type: %s\n", strbuf);
    return false;
  }

  if (strncmp(TWFN, strbuf, TYPE_SIZE)) {
    fprintf(stderr, "Unexpected type: %s\n", strbuf);
    exit(1);
  }

  printf("File Header type: %s\n", strbuf);

  printf("File size: %" PRIu64 "\n", *size_remaining);

  memcpy(strbuf, file_header->name, NAME_SIZE);
  printf("Raw file name: %s\n", strbuf);
  char* last_element = strtok(strbuf, "/");
  char* current_element;
  while ((current_element = strtok(NULL, "/")) != NULL) {
     last_element = current_element;
  };
  printf("File name: %s\n", last_element);

  FILE* output_file = fopen(last_element, "wb");

  do {
   twrpabx::ReadChunk(input_file, output_file, buf, size_remaining);
  } while (*size_remaining > 1024);

  if (fclose(output_file) < 0) {
    perror("Failed to close output file");
    exit(1);
  }

  printf("Current offset: %lu\n", ftell(input_file));

  /* trailer */

  bytes_read = fread((void*)buf, 1, BLOCK_SIZE, input_file);
  printf("Read %zu bytes\n", bytes_read);
  if (bytes_read < BLOCK_SIZE) {
    fprintf(stderr, "Failed to read file trailer\n");
    exit(1);
  }
  *size_remaining -= bytes_read;

  AdbBackupFileTrailer* trailer = (AdbBackupFileTrailer*)buf;
  if (strncmp(file_header->start_of_header, "TWRP", 4)) {
    fprintf(stderr, "Corrupt start of file trailer\n");
    exit(1);
  }

  memcpy(strbuf, file_header->type, TYPE_SIZE);
  printf("File trailer type: %s\n", strbuf);

  return true;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Expected exactly 1 argument: <input_file>\n");
    exit(1);
  }

  char* filename = argv[1];
  printf("Input file: '%s'\n", filename);

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

