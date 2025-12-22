#ifndef BATCH_PROCESSING_H
#define BATCH_PROCESSING_H

#include <stdint.h>
#include <stdlib.h>
#include "cli.h"

// Structure to hold a list of file paths
typedef struct {
    char **paths;       // Array of file paths
    size_t count;       // Number of files
    size_t capacity;    // Allocated capacity
} file_list_t;

// Structure to hold strategy usage statistics
typedef struct {
    char name[64];              // Strategy name
    int success_count;          // Number of successful applications
    int failure_count;          // Number of failed applications
    size_t total_output_size;   // Total output size from this strategy
    double avg_output_size;     // Average output size per application
} strategy_stats_t;

// Structure to hold file complexity statistics
typedef struct {
    char *path;                 // File path
    size_t input_size;          // Input file size
    size_t output_size;         // Output file size
    double size_ratio;          // Output/input ratio
    int instruction_count;      // Number of instructions processed
    int bad_char_count;         // Number of bad characters eliminated
    int success;                // Whether processing was successful
} file_complexity_stats_t;

// Structure to hold batch processing statistics
typedef struct {
    size_t total_files;
    size_t processed_files;
    size_t failed_files;
    size_t skipped_files;
    size_t total_input_bytes;
    size_t total_output_bytes;
    // List of failed files
    char **failed_file_list;
    size_t failed_file_count;
    size_t failed_file_capacity;
    // Bad character statistics
    int bad_char_count;
    int bad_char_set[256];  // Use int to match ml_metrics.h session_metrics_t bad_char_set type
    // Strategy usage statistics
    strategy_stats_t *strategy_stats;
    size_t strategy_count;
    size_t strategy_capacity;
    // File complexity statistics
    file_complexity_stats_t *file_stats;
    size_t file_count;
    size_t file_capacity;
} batch_stats_t;

// Function to check if a path is a directory
int is_directory(const char *path);

// Function to check if a filename matches a pattern (simple wildcard matching)
int match_pattern(const char *filename, const char *pattern);

// Initialize a file list
void file_list_init(file_list_t *list);

// Add a file to the list
int file_list_add(file_list_t *list, const char *path);

// Free a file list
void file_list_free(file_list_t *list);

// Find all files in a directory matching the pattern
int find_files(const char *dir_path, const char *pattern, int recursive, file_list_t *list);

// Construct output path from input path
char* construct_output_path(const char *input_path, const char *input_base,
                           const char *output_base, int preserve_structure);

// Initialize batch statistics
void batch_stats_init(batch_stats_t *stats);

// Add a failed file to the statistics
int batch_stats_add_failed_file(batch_stats_t *stats, const char *failed_file_path);

// Write failed files to output file
int batch_write_failed_files(const batch_stats_t *stats, const char *output_file);

// Free batch statistics resources
void batch_stats_free(batch_stats_t *stats);

// Print batch statistics
void batch_stats_print(const batch_stats_t *stats, int quiet);

// Add strategy usage statistics
int batch_stats_add_strategy_usage(batch_stats_t *stats, const char *strategy_name, int success, size_t output_size);

// Add file complexity statistics
int batch_stats_add_file_stats(batch_stats_t *stats, const char *file_path, size_t input_size,
                               size_t output_size, int instruction_count, int bad_char_count, int success);

#endif // BATCH_PROCESSING_H
