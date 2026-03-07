/*
 * Metrics Header - Prometheus-style Metrics for Observability
 */

#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include <time.h>

#define METRICS_MAX_COUNTERS 50
#define METRICS_MAX_GAUGES 50
#define METRICS_MAX_HISTOGRAMS 20
#define METRICS_MAX_LABELS 10
#define METRICS_NAME_LENGTH 64
#define METRICS_LABEL_LENGTH 256

// Metric types
typedef enum {
    METRIC_TYPE_COUNTER,
    METRIC_TYPE_GAUGE,
    METRIC_TYPE_HISTOGRAM
} metric_type_t;

// Label structure
typedef struct {
    char name[METRICS_NAME_LENGTH];
    char value[METRICS_LABEL_LENGTH];
} metric_label_t;

// Counter metric
typedef struct {
    char name[METRICS_NAME_LENGTH];
    char help[256];
    uint64_t value;
    metric_label_t labels[METRICS_MAX_LABELS];
    int label_count;
} metric_counter_t;

// Gauge metric
typedef struct {
    char name[METRICS_NAME_LENGTH];
    char help[256];
    double value;
    metric_label_t labels[METRICS_MAX_LABELS];
    int label_count;
} metric_gauge_t;

// Histogram bucket
typedef struct {
    double upper_bound;
    uint64_t count;
} metric_bucket_t;

// Histogram metric
typedef struct {
    char name[METRICS_NAME_LENGTH];
    char help[256];
    metric_bucket_t buckets[10];
    int bucket_count;
    uint64_t sum;
    uint64_t count;
} metric_histogram_t;

// Metrics registry
typedef struct {
    metric_counter_t counters[METRICS_MAX_COUNTERS];
    int counter_count;
    metric_gauge_t gauges[METRICS_MAX_GAUGES];
    int gauge_count;
    metric_histogram_t histograms[METRICS_MAX_HISTOGRAMS];
    int histogram_count;
} metrics_registry_t;

// Function prototypes
int metrics_init(metrics_registry_t *registry);
void metrics_close(metrics_registry_t *registry);

// Counter operations
int metrics_counter_create(metrics_registry_t *registry, const char *name, const char *help);
void metrics_counter_inc(metrics_registry_t *registry, const char *name);
void metrics_counter_add(metrics_registry_t *registry, const char *name, uint64_t value);

// Gauge operations
int metrics_gauge_create(metrics_registry_t *registry, const char *name, const char *help);
void metrics_gauge_set(metrics_registry_t *registry, const char *name, double value);
void metrics_gauge_inc(metrics_registry_t *registry, const char *name);
void metrics_gauge_dec(metrics_registry_t *registry, const char *name);

// Histogram operations
int metrics_histogram_create(metrics_registry_t *registry, const char *name, const char *help, 
                              double *buckets, int bucket_count);
void metrics_histogram_observe(metrics_registry_t *registry, const char *name, double value);

// Export
int metrics_export_prometheus(metrics_registry_t *registry, char *buffer, size_t buffer_size);

#endif // METRICS_H