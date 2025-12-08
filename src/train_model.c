/**
 * @file train_model.c
 * @brief Utility to train the ML model for BYVALVER
 *
 * This utility program runs the complete training pipeline to generate
 * the ML model file required by the main application.
 */

#include <stdio.h>
#include <stdlib.h>
#include "training_pipeline.h"

int main(int argc __attribute__((unused)), char* argv[] __attribute__((unused))) {
    printf("BYVALVER ML Model Training Utility\n");
    printf("==================================\n\n");

    // Initialize training configuration
    training_config_t config;
    if (training_pipeline_init_config(&config) != 0) {
        printf("[ERROR] Failed to initialize training configuration\n");
        return 1;
    }

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