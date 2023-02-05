/*
 * Copyright (C) 2019 Zilliqa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ZILLIQA_SRC_COMMON_CONSTANTS_H_
#define ZILLIQA_SRC_COMMON_CONSTANTS_H_

#include <string>

namespace {
std::string METRIC_ZILLIQA_PROVIDER{"STDOUT"};
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
std::string METRIC_ZILLIQA_MASK{"ALL"};
std::string TRACE_ZILLIQA_PROVIDER{"NONE"};
const std::string TRACE_ZILLIQA_HOSTNAME{"localhost"};
const std::string TRACE_ZILLIQA_PORT{"4318"};
const double METRICS_VERSION{8.6};
const std::string WARNING{"WARNING"};
const std::string INFO{"INFO"};
std::string TRACE_ZILLIQA_MASK{"ALL"};
const std::string ZILLIQA_METRIC_FAMILY{"zilliqa_cpp"};
};

#endif  // ZILLIQA_SRC_COMMON_CONSTANTS_H_
