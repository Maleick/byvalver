#include "ml_metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Initialize metrics tracker
ml_metrics_tracker_t* ml_metrics_init(const char* metrics_file) {
    ml_metrics_tracker_t* tracker = (ml_metrics_tracker_t*)calloc(1, sizeof(ml_metrics_tracker_t));
    if (!tracker) {
        fprintf(stderr, "[METRICS] Failed to allocate metrics tracker\n");
        return NULL;
    }

    tracker->metrics_enabled = 1;
    tracker->strategy_count = 0;

    if (metrics_file) {
        strncpy(tracker->metrics_file_path, metrics_file, sizeof(tracker->metrics_file_path) - 1);
    } else {
        strncpy(tracker->metrics_file_path, "./ml_metrics.log", sizeof(tracker->metrics_file_path) - 1);
    }

    // Initialize session
    ml_metrics_start_session(tracker);

    // Initialize learning metrics
    tracker->learning.learning_enabled = 0;
    tracker->learning.total_feedback_iterations = 0;
    tracker->learning.total_weight_delta = 0.0;
    tracker->learning.max_weight_delta = 0.0;

    printf("[METRICS] ML metrics tracker initialized: %s\n", tracker->metrics_file_path);

    return tracker;
}

// Cleanup metrics tracker
void ml_metrics_cleanup(ml_metrics_tracker_t* tracker) {
    if (!tracker) return;

    ml_metrics_end_session(tracker);
    ml_metrics_export_to_file(tracker, tracker->metrics_file_path);

    printf("[METRICS] Metrics tracker cleaned up\n");
    free(tracker);
}

// Find or create strategy metrics entry
static strategy_metrics_t* find_or_create_strategy(ml_metrics_tracker_t* tracker, const char* strategy_name) {
    if (!tracker || !strategy_name) return NULL;

    // Search for existing entry
    for (int i = 0; i < tracker->strategy_count; i++) {
        if (strcmp(tracker->strategy_metrics[i].strategy_name, strategy_name) == 0) {
            return &tracker->strategy_metrics[i];
        }
    }

    // Create new entry
    if (tracker->strategy_count >= MAX_TRACKED_STRATEGIES) {
        fprintf(stderr, "[METRICS] Maximum tracked strategies reached\n");
        return NULL;
    }

    strategy_metrics_t* new_metric = &tracker->strategy_metrics[tracker->strategy_count++];
    strncpy(new_metric->strategy_name, strategy_name, sizeof(new_metric->strategy_name) - 1);
    new_metric->min_confidence = 1.0;
    new_metric->max_confidence = 0.0;

    return new_metric;
}

// Record strategy attempt
void ml_metrics_record_strategy_attempt(ml_metrics_tracker_t* tracker,
                                       const char* strategy_name,
                                       double confidence) {
    if (!tracker || !tracker->metrics_enabled) return;

    strategy_metrics_t* metric = find_or_create_strategy(tracker, strategy_name);
    if (!metric) return;

    metric->times_attempted++;
    metric->total_confidence += confidence;

    if (confidence < metric->min_confidence) {
        metric->min_confidence = confidence;
    }
    if (confidence > metric->max_confidence) {
        metric->max_confidence = confidence;
    }
}

// Record strategy result (v3.0 with bad byte support)
void ml_metrics_record_strategy_result_v3(ml_metrics_tracker_t* tracker,
                                          const char* strategy_name,
                                          int success,
                                          int bad_bytes_eliminated,
                                          int size_increase,
                                          double processing_time_ms) {
    if (!tracker || !tracker->metrics_enabled) return;

    strategy_metrics_t* metric = find_or_create_strategy(tracker, strategy_name);
    if (!metric) return;

    if (success) {
        metric->times_succeeded++;
        metric->nulls_eliminated += bad_bytes_eliminated;  // Backward compatibility
        metric->bad_bytes_eliminated += bad_bytes_eliminated;
        tracker->session.total_nulls_eliminated += bad_bytes_eliminated;  // Backward compatibility
        tracker->session.total_bad_bytes_eliminated += bad_bytes_eliminated;
    } else {
        metric->times_failed++;
    }

    metric->total_output_size_increase += size_increase;
    metric->total_processing_time_ms += processing_time_ms;

    tracker->session.total_strategies_applied++;
}

