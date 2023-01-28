#include <iostream>

#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include <opentelemetry/sdk/metrics/view/meter_selector.h>
#include <opentelemetry/sdk/metrics/view/view.h>
#include "opentelemetry/context/context.h"
#include "opentelemetry/metrics/provider.h"

#include "libUtils/Metrics.h"
#include "libUtils/Tracing.h"

namespace metrics_api = opentelemetry::metrics;
namespace metrics_sdk = opentelemetry::sdk::metrics;
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


int main() {
    Metrics::GetInstance();
    Tracing::GetInstance();

    TestNewWrappers();

    auto span = START_SPAN(ACC_EVM, {});
    SCOPED_SPAN(ACC_EVM, scope, span);

    span->SetStatus(opentelemetry::trace::StatusCode::kOk, "We are all good here");


    Metrics::GetInstance().CaptureEMT(span, zil::metrics::FilterClass::ACCOUNTSTORE_EVM,
                                      zil::trace::FilterClass::ACC_EVM, evm::GetInvocationsCounter(), "It only works");

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


