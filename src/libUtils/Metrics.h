/*
 * Copyright (C) 2022 Zilliqa
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

#ifndef ZILLIQA_SRC_LIBUTILS_METRICS_H_
#define ZILLIQA_SRC_LIBUTILS_METRICS_H_

#include <cassert>
#include <variant>

#include "common/MetricFilters.h"
#include "common/Singleton.h"

#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/metrics/async_instruments.h"
#include "opentelemetry/metrics/sync_instruments.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/sdk/metrics/metric_reader.h"
#include <opentelemetry/sdk/metrics/view/view.h>
#include "opentelemetry/common/key_value_iterable.h"
#include "opentelemetry/common/attribute_value.h"
#include "common/TraceFilters.h"

class Metrics;

namespace opentelemetry::metrics {
    class MeterProvider;
}

namespace zil {
    namespace metrics {

        std::chrono::system_clock::time_point r_timer_start();

        double r_timer_end(std::chrono::system_clock::time_point start_time);

        namespace common = opentelemetry::common;
        namespace metrics_api = opentelemetry::metrics;

        const std::string METRIC_FAMILY{"zilliqa"};
        const std::string METRIC_SCHEMA_VERSION{"1.2.0"};
        const std::string METRIC_SCHEMA{"https://opentelemetry.io/schemas/1.2.0"};

        using uint64Counter_t = std::unique_ptr<metrics_api::Counter<uint64_t>>;
        using doubleCounter_t = std::unique_ptr<metrics_api::Counter<double>>;

        using uint64Historgram_t = std::unique_ptr<metrics_api::Histogram<uint64_t>>;
        using doubleHistogram_t = std::unique_ptr<metrics_api::Histogram<double>>;

        inline auto GetMeter(
                std::shared_ptr<opentelemetry::metrics::MeterProvider> &provider, const std::string &family) {
            return provider->GetMeter(family, METRIC_SCHEMA_VERSION, METRIC_SCHEMA);
        }

        inline std::string GetFullName(const std::string &family,
                                       const std::string &name) {
            std::string full_name;
            full_name.reserve(family.size() + name.size() + 1);
            full_name += family;
            full_name += "_";
            full_name += name;
            return full_name;
        }

        class Filter : public Singleton<Filter> {
        public:
            void init();

            bool Enabled(FilterClass to_test) {
                return m_mask & (1 << static_cast<int>(to_test));
            }

        private:
            uint64_t m_mask{};
        };

        class Observable {
        public:
            class Result {
            public:
                template<class T>
                void Set(T value, const common::KeyValueIterable &attributes) {
#if TESTING  // TODO : warn to logger
                    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
#endif
                    if constexpr (std::is_integral_v<T>) {
                        // This looks like a bug in openTelemetry, need to investigate, clash
                        // between uint64_t and long int should be unsigned, losing precision.

                        SetImpl(static_cast<int64_t>(value), attributes);
                    } else {
                        SetImpl(static_cast<double>(value), attributes);
                    }
                }

                template<class T, class U,
                        std::enable_if_t<
                                common::detail::is_key_value_iterable<U>::value> * = nullptr>
                void Set(T value, const U &attributes) noexcept {
                    Set(value, common::KeyValueIterableView<U>{attributes});
                }

                template<class T>
                void Set(T value, std::initializer_list<
                        std::pair<std::string, common::AttributeValue>>
                attributes) noexcept {
                    Set(value, opentelemetry::nostd::span<
                            const std::pair<std::string, common::AttributeValue>>{
                            attributes.begin(), attributes.end()});
                }

            private:
                friend Observable;  // for ctor

                Result(opentelemetry::metrics::ObserverResult &r) : m_result(r) {}

                void SetImpl(int64_t value, const common::KeyValueIterable &attributes);

                void SetImpl(double value, const common::KeyValueIterable &attributes);

                opentelemetry::metrics::ObserverResult &m_result;
            };

            using Callback = std::function<void(Result &&result)>;

            void SetCallback(Callback cb);

            /// Dtor resets callback in compliance to opentelemetry API
            ~Observable();

            // No copy-move because stability of 'this' ptr is required
            Observable(const Observable &) = delete;

            Observable(Observable &&) = delete;

            Observable &operator=(const Observable &) = delete;

            Observable &operator=(Observable &&) = delete;

        private:
            using observable_t = std::shared_ptr<metrics_api::ObservableInstrument>;

            // for ctor.
            friend Metrics;

            Observable(observable_t ob)
                    : m_observable(std::move(ob)) {
                assert(m_observable);
            }

            static void RawCallback(
                    opentelemetry::metrics::ObserverResult observer_result, void *state);

            observable_t m_observable;
            Callback m_callback;
        };

    }  // namespace metrics
}  // namespace zil

// Class metrics updated to OpenTelemetry
//
// Uses a singleton to lazy load and initialise metrics if at least one metric
// is called.

class Metrics : public Singleton<Metrics> {
public:
    Metrics();

    std::string Version() { return "Initial"; }

    zil::metrics::uint64Counter_t
    CreateInt64Metric(const std::string &name, const std::string &desc, std::string unit = "");

    zil::metrics::doubleCounter_t
    CreateDoubleMetric(const std::string &name, const std::string &desc, std::string unit = "");

    zil::metrics::doubleHistogram_t
    CreateDoubleHistogram(const std::string &name, const std::string &desc, std::string unit = "");

    zil::metrics::Observable CreateInt64Gauge(const std::string &name, const std::string &desc, std::string unit = "");

    zil::metrics::Observable CreateDoubleGauge(const std::string &name, const std::string &desc, std::string unit = "");

    zil::metrics::Observable
    CreateInt64UpDownMetric(const std::string &name, const std::string &desc, std::string unit = "");

    zil::metrics::Observable
    CreateDoubleUpDownMetric(const std::string &name, const std::string &desc, std::string unit = "");

    zil::metrics::Observable
    CreateInt64ObservableCounter(const std::string &name, const std::string &desc, std::string unit = "");

    zil::metrics::Observable
    CreateDoubleObservableCounter(const std::string &name, const std::string &desc, std::string unit = "");

    /// Called on main() exit explicitly
    void Shutdown();

    void AddCounterSumView(const std::string &name, const std::string &description);

    void AddCounterHistogramView(const std::string name, std::list<double> list, const std::string &description);

    bool CaptureEMT(std::shared_ptr<opentelemetry::trace::Span> &span,
                    zil::metrics::FilterClass fc,
                    zil::trace::FilterClass tc,
                    zil::metrics::uint64Counter_t &metric,
                    const std::string &messageText = "",
                    const uint8_t &code = 0);

    static std::shared_ptr<opentelemetry::metrics::Meter> GetMeter();

    struct LatencyScopeMarker final {
        LatencyScopeMarker(zil::metrics::uint64Counter_t &metric,
                           zil::metrics::doubleHistogram_t &latency,
                           zil::metrics::FilterClass fc,
                           const char *file,
                           int line,
                           const char *func,
                           bool should_print = true);

        ~LatencyScopeMarker();

    private:
        std::string m_file;
        int m_line;
        std::string m_func;
        bool should_print;
        zil::metrics::uint64Counter_t &m_metric;
        zil::metrics::doubleHistogram_t &m_latency;
        zil::metrics::FilterClass &m_filterClass;
        std::chrono::system_clock::time_point m_startTime;

        LatencyScopeMarker(const LatencyScopeMarker &) = delete;

        LatencyScopeMarker &operator=(const LatencyScopeMarker &) = delete;
    };

private:
    void Init();

    void InitPrometheus(const std::string &addr);

    void InitOTHTTP();

    void InitOtlpGrpc();

    void InitStdOut();

    std::unique_ptr<opentelemetry::sdk::metrics::MetricReader> GetReader();
};

#define INCREMENT_CALLS_COUNTER(COUNTER, FILTER_CLASS, ATTRIBUTE, VALUE) \
  if (zil::metrics::Filter::GetInstance().Enabled(                       \
          zil::metrics::FilterClass::FILTER_CLASS)) {                    \
    COUNTER->Add(1, {{ATTRIBUTE, VALUE}});                               \
  }

#define INCREMENT_CALLS_COUNTER2(COUNTER, FILTER_CLASS, ATTRIBUTE, VALUE, \
                                 ATTRIBUTE2, VALUE2)                      \
  if (zil::metrics::Filter::GetInstance().Enabled(                        \
          zil::metrics::FilterClass::FILTER_CLASS)) {                     \
    COUNTER->Add(1, {{ATTRIBUTE, VALUE}, {ATTRIBUTE2, VALUE2}});          \
  }

#define INCREMENT_METHOD_CALLS_COUNTER(COUNTER, FILTER_CLASS) \
  if (zil::metrics::Filter::GetInstance().Enabled(            \
          zil::metrics::FilterClass::FILTER_CLASS)) {         \
    COUNTER->Add(1, {{"Method", __FUNCTION__}});              \
  }

#define METRICS_ENABLED(FILTER_CLASS)          \
  zil::metrics::Filter::GetInstance().Enabled( \
      zil::metrics::FilterClass::FILTER_CLASS)

#define INCREMENT_METHOD_CALLS_COUNTER2(COUNTER, FILTER_CLASS, METHOD) \
  if (zil::metrics::Filter::GetInstance().Enabled(                     \
          zil::metrics::FilterClass::FILTER_CLASS)) {                  \
    COUNTER->Add(1, {{"Method", METHOD}});                             \
  }

namespace zil {
    namespace metrics {

        using METRIC_ATTRIBUTE =
                std::map<std::string, opentelemetry::common::AttributeValue>;

        // Wrap an integer Counter

        class I64Counter {
        public:
            I64Counter(const std::string &name, const std::string &description, const std::string &units) {
                m_theCounter = Metrics::GetMeter()->CreateUInt64Counter(GetFullName(METRIC_FAMILY, name), description,
                                                                        units);
            }

            void
            Increment() {
                m_theCounter->Add(1);
            }

            void
            IncrementWithAttributes(long val, METRIC_ATTRIBUTE &attr) {
                m_theCounter->Add(val, attr);
            }

            virtual ~I64Counter() {}

            friend std::ostream &operator<<(std::ostream &os, const I64Counter &counter);

        private:
            uint64Counter_t m_theCounter;
        };

        // wrap a double counter

        class DoubleCounter {
        public:
            DoubleCounter(const std::string &name, const std::string &description, const std::string &units) {
                m_theCounter = Metrics::GetMeter()->CreateDoubleCounter(GetFullName(METRIC_FAMILY, name), description,
                                                                        units);
            }

            void
            Increment() {
                m_theCounter->Add(1);
            }

            void
            IncrementWithAttributes(long val, METRIC_ATTRIBUTE &attr) {
                m_theCounter->Add(val, attr);
            }

        private:
            doubleCounter_t m_theCounter;
        };

        // wrap a histogram

        class DoubleHistogram {
        public:

            DoubleHistogram(const std::string &name,
                            const std::list<double> &boundaries,
                            const std::string &description,
                            const std::string &units)
                    : m_boundaries(boundaries) {
                Metrics::GetInstance().AddCounterHistogramView(GetFullName(METRIC_FAMILY, name), boundaries,
                                                               description);
                m_theCounter = Metrics::GetMeter()->CreateDoubleHistogram(GetFullName(METRIC_FAMILY, name), description,
                                                                          units);
            }

            void
            Record(double val) {
                auto context = opentelemetry::context::Context{};
                m_theCounter->Record(val, context);
            }

            void
            Record(double val, const METRIC_ATTRIBUTE &attr) {
                auto context = opentelemetry::context::Context{};
                m_theCounter->Record(val, attr, context);
            }

        private:
            std::list<double> m_boundaries;
            doubleHistogram_t m_theCounter;
        };

        class DoubleGauge {
        public:

            DoubleGauge(const std::string &name,
                        const std::string &description,
                        const std::string &units,
                        bool obs)
                    : m_theGauge(
                    Metrics::GetInstance().CreateDoubleGauge(zil::metrics::GetFullName(METRIC_FAMILY, name),
                                                             description, units)) {
            }

            using Callback = std::function<void(Observable::Result &&result)>;

            void SetCallback(const Callback &cb) {
                m_theGauge.SetCallback(cb);
            }

        private:
            zil::metrics::Observable m_theGauge;
        };

        class I64Gauge {
        public:

            I64Gauge(const std::string &name,
                     const std::string &description,
                     const std::string &units,
                     bool obs)
                    : m_theGauge(
                    Metrics::GetInstance().CreateInt64Gauge(GetFullName(METRIC_FAMILY, name), description,
                                                            units)) {
            }

            using Callback = std::function<void(Observable::Result &&result)>;

            void SetCallback(const Callback &cb) {
                m_theGauge.SetCallback(cb);
            }

        private:
            zil::metrics::Observable m_theGauge;
        };

        class I64UpDown {
        public:
            I64UpDown(const std::string &name,
                      const std::string &description,
                      const std::string &units,
                      bool obs)
                    : m_theGauge(
                    Metrics::GetInstance().CreateInt64UpDownMetric(GetFullName(METRIC_FAMILY, name), description,
                                                                   units)) {
            }

            using Callback = std::function<void(Observable::Result &&result)>;

            void SetCallback(const Callback &cb) {
                m_theGauge.SetCallback(cb);
            }

        private:
            zil::metrics::Observable m_theGauge;
        };

        class DoubleUpDown {
        public:
            DoubleUpDown(const std::string &name,
                         const std::string &description,
                         const std::string &units,
                         bool obs)
                    : m_theGauge(
                    Metrics::GetInstance().CreateDoubleUpDownMetric(GetFullName(METRIC_FAMILY, name), description,
                                                                    units)) {
            }

            using Callback = std::function<void(Observable::Result &&result)>;

            void SetCallback(const Callback &cb) {
                m_theGauge.SetCallback(cb);
            }

        private:
            zil::metrics::Observable m_theGauge;
        };

        template<typename T>
        struct InstrumentWrapper : T {
            InstrumentWrapper(zil::metrics::FilterClass fc, const std::string &name, const std::string &description,
                              const std::string &units)
                    : T(name, description, units) {
                m_fc = fc;
            }

            // Special for the histogram.

            InstrumentWrapper(zil::metrics::FilterClass fc, const std::string &name, const std::list<double> &list,
                              const std::string &description,
                              const std::string &units)
                    : T(name, list, description, units) {
                m_fc = fc;
            }

            InstrumentWrapper(zil::metrics::FilterClass fc,
                              const std::string &name,
                              const std::string &description,
                              const std::string &units,
                              bool obs)
                    : T(name, description, units, obs) {
                m_fc = fc;
            }

            InstrumentWrapper &operator++() {
                if (Filter::GetInstance().Enabled(m_fc)) {
                    T::Increment();
                }
                return *this;
            }

            // Prefix increment operator.
            InstrumentWrapper &operator++(int) {
                if (Filter::GetInstance().Enabled(m_fc)) {
                    T::Increment();
                }
                return *this;
            }

            // Declare prefix and postfix decrement operators.
            InstrumentWrapper &operator--() {
                if (Filter::GetInstance().Enabled(m_fc)) {
                    T::Decrement();
                }
                return *this;
            }     // Prefix d

            // decrement operator.
            InstrumentWrapper operator--(int) {
                InstrumentWrapper temp = *this;
                if (Filter::GetInstance().Enabled(m_fc)) {
                    T::Decrement();
                }
                --*this;
                return temp;
            }

            void IncrementAttr(METRIC_ATTRIBUTE attr) {
                if (Filter::GetInstance().Enabled(m_fc)) {
                    T::IncrementWithAttributes(1, attr);
                }
            }

            void Increment(size_t steps) {
                if (Filter::GetInstance().Enabled(m_fc)) {
                    while (steps--)
                        T::Increment();
                }
            }

            void Decrement(size_t steps) {
                if (Filter::GetInstance().Enabled(m_fc)) {
                    while (steps--)
                        T::Assign();
                }
            }

        private:
            zil::metrics::FilterClass m_fc;
        };
    } // evm
} // observability

#define CALLS_LATENCY_MARKER(COUNTER, LATENCY, FILTER_CLASS) \
  Metrics::LatencyScopeMarker marker{COUNTER, LATENCY, FILTER_CLASS, __FILE__, __LINE__, __FUNCTION__};

#endif  // ZILLIQA_SRC_LIBUTILS_METRICS_H_
