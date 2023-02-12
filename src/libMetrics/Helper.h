//
// Created by stephen on 12/02/23.
//

#ifndef OTEL_HELPER_H
#define OTEL_HELPER_H

#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/span_context.h>
#include <string>

std::string ExtractTraceInfoFromActiveSpan();

trace_api::SpanContext ExtractSpanContextFromTraceInfo(const std::string &traceInfo);

std::shared_ptr<trace_api::Span> CreateChildSpan(std::string_view name, const std::string &traceInfo);

#endif  // OTEL_HELPER_H