// Record strategy result (backward compatibility)
void ml_metrics_record_strategy_result(ml_metrics_tracker_t* tracker,
                                      const char* strategy_name,
                                      int success,
                                      int nulls_eliminated,           // DEPRECATED: Use bad_bytes_eliminated
                                      int size_increase,
                                      double processing_time_ms) {
    // Call the new function with the same values for backward compatibility
    ml_metrics_record_strategy_result_v3(tracker, strategy_name, success, nulls_eliminated, size_increase, processing_time_ms);
}

// Record bad byte configuration
void ml_metrics_record_bad_byte_config(ml_metrics_tracker_t* tracker,
                                       uint8_t* bad_bytes,
                                       int bad_byte_count) {
    if (!tracker || !tracker->metrics_enabled) return;

    tracker->session.bad_byte_count = bad_byte_count;

    // Copy the bad byte bitmap
    for (int i = 0; i < 256; i++) {
        tracker->session.bad_byte_set[i] = bad_bytes ? bad_bytes[i] : 0;
    }
}

// Record instruction processed with bad byte tracking (backward compatibility)
void ml_metrics_record_instruction_processed(ml_metrics_tracker_t* tracker,
                                            int nulls_in_instruction) {
    if (!tracker || !tracker->metrics_enabled) return;

    tracker->session.total_instructions_processed++;
    tracker->session.total_null_bytes_original += nulls_in_instruction;  // Backward compatibility
    // In v3.0, we'd call a function to count all bad chars in instruction
    // For backward compatibility, we'll still record the nulls
}

// Record instruction processed with detailed bad byte tracking (v3.0)
void ml_metrics_record_instruction_processed_v3(ml_metrics_tracker_t* tracker,
                                                uint8_t* bad_bytes_in_instruction,
                                                int bad_byte_count) {
    if (!tracker || !tracker->metrics_enabled) return;

    tracker->session.total_instructions_processed++;

    // Count total bad chars in the instruction for the session
    tracker->session.total_bad_bytes_original += bad_byte_count;
    tracker->session.total_null_bytes_original += bad_bytes_in_instruction ? bad_bytes_in_instruction[0x00] : 0; // Backward compatibility

    // Update the bad byte breakdown in session
    for (int i = 0; i < 256; i++) {
        if (bad_bytes_in_instruction && bad_bytes_in_instruction[i]) {
            tracker->session.bad_byte_set[i] = 1;  // Mark this bad char as present in input
        }
    }
}

// Record feedback iteration
void ml_metrics_record_feedback(ml_metrics_tracker_t* tracker,
                               int positive,
                               double weight_delta) {
    if (!tracker || !tracker->metrics_enabled) return;

    tracker->learning.total_feedback_iterations++;

    if (positive) {
        tracker->learning.positive_feedback_count++;
    } else {
        tracker->learning.negative_feedback_count++;
    }

    tracker->learning.total_weight_delta += fabs(weight_delta);

    if (fabs(weight_delta) > tracker->learning.max_weight_delta) {
        tracker->learning.max_weight_delta = fabs(weight_delta);
    }

    if (tracker->learning.total_feedback_iterations > 0) {
        tracker->learning.avg_weight_delta =
            tracker->learning.total_weight_delta / tracker->learning.total_feedback_iterations;
    }

    tracker->learning.last_learning_timestamp = time(NULL);
}

// Record learning iteration
void ml_metrics_record_learning_iteration(ml_metrics_tracker_t* tracker,
                                         double avg_weight_change,
                                         double max_weight_change) {
    if (!tracker || !tracker->metrics_enabled) return;

    tracker->learning.total_feedback_iterations++;
    tracker->learning.total_weight_delta += avg_weight_change;

    if (max_weight_change > tracker->learning.max_weight_delta) {
        tracker->learning.max_weight_delta = max_weight_change;
    }

    tracker->learning.last_learning_timestamp = time(NULL);
}

