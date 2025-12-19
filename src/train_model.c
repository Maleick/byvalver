/**
 * @file train_model.c
 * @brief Utility to train the ML model for BYVALVER
 *
 * This utility program runs the complete training pipeline to generate
 * the ML model file required by the main application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "training_pipeline.h"

static void print_train_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] TRAINING_DATA_DIR\n\n", program_name);
    printf("Train the BYVALVER ML model using shellcode samples.\n\n");
    printf("Arguments:\n");
    printf("  TRAINING_DATA_DIR    Directory containing .bin training files (required)\n\n");
    printf("Options:\n");
    printf("  -h, --help           Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s /path/to/shellcodes          # Train with shellcodes directory\n", program_name);
    printf("  %s ~/my_training_data           # Train with home directory path\n", program_name);
    printf("  %s ./training_bins              # Train with relative path\n\n", program_name);
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    const char* training_dir = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_train_usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            printf("[ERROR] Unknown option: %s\n", argv[i]);
            print_train_usage(argv[0]);
            return 1;
        } else {
            if (training_dir != NULL) {
                printf("[ERROR] Multiple directories specified\n");
                print_train_usage(argv[0]);
                return 1;
            }
            training_dir = argv[i];
        }
    }

    printf("BYVALVER ML Model Training Utility\n");
    printf("==================================\n\n");

    // Initialize training configuration
    training_config_t config;
    if (training_pipeline_init_config(&config) != 0) {
        printf("[ERROR] Failed to initialize training configuration\n");
        return 1;
    }

    // Set training directory (required)
    if (training_dir == NULL) {
        printf("[ERROR] Training data directory is required\n\n");
        print_train_usage(argv[0]);
        return 1;
    }

    strncpy(config.training_data_dir, training_dir, sizeof(config.training_data_dir) - 1);
    config.training_data_dir[sizeof(config.training_data_dir) - 1] = '\0';

    printf("Training data directory: %s\n", config.training_data_dir);
    printf("Model output path: %s\n", config.model_output_path);
    printf("Max training samples: %zu\n", config.max_training_samples);
    printf("Training epochs: %d\n", config.epochs);
    printf("\n");

    // Execute the complete training pipeline
    int result = training_pipeline_execute(&config);

    if (result == 0) {
        printf("\n[SUCCESS] ML model training completed successfully!\n");
        printf("Model saved to: %s\n", config.model_output_path);
    } else {
        printf("\n[ERROR] ML model training failed with code: %d\n", result);
        return 1;
    }

    return 0;
}