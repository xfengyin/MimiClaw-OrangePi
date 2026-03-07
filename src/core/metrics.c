/*
 * Metrics Implementation - Prometheus-style Metrics
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "metrics.h"
#include "logger.h"

static pthread_mutex_t metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

int metrics_init(metrics_registry_t *registry) {
    if (!registry) return -1;
    
    memset(registry, 0, sizeof(metrics_registry_t));
    LOG_INFO("Metrics registry initialized");
    return 0;
}

void metrics_close(metrics_registry_t *registry) {
    if (!registry) return;
    LOG_INFO("Metrics registry closed");
}

int metrics_counter_create(metrics_registry_t *registry, const char *name, const char *help) {
    if (!registry || !name || !help) return -1;
    
    pthread_mutex_lock(&metrics_mutex);
    
    if (registry->counter_count >= METRICS_MAX_COUNTERS) {
        pthread_mutex_unlock(&metrics_mutex);
        return -1;
    }
    
    metric_counter_t *counter = &registry->counters[registry->counter_count++];
    strncpy(counter->name, name, METRICS_NAME_LENGTH - 1);
    strncpy(counter->help, help, 255);
    counter->value = 0;
    counter->label_count = 0;
    
    pthread_mutex_unlock(&metrics_mutex);
    LOG_DEBUG("Counter created: %s", name);
    return 0;
}

void metrics_counter_inc(metrics_registry_t *registry, const char *name) {
    metrics_counter_add(registry, name, 1);
}

void metrics_counter_add(metrics_registry_t *registry, const char *name, uint64_t value) {
    if (!registry || !name) return;
    
    pthread_mutex_lock(&metrics_mutex);
    
    for (int i = 0; i < registry->counter_count; i++) {
        if (strcmp(registry->counters[i].name, name) == 0) {
            registry->counters[i].value += value;
            break;
        }
    }
    
    pthread_mutex_unlock(&metrics_mutex);
}

int metrics_gauge_create(metrics_registry_t *registry, const char *name, const char *help) {
    if (!registry || !name || !help) return -1;
    
    pthread_mutex_lock(&metrics_mutex);
    
    if (registry->gauge_count >= METRICS_MAX_GAUGES) {
        pthread_mutex_unlock(&metrics_mutex);
        return -1;
    }
    
    metric_gauge_t *gauge = &registry->gauges[registry->gauge_count++];
    strncpy(gauge->name, name, METRICS_NAME_LENGTH - 1);
    strncpy(gauge->help, help, 255);
    gauge->value = 0;
    gauge->label_count = 0;
    
    pthread_mutex_unlock(&metrics_mutex);
    LOG_DEBUG("Gauge created: %s", name);
    return 0;
}

void metrics_gauge_set(metrics_registry_t *registry, const char *name, double value) {
    if (!registry || !name) return;
    
    pthread_mutex_lock(&metrics_mutex);
    
    for (int i = 0; i < registry->gauge_count; i++) {
        if (strcmp(registry->gauges[i].name, name) == 0) {
            registry->gauges[i].value = value;
            break;
        }
    }
    
    pthread_mutex_unlock(&metrics_mutex);
}

void metrics_gauge_inc(metrics_registry_t *registry, const char *name) {
    if (!registry || !name) return;
    
    pthread_mutex_lock(&metrics_mutex);
    
    for (int i = 0; i < registry->gauge_count; i++) {
        if (strcmp(registry->gauges[i].name, name) == 0) {
            registry->gauges[i].value++;
            break;
        }
    }
    
    pthread_mutex_unlock(&metrics_mutex);
}

void metrics_gauge_dec(metrics_registry_t *registry, const char *name) {
    if (!registry || !name) return;
    
    pthread_mutex_lock(&metrics_mutex);
    
    for (int i = 0; i < registry->gauge_count; i++) {
        if (strcmp(registry->gauges[i].name, name) == 0) {
            registry->gauges[i].value--;
            break;
        }
    }
    
    pthread_mutex_unlock(&metrics_mutex);
}

int metrics_export_prometheus(metrics_registry_t *registry, char *buffer, size_t buffer_size) {
    if (!registry || !buffer || buffer_size == 0) return -1;
    
    pthread_mutex_lock(&metrics_mutex);
    
    buffer[0] = '\0';
    size_t offset = 0;
    
    // Export counters
    for (int i = 0; i < registry->counter_count; i++) {
        metric_counter_t *c = &registry->counters[i];
        offset += snprintf(buffer + offset, buffer_size - offset,
            "# HELP %s %s\n# TYPE %s counter\n%s %lu\n\n",
            c->name, c->help, c->name, c->name, c->value);
    }
    
    // Export gauges
    for (int i = 0; i < registry->gauge_count; i++) {
        metric_gauge_t *g = &registry->gauges[i];
        offset += snprintf(buffer + offset, buffer_size - offset,
            "# HELP %s %s\n# TYPE %s gauge\n%s %f\n\n",
            g->name, g->help, g->name, g->name, g->value);
    }
    
    pthread_mutex_unlock(&metrics_mutex);
    
    return 0;
}