// Record prediction
void ml_metrics_record_prediction(ml_metrics_tracker_t* tracker,
                                 int correct,
                                 double confidence) {
    if (!tracker || !tracker->metrics_enabled) return;

    tracker->model.predictions_made++;

    if (correct) {
        tracker->model.correct_predictions++;
    } else {
        tracker->model.incorrect_predictions++;
    }

    tracker->model.avg_prediction_confidence =
        (tracker->model.avg_prediction_confidence * (tracker->model.predictions_made - 1) + confidence)
        / tracker->model.predictions_made;

    if (tracker->model.predictions_made > 0) {
        tracker->model.current_accuracy =
            (double)tracker->model.correct_predictions / tracker->model.predictions_made;

        if (tracker->model.initial_accuracy == 0.0) {
            tracker->model.initial_accuracy = tracker->model.current_accuracy;
        }

        tracker->model.accuracy_improvement =
            tracker->model.current_accuracy - tracker->model.initial_accuracy;
    }
}

// Update model accuracy
void ml_metrics_update_model_accuracy(ml_metrics_tracker_t* tracker,
                                     double new_accuracy) {
    if (!tracker || !tracker->metrics_enabled) return;

    if (tracker->model.initial_accuracy == 0.0) {
        tracker->model.initial_accuracy = new_accuracy;
    }

    tracker->model.current_accuracy = new_accuracy;
    tracker->model.accuracy_improvement = new_accuracy - tracker->model.initial_accuracy;
}

// Start session
void ml_metrics_start_session(ml_metrics_tracker_t* tracker) {
    if (!tracker) return;

    tracker->session.session_start = time(NULL);
    tracker->session.total_instructions_processed = 0;
    tracker->session.total_strategies_applied = 0;
    tracker->session.total_nulls_eliminated = 0;
    tracker->session.total_null_bytes_original = 0;
    tracker->session.model_saves = 0;
    tracker->session.model_loads = 0;

    printf("[METRICS] Session started\n");
}

// End session
void ml_metrics_end_session(ml_metrics_tracker_t* tracker) {
    if (!tracker) return;

    tracker->session.session_end = time(NULL);

    if (tracker->session.total_null_bytes_original > 0) {
        tracker->session.null_elimination_rate =
            (double)tracker->session.total_nulls_eliminated / tracker->session.total_null_bytes_original;
    }

    printf("[METRICS] Session ended\n");
}

// Record model save
void ml_metrics_record_model_save(ml_metrics_tracker_t* tracker) {
    if (!tracker || !tracker->metrics_enabled) return;
    tracker->session.model_saves++;
}

// Record model load
void ml_metrics_record_model_load(ml_metrics_tracker_t* tracker) {
    if (!tracker || !tracker->metrics_enabled) return;
    tracker->session.model_loads++;
}

// Record bad byte elimination details
void ml_metrics_record_bad_byte_elimination_detail(ml_metrics_tracker_t* tracker,
                                                   uint8_t* eliminated_chars,
                                                   int count) {
    if (!tracker || !tracker->metrics_enabled || !eliminated_chars) return;

    (void)count; // Unused parameter, avoid warning

    // Update strategy-specific bad byte breakdown
    // This would typically be called with specific strategy context
    // For now, we'll update the session-level tracking
    for (int i = 0; i < 256; i++) {
        if (eliminated_chars[i]) {
            // This would increment the breakdown for each strategy if we had strategy context
            // For now, we're just tracking at session level
        }
    }
}

// Print bad byte breakdown
void ml_metrics_print_bad_byte_breakdown(ml_metrics_tracker_t* tracker) {
    if (!tracker) return;

    printf("\n=== BAD CHARACTER ELIMINATION BREAKDOWN ===\n\n");
    printf("Configured Bad Characters (%d total): ", tracker->session.bad_byte_count);

    // Print the bad byte set
    int printed = 0;
    for (int i = 0; i < 256; i++) {
        if (tracker->session.bad_byte_set[i]) {
            if (printed > 0) printf(", ");
            printf("0x%02x", i);
            printed++;
        }
    }
    printf("\n");

    // Calculate elimination rates per character if possible
    printf("\nPer-Bad-Byte Statistics:\n");
    printf("%-8s %10s %10s %12s\n", "Char", "Orig", "Eliminated", "Elim Rate");
    printf("%-8s %10s %10s %12s\n", "----", "----", "----------", "---------");

    // This would show actual breakdown per bad byte
    // For now, showing total stats
    printf("%-8s %10d %10d %11.2f%%\n", "TOTAL",
           tracker->session.total_bad_bytes_original,
           tracker->session.total_bad_bytes_eliminated,
           tracker->session.total_bad_bytes_original > 0 ?
               (double)tracker->session.total_bad_bytes_eliminated / tracker->session.total_bad_bytes_original * 100.0 : 0.0);

    printf("\n========================================\n");
}

