#ifndef BOLLOX
#define BOLLOX

#include <string>
#include <boost/lexical_cast.hpp>

namespace {
const std::string METRIC_ZILLIQA_PROVIDER{"STDOUT"};
const uint64_t METRIC_ZILLIQA_READER_EXPORT_MS{1000};
const uint64_t METRIC_ZILLIQA_READER_TIMEOUT_MS{500};
const std::string METRIC_ZILLIQA_HOSTNAME{"localhost"};
const int METRIC_ZILLIQA_PORT{4318};
const int METRIC_ZILLIQA_GRPC_PORT{4317};
const std::string METRIC_ZILLIQA_GRPC_HOST{"0.0.0.0"};
const std::string METRIC_ZILLIQA_PROMETHEUS_PORT{"8090"};
const std::string METRIC_ZILLIQA_SCHEMA_VERSION{"1.2.0"};
const std::string METRIC_ZILLIQA_SCHEMA{
    "https://opentelemetry.io/schemas/1.2.0"};
const std::string METRIC_ZILLIQA_MASK{"ALL"};
const std::string TRACE_ZILLIQA_PROVIDER{"STDOUT"};
const std::string TRACE_ZILLIQA_HOSTNAME{"localhost"};
const std::string TRACE_ZILLIQA_PORT{"4318"};
const double METRICS_VERSION{8.6};
const std::string WARNING{"WARNING"};
const std::string INFO{"INFO"};
const std::string TRACE_ZILLIQA_MASK{"ALL"};
const std::string ZILLIQA_METRIC_FAMILY{"zilliqa_cpp"};
};

#define LOG_GENERAL(level, msg) \
  { std::cout << "level:" << level << ":" << msg; }

#endif