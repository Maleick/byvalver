#ifndef ML_METRICS_H
#define ML_METRICS_H

#include <time.h>
#include <stdint.h>

// Maximum number of strategies to track
#define MAX_TRACKED_STRATEGIES 200

// Strategy-specific performance metrics
typedef struct {
    char strategy_name[64];
    int times_attempted;
    int times_succeeded;
    int times_failed;
    double total_confidence;
    double min_confidence;
    double max_confidence;
    int nulls_eliminated;
    int total_output_size_increase;
    double total_processing_time_ms;
} strategy_metrics_t;

// Learning cycle metrics
typedef struct {
    int total_feedback_iterations;
    int positive_feedback_count;
    int negative_feedback_count;
    double total_weight_delta;
    double avg_weight_delta;
    double max_weight_delta;
    time_t last_learning_timestamp;
    int learning_enabled;
} learning_metrics_t;

// Model performance over time
typedef struct {
    double initial_accuracy;
    double current_accuracy;
    double accuracy_improvement;
    int predictions_made;
    int correct_predictions;
    int incorrect_predictions;
    double avg_prediction_confidence;
} model_performance_t;

// Session-wide metrics
typedef struct {
    time_t session_start;
    time_t session_end;
    int total_instructions_processed;
    int total_strategies_applied;
    int total_nulls_eliminated;
    int total_null_bytes_original;
    double null_elimination_rate;
    double total_processing_time_ms;
    int model_saves;
    int model_loads;
} session_metrics_t;

// Complete metrics tracker
typedef struct {
    strategy_metrics_t strategy_metrics[MAX_TRACKED_STRATEGIES];
    int strategy_count;
    learning_metrics_t learning;
    model_performance_t model;
    session_metrics_t session;
    int metrics_enabled;
    char metrics_file_path[256];
} ml_metrics_tracker_t;

// Metrics API
ml_metrics_tracker_t* ml_metrics_init(const char* metrics_file);
void ml_metrics_cleanup(ml_metrics_tracker_t* tracker);

// Strategy tracking
void ml_metrics_record_strategy_attempt(ml_metrics_tracker_t* tracker,
                                       const char* strategy_name,
                                       double confidence);
void ml_metrics_record_strategy_result(ml_metrics_tracker_t* tracker,
                                      const char* strategy_name,
                                      int success,
                                      int nulls_eliminated,
                                      int size_increase,
                                      double processing_time_ms);

// Learning tracking
void ml_metrics_record_feedback(ml_metrics_tracker_t* tracker,
                               int positive,
                               double weight_delta);
void ml_metrics_record_learning_iteration(ml_metrics_tracker_t* tracker,
                                         double avg_weight_change,
                                         double max_weight_change);

// Model performance tracking
void ml_metrics_record_prediction(ml_metrics_tracker_t* tracker,
                                 int correct,
                                 double confidence);
void ml_metrics_update_model_accuracy(ml_metrics_tracker_t* tracker,
                                     double new_accuracy);

// Session tracking
void ml_metrics_start_session(ml_metrics_tracker_t* tracker);
void ml_metrics_end_session(ml_metrics_tracker_t* tracker);
void ml_metrics_record_instruction_processed(ml_metrics_tracker_t* tracker,
                                            int nulls_in_instruction);
void ml_metrics_record_model_save(ml_metrics_tracker_t* tracker);
void ml_metrics_record_model_load(ml_metrics_tracker_t* tracker);

// Reporting and export
void ml_metrics_print_summary(ml_metrics_tracker_t* tracker);
void ml_metrics_print_strategy_breakdown(ml_metrics_tracker_t* tracker);
void ml_metrics_print_learning_progress(ml_metrics_tracker_t* tracker);
void ml_metrics_export_to_file(ml_metrics_tracker_t* tracker, const char* filepath);
void ml_metrics_export_to_json(ml_metrics_tracker_t* tracker, const char* filepath);
void ml_metrics_export_to_csv(ml_metrics_tracker_t* tracker, const char* filepath);

// Real-time monitoring
void ml_metrics_print_live_stats(ml_metrics_tracker_t* tracker);
double ml_metrics_get_overall_success_rate(ml_metrics_tracker_t* tracker);
double ml_metrics_get_strategy_success_rate(ml_metrics_tracker_t* tracker,
                                            const char* strategy_name);
int ml_metrics_get_top_strategies(ml_metrics_tracker_t* tracker,
                                  char* top_strategies[],
                                  int count);

#endif // ML_METRICS_H