// Get bad byte elimination rate for specific character
double ml_metrics_get_bad_byte_elimination_rate(ml_metrics_tracker_t* tracker,
                                               uint8_t bad_byte) {
    if (!tracker) return 0.0;

    (void)bad_byte; // Unused parameter, avoid warning

    // This would return specific rate for the bad byte
    // For now, return overall rate as placeholder
    return tracker->session.total_bad_bytes_original > 0 ?
        (double)tracker->session.total_bad_bytes_eliminated / tracker->session.total_bad_bytes_original : 0.0;
}

// Get most difficult bad bytes to eliminate
int ml_metrics_get_most_difficult_bad_bytes(ml_metrics_tracker_t* tracker,
                                           uint8_t* bad_bytes,
                                           int count) {
    if (!tracker || !bad_bytes || count <= 0) return 0;

    // For now, return all configured bad bytes
    // In a more advanced implementation, this would analyze which chars
    // were eliminated with lower success rates
    int actual_count = 0;
    for (int i = 0; i < 256 && actual_count < count; i++) {
        if (tracker->session.bad_byte_set[i]) {
            bad_bytes[actual_count++] = i;
        }
    }

    return actual_count;
}

// Print summary
void ml_metrics_print_summary(ml_metrics_tracker_t* tracker) {
    if (!tracker) return;

    printf("\n=== ML STRATEGIST PERFORMANCE SUMMARY ===\n\n");

    // Session info
    printf("Session Duration: %.2f seconds\n",
           difftime(tracker->session.session_end > 0 ? tracker->session.session_end : time(NULL),
                   tracker->session.session_start));
    printf("Instructions Processed: %d\n", tracker->session.total_instructions_processed);
    printf("Strategies Applied: %d\n", tracker->session.total_strategies_applied);

    // Null bytes (backward compatibility)
    double null_elim_pct = tracker->session.total_null_bytes_original > 0 ?
        (double)tracker->session.total_nulls_eliminated / tracker->session.total_null_bytes_original * 100.0 : 0.0;
    printf("Null Bytes Eliminated: %d / %d (%.2f%%)\n",
           tracker->session.total_nulls_eliminated,
           tracker->session.total_null_bytes_original,
           null_elim_pct);

    // Bad character metrics (v3.0)
    double bad_byte_elim_pct = tracker->session.total_bad_bytes_original > 0 ?
        (double)tracker->session.total_bad_bytes_eliminated / tracker->session.total_bad_bytes_original * 100.0 : 0.0;
    printf("Bad Characters Eliminated: %d / %d (%.2f%%)\n",
           tracker->session.total_bad_bytes_eliminated,
           tracker->session.total_bad_bytes_original,
           bad_byte_elim_pct);
    printf("Configured Bad Characters: %d\n", tracker->session.bad_byte_count);

    // Model performance
    printf("\n--- Model Performance ---\n");
    printf("Predictions Made: %d\n", tracker->model.predictions_made);
    printf("Current Accuracy: %.2f%%\n", tracker->model.current_accuracy * 100.0);
    printf("Accuracy Improvement: %+.2f%%\n", tracker->model.accuracy_improvement * 100.0);
    printf("Avg Prediction Confidence: %.4f\n", tracker->model.avg_prediction_confidence);

    // Learning stats
    printf("\n--- Learning Progress ---\n");
    printf("Learning Enabled: %s\n", tracker->learning.learning_enabled ? "YES" : "NO");
    printf("Total Feedback Iterations: %d\n", tracker->learning.total_feedback_iterations);
    printf("Positive Feedback: %d\n", tracker->learning.positive_feedback_count);
    printf("Negative Feedback: %d\n", tracker->learning.negative_feedback_count);
    printf("Avg Weight Delta: %.6f\n", tracker->learning.avg_weight_delta);
    printf("Max Weight Delta: %.6f\n", tracker->learning.max_weight_delta);

    if (tracker->learning.last_learning_timestamp > 0) {
        char time_buf[64];
        struct tm* tm_info = localtime(&tracker->learning.last_learning_timestamp);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
        printf("Last Learning Update: %s\n", time_buf);
    }

    printf("\n========================================\n");
}

