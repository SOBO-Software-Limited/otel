#include <iostream>

#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "libMetrics/Api.h"

#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/context.h"

#include "opentelemetry/trace/span_context_kv_iterable_view.h"




namespace metrics_api = opentelemetry::metrics;
namespace nostd = opentelemetry::nostd;
namespace common = opentelemetry::common;

namespace evm {

    zil::metrics::uint64Counter_t &GetInvocationsCounter() {
        static auto counter = Metrics::GetInstance().CreateInt64Metric("evm.invocations.count",
                                                                       "Metrics for AccountStore", "calls");
        return counter;
    }

    zil::metrics::doubleHistogram_t &GetHistogramCounter() {
        static auto counter = Metrics::GetMeter()->CreateDoubleHistogram("zilliqa.accounts", "The Histo Metric", "ms");
        return counter;
    }

}  // namespace


void
TestNewWrappers() {

    std::list<double> boundary{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};

    zil::metrics::InstrumentWrapper<zil::metrics::I64Counter>
            iCounter(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "counter1", "the first counter", "seconds");

    zil::metrics::InstrumentWrapper<zil::metrics::DoubleCounter>
            dCounter(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "counter2", "the Second counter", "seconds");

    zil::metrics::InstrumentWrapper<zil::metrics::DoubleHistogram>
            dHistogram(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "histo1", boundary, "the first Histogram counter",
                       "seconds");

    zil::metrics::InstrumentWrapper<zil::metrics::DoubleHistogram>
            sHistogram(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "histo2", boundary, "the second Histogram counter",
                       "seconds");

    zil::metrics::InstrumentWrapper<zil::metrics::DoubleGauge>
            dGauge(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "gauge1", "My very first gauge", "seconds", true);

    zil::metrics::InstrumentWrapper<zil::metrics::I64Gauge>
            i64Gauge(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "i64gauge1", "My very first i64gauge", "minutes",
                     true);

    zil::metrics::InstrumentWrapper<zil::metrics::I64UpDown>
            i64upAndDown(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "upAndDown", "My very first updown", "flips",
                         true);

    zil::metrics::InstrumentWrapper<zil::metrics::I64UpDown>
            i64downAndUp(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "DownandUp", "My very first downup", "flops",
                         true);

    dGauge.SetCallback([](auto &&result) {
        std::cout << "Holds double data" << std::endl;
        result.Set(87654.0, {{"counter", "BlockNumber"}});
        result.Set(12345.5, {{"counter", "DSBlockNumber"}});
    });


    i64Gauge.SetCallback([](auto &&result) {
        std::cout << "Holds i64 data" << std::endl;
        result.Set(87654, {{"counter", "BlockNumber"}});
        result.Set(12345, {{"counter", "DSBlockNumber"}});
    });

    const auto lambda = [](auto &&result) {

        // do any you like in here

        std::cout << "hello world" << std::endl;

        result.Set(666, {{}});

    };

    i64upAndDown.SetCallback(lambda);

    int i = 1;

    while (1) {
        iCounter++;
        dCounter++;
        dHistogram.Record((i++ % 9));
        dHistogram.Record(((double) (rand() % 9)), {{"counter", i}});
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

namespace context = opentelemetry::context;


template <typename T>
class ZilliqaMapCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
   public:
    ZilliqaMapCarrier(T &headers) : headers_(headers) {}
    ZilliqaMapCarrier() = default;
    virtual opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view key) const noexcept override
    {
        std::string key_to_compare = key.data();
        // Header's first letter seems to be  automatically capitaliazed by our test http-server, so
        // compare accordingly.
        if (key == opentelemetry::trace::propagation::kTraceParent)
        {
          key_to_compare = "Traceparent";
        }
        else if (key == opentelemetry::trace::propagation::kTraceState)
        {
          key_to_compare = "Tracestate";
        }
        auto it = headers_.find(key_to_compare);
        if (it != headers_.end())
        {
          return it->second;
        }
        return "";
    }

    virtual void Set(opentelemetry::nostd::string_view key,
                     opentelemetry::nostd::string_view value) noexcept override
    {
        headers_.insert(std::pair<std::string, std::string>(std::string(key), std::string(value)));
    }

    T headers_;
};


int main() {
    Metrics::GetInstance();
    Tracing::GetInstance();


    TestNewWrappers();


    auto Topspan = START_SPAN(ACC_EVM, {});
    SCOPED_SPAN(ACC_EVM, Topscope, Topspan);

    ZilliqaMapCarrier<std::map<std::string,std::string>>  ourCarrier{};

    // pretend these came in from the wire, although we do not need the carrier at all i suspect

    ourCarrier.headers_.insert({"silly","string"});
    ourCarrier.headers_.insert({"something","else"});

    auto prop        = context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();
    auto current_ctx      = context::RuntimeContext::GetCurrent();
    auto new_context      = prop->Extract(ourCarrier, current_ctx);

    opentelemetry::trace::StartSpanOptions options;
    options.kind    = opentelemetry::trace::SpanKind::kClient;
    options.parent  = opentelemetry::trace::GetSpan(new_context)->GetContext();

    std::string span_name = "Example Span from a client";
    auto span             = Tracing::GetInstance().get_tracer()->StartSpan(span_name,
                                                               {{"txn","zila89374598y98u06bdfef12345efdg"}},
                                              options);

    auto scope            = Tracing::GetInstance().get_tracer()->WithActiveSpan(span);

    auto another_current_ctx      = context::RuntimeContext::GetCurrent();
    auto another_new_context      = prop->Extract(ourCarrier, another_current_ctx);

    std::cout << "things to carry over the wire, if  you wish" << std::endl;

    for (auto& x : ourCarrier.headers_ ){
        std::cout << "Key [" << x.first << "] value [" << x.second << std::endl;
    }

    auto latest_span = another_current_ctx.GetValue("active_span");

    bool holds_span = std::holds_alternative<std::shared_ptr<opentelemetry::v1::trace::Span>>(latest_span);

    if (holds_span){
        auto span = std::get<std::shared_ptr<opentelemetry::v1::trace::Span>>(latest_span);

        // these are uint8_t arrays, so will need to convert to string to print or iter.

        auto spanId = span->GetContext().span_id();
        auto traceId = span->GetContext().trace_id();

    }

    span->SetStatus(opentelemetry::trace::StatusCode::kOk, "We are all good here");

    span->End();

    for (
            int i = 0;
            i < 1000; i++) {
        std::cout << "Sleeping to make sure the is no more activity" <<
                  std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)
        );
    }

    std::shared_ptr<metrics_api::MeterProvider> none;
    metrics_api::Provider::SetMeterProvider(none);

    return 0;
}