// Print strategy breakdown
void ml_metrics_print_strategy_breakdown(ml_metrics_tracker_t* tracker) {
    if (!tracker) return;

    printf("\n=== STRATEGY PERFORMANCE BREAKDOWN ===\n\n");
    printf("%-30s %8s %8s %8s %10s %8s %12s\n",
           "Strategy", "Attempts", "Success", "Failed", "Success%", "AvgConf", "Bad Chars");
    printf("%-30s %8s %8s %8s %10s %8s %12s\n",
           "--------", "--------", "-------", "------", "--------", "-------", "---------");

    for (int i = 0; i < tracker->strategy_count; i++) {
        strategy_metrics_t* m = &tracker->strategy_metrics[i];
        double success_rate = m->times_attempted > 0 ?
            (double)m->times_succeeded / m->times_attempted * 100.0 : 0.0;
        double avg_conf = m->times_attempted > 0 ?
            m->total_confidence / m->times_attempted : 0.0;

        printf("%-30s %8d %8d %8d %9.2f%% %8.4f %12d\n",
               m->strategy_name,
               m->times_attempted,
               m->times_succeeded,
               m->times_failed,
               success_rate,
               avg_conf,
               m->bad_bytes_eliminated);  // Added bad chars eliminated column
    }

    printf("\n======================================\n");
}

// Print learning progress
void ml_metrics_print_learning_progress(ml_metrics_tracker_t* tracker) {
    if (!tracker) return;

    printf("\n=== LEARNING CYCLE PROGRESS ===\n\n");
    printf("Learning Status: %s\n", tracker->learning.learning_enabled ? "ENABLED" : "DISABLED");
    printf("Total Cycles: %d\n", tracker->learning.total_feedback_iterations);
    printf("Positive Reinforcements: %d (%.1f%%)\n",
           tracker->learning.positive_feedback_count,
           tracker->learning.total_feedback_iterations > 0 ?
               (double)tracker->learning.positive_feedback_count / tracker->learning.total_feedback_iterations * 100.0 : 0.0);
    printf("Negative Reinforcements: %d (%.1f%%)\n",
           tracker->learning.negative_feedback_count,
           tracker->learning.total_feedback_iterations > 0 ?
               (double)tracker->learning.negative_feedback_count / tracker->learning.total_feedback_iterations * 100.0 : 0.0);
    printf("\nWeight Update Statistics:\n");
    printf("  Average Delta: %.6f\n", tracker->learning.avg_weight_delta);
    printf("  Maximum Delta: %.6f\n", tracker->learning.max_weight_delta);
    printf("  Total Accumulated: %.6f\n", tracker->learning.total_weight_delta);

    if (tracker->learning.total_feedback_iterations > 0) {
        printf("\nLearning Rate: %.2f feedback/instruction\n",
               tracker->session.total_instructions_processed > 0 ?
                   (double)tracker->learning.total_feedback_iterations / tracker->session.total_instructions_processed : 0.0);
    }

    printf("\n================================\n");
}

// Export to file
void ml_metrics_export_to_file(ml_metrics_tracker_t* tracker, const char* filepath) {
    if (!tracker || !filepath) return;

    FILE* f = fopen(filepath, "w");
    if (!f) {
        fprintf(stderr, "[METRICS] Failed to open file for export: %s\n", filepath);
        return;
    }

    fprintf(f, "=== ML STRATEGIST METRICS EXPORT ===\n");
    fprintf(f, "Generated: %s\n", ctime(&tracker->session.session_end));
    fprintf(f, "\n--- Session Summary ---\n");
    fprintf(f, "Instructions Processed: %d\n", tracker->session.total_instructions_processed);
    fprintf(f, "Strategies Applied: %d\n", tracker->session.total_strategies_applied);
    fprintf(f, "Nulls Eliminated: %d/%d (%.2f%%)\n",
            tracker->session.total_nulls_eliminated,
            tracker->session.total_null_bytes_original,
            tracker->session.null_elimination_rate * 100.0);

    fprintf(f, "\n--- Model Performance ---\n");
    fprintf(f, "Predictions: %d\n", tracker->model.predictions_made);
    fprintf(f, "Accuracy: %.2f%%\n", tracker->model.current_accuracy * 100.0);
    fprintf(f, "Improvement: %+.2f%%\n", tracker->model.accuracy_improvement * 100.0);

    fprintf(f, "\n--- Learning Progress ---\n");
    fprintf(f, "Feedback Iterations: %d\n", tracker->learning.total_feedback_iterations);
    fprintf(f, "Positive: %d, Negative: %d\n",
            tracker->learning.positive_feedback_count,
            tracker->learning.negative_feedback_count);
    fprintf(f, "Avg Weight Delta: %.6f\n", tracker->learning.avg_weight_delta);

    fprintf(f, "\n--- Strategy Breakdown ---\n");
    for (int i = 0; i < tracker->strategy_count; i++) {
        strategy_metrics_t* m = &tracker->strategy_metrics[i];
        fprintf(f, "%s: %d attempts, %d success, %.2f%% rate\n",
                m->strategy_name,
                m->times_attempted,
                m->times_succeeded,
                m->times_attempted > 0 ? (double)m->times_succeeded / m->times_attempted * 100.0 : 0.0);
    }

    fclose(f);
    printf("[METRICS] Exported to: %s\n", filepath);
}

// Export to JSON
void ml_metrics_export_to_json(ml_metrics_tracker_t* tracker, const char* filepath) {
    if (!tracker || !filepath) return;

    FILE* f = fopen(filepath, "w");
    if (!f) {
        fprintf(stderr, "[METRICS] Failed to open file for JSON export: %s\n", filepath);
        return;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"session\": {\n");
    fprintf(f, "    \"instructions_processed\": %d,\n", tracker->session.total_instructions_processed);
    fprintf(f, "    \"strategies_applied\": %d,\n", tracker->session.total_strategies_applied);
    fprintf(f, "    \"nulls_eliminated\": %d,\n", tracker->session.total_nulls_eliminated);
    fprintf(f, "    \"null_elimination_rate\": %.4f,\n", tracker->session.null_elimination_rate);
    fprintf(f, "    \"bad_bytes_eliminated\": %d,\n", tracker->session.total_bad_bytes_eliminated);
    fprintf(f, "    \"bad_chars_original\": %d,\n", tracker->session.total_bad_bytes_original);
    fprintf(f, "    \"bad_byte_elimination_rate\": %.4f,\n", tracker->session.bad_byte_elimination_rate);
    fprintf(f, "    \"bad_byte_count\": %d,\n", tracker->session.bad_byte_count);
    fprintf(f, "    \"bad_byte_set\": [");

    // Print the bad byte set
    int first = 1;
    for (int i = 0; i < 256; i++) {
        if (tracker->session.bad_byte_set[i]) {
            if (!first) fprintf(f, ",");
            fprintf(f, "%d", i);
            first = 0;
        }
    }
    fprintf(f, "]\n");
    fprintf(f, "  },\n");

    fprintf(f, "  \"model\": {\n");
    fprintf(f, "    \"predictions\": %d,\n", tracker->model.predictions_made);
    fprintf(f, "    \"accuracy\": %.4f,\n", tracker->model.current_accuracy);
    fprintf(f, "    \"improvement\": %.4f,\n", tracker->model.accuracy_improvement);
    fprintf(f, "    \"avg_confidence\": %.4f\n", tracker->model.avg_prediction_confidence);
    fprintf(f, "  },\n");

    fprintf(f, "  \"learning\": {\n");
    fprintf(f, "    \"enabled\": %s,\n", tracker->learning.learning_enabled ? "true" : "false");
    fprintf(f, "    \"iterations\": %d,\n", tracker->learning.total_feedback_iterations);
    fprintf(f, "    \"positive_feedback\": %d,\n", tracker->learning.positive_feedback_count);
    fprintf(f, "    \"negative_feedback\": %d,\n", tracker->learning.negative_feedback_count);
    fprintf(f, "    \"avg_weight_delta\": %.6f,\n", tracker->learning.avg_weight_delta);
    fprintf(f, "    \"max_weight_delta\": %.6f\n", tracker->learning.max_weight_delta);
    fprintf(f, "  },\n");

    fprintf(f, "  \"strategies\": [\n");
    for (int i = 0; i < tracker->strategy_count; i++) {
        strategy_metrics_t* m = &tracker->strategy_metrics[i];
        fprintf(f, "    {\n");
        fprintf(f, "      \"name\": \"%s\",\n", m->strategy_name);
        fprintf(f, "      \"attempts\": %d,\n", m->times_attempted);
        fprintf(f, "      \"success\": %d,\n", m->times_succeeded);
        fprintf(f, "      \"failed\": %d,\n", m->times_failed);
        fprintf(f, "      \"nulls_eliminated\": %d,\n", m->nulls_eliminated);
        fprintf(f, "      \"bad_bytes_eliminated\": %d,\n", m->bad_bytes_eliminated);
        fprintf(f, "      \"avg_confidence\": %.4f\n",
                m->times_attempted > 0 ? m->total_confidence / m->times_attempted : 0.0);
        fprintf(f, "    }%s\n", i < tracker->strategy_count - 1 ? "," : "");
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    printf("[METRICS] JSON exported to: %s\n", filepath);
}

// Export to CSV
void ml_metrics_export_to_csv(ml_metrics_tracker_t* tracker, const char* filepath) {
    if (!tracker || !filepath) return;

    FILE* f = fopen(filepath, "w");
    if (!f) {
        fprintf(stderr, "[METRICS] Failed to open file for CSV export: %s\n", filepath);
        return;
    }

    fprintf(f, "Strategy,Attempts,Success,Failed,SuccessRate,AvgConfidence,NullsEliminated,BadCharsEliminated\n");

    for (int i = 0; i < tracker->strategy_count; i++) {
        strategy_metrics_t* m = &tracker->strategy_metrics[i];
        double success_rate = m->times_attempted > 0 ?
            (double)m->times_succeeded / m->times_attempted : 0.0;
        double avg_conf = m->times_attempted > 0 ?
            m->total_confidence / m->times_attempted : 0.0;

        fprintf(f, "%s,%d,%d,%d,%.4f,%.4f,%d,%d\n",
                m->strategy_name,
                m->times_attempted,
                m->times_succeeded,
                m->times_failed,
                success_rate,
                avg_conf,
                m->nulls_eliminated,
                m->bad_bytes_eliminated);
    }

    fclose(f);
    printf("[METRICS] CSV exported to: %s\n", filepath);
}

// Get overall success rate
double ml_metrics_get_overall_success_rate(ml_metrics_tracker_t* tracker) {
    if (!tracker) return 0.0;

    int total_attempts = 0;
    int total_success = 0;

    for (int i = 0; i < tracker->strategy_count; i++) {
        total_attempts += tracker->strategy_metrics[i].times_attempted;
        total_success += tracker->strategy_metrics[i].times_succeeded;
    }

    return total_attempts > 0 ? (double)total_success / total_attempts : 0.0;
}

// Get strategy success rate
double ml_metrics_get_strategy_success_rate(ml_metrics_tracker_t* tracker,
                                            const char* strategy_name) {
    if (!tracker || !strategy_name) return 0.0;

    for (int i = 0; i < tracker->strategy_count; i++) {
        if (strcmp(tracker->strategy_metrics[i].strategy_name, strategy_name) == 0) {
            strategy_metrics_t* m = &tracker->strategy_metrics[i];
            return m->times_attempted > 0 ?
                (double)m->times_succeeded / m->times_attempted : 0.0;
        }
    }

    return 0.0;
}

// Print live stats
void ml_metrics_print_live_stats(ml_metrics_tracker_t* tracker) {
    if (!tracker) return;

    double overall_success = ml_metrics_get_overall_success_rate(tracker);

    printf("[METRICS] Instructions: %d | Strategies: %d | Success: %.1f%% | Nulls: %d | Learning: %d cycles\n",
           tracker->session.total_instructions_processed,
           tracker->session.total_strategies_applied,
           overall_success * 100.0,
           tracker->session.total_nulls_eliminated,
           tracker->learning.total_feedback_iterations);
}
