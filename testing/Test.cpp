#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "libMetrics/Api.h"

// These will be ssummed into the cpp files of the API and not exposed once testing completed

#include "common/Constants.h"
#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/trace/context.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/span_context_kv_iterable_view.h"

namespace sobo {
namespace otel {

template <typename T>
class ZilliqaMapCarrier : public opentelemetry::context::propagation::TextMapCarrier {
 public:
  ZilliqaMapCarrier(T &headers) : headers_(headers) {}
  ZilliqaMapCarrier() = default;
  virtual opentelemetry::nostd::string_view Get(opentelemetry::nostd::string_view key) const noexcept override {
    std::string key_to_compare = key.data();
    // Header's first letter seems to be  automatically capitaliazed by our test http-server, so
    // compare accordingly.
    if (key == opentelemetry::trace::propagation::kTraceParent) {
      key_to_compare = "Traceparent";
    } else if (key == opentelemetry::trace::propagation::kTraceState) {
      key_to_compare = "Tracestate";
    }
    auto it = headers_.find(key_to_compare);
    if (it != headers_.end()) {
      return it->second;
    }
    return "";
  }

  virtual void Set(opentelemetry::nostd::string_view key, opentelemetry::nostd::string_view value) noexcept override {
    headers_.insert(std::pair<std::string, std::string>(std::string(key), std::string(value)));
  }

  T headers_;
};

class ApiTest : public ::testing::Test {
 protected:
  ApiTest() {
    Metrics::GetInstance();
    Tracing::GetInstance();
  }

  ~ApiTest() override {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(ApiTest, TestBadProviderConfiguration) {



  Z_I64METRIC iCounter(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "i64Counter", "the first i64 counter", "seconds");

  for (int i = 0; i < 100; i++) iCounter++;


}

TEST_F(ApiTest, TestGoodProviderConfiguration) {



  Z_I64METRIC iCounter(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "i64Counter", "the first i64 counter", "seconds");

  for (int i = 0; i < 100; i++) iCounter++;



}

TEST_F(ApiTest, TestStdOutProviderConfiguration) {


  Z_I64METRIC iCounter(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "i64Counter", "the first i64 counter", "seconds");

  for (int i = 0; i < 100; i++) iCounter++;


}

TEST_F(ApiTest, TestNoneProviderConfiguration) {

  Z_I64METRIC iCounter(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "i64Counter", "the first i64 counter", "seconds");

  for (int i = 0; i < 100; i++) iCounter++;

}

TEST_F(ApiTest, TestI64Counter) {


  Z_I64METRIC iCounter(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "i64Counter", "the first i64 counter", "seconds");

  for (int i = 0; i < 100; i++) iCounter++;



}

TEST_F(ApiTest, TestdoubleCounter) {
  Z_DBLMETRIC dCounter(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "dblCounter", "the first Double counter", "seconds");

  for (int i = 0; i < 100; i++) dCounter++;
}

TEST_F(ApiTest, TestdoubleHistogram) {
  std::list<double> boundary{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  Z_DBLHIST histogram(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "dblHistogram", boundary, "the first Histogram counter","seconds");

  for (int i = 0; i < 100; i++) {
    histogram.Record((i++ % 9));
    histogram.Record(((double)(rand() % 9)), {{"counter", i}});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

TEST_F(ApiTest, TestDoubleGauge) {
  Z_DBLGAUGE dGauge(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "dblGauge", "My very first gauge", "seconds", true);

  double a,b;

  dGauge.SetCallback([&a,&b](auto &&result) {
    std::cout << "Holds double data" << std::endl;
    result.Set(a, {{"counter", "BlockNumber"}});
    result.Set(b, {{"counter", "DSBlockNumber"}});
  });


  for (int i =0 ; i < 100;i++) {
    a = rand() % 100;
    b = rand() % 99;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

}

TEST_F(ApiTest, Testi64Gauge) {
  Z_I64GAUGE iGauge(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "i64Gauge", "My very first gauge", "seconds", true);

  iGauge.SetCallback([](auto &&result) {
    std::cout << "Holds double data" << std::endl;
    result.Set(87654, {{"counter", "BlockNumber"}});
    result.Set(12345, {{"counter", "DSBlockNumber"}});
  });
}

TEST_F(ApiTest, TestUpDown) {
  Z_I64UPDOWN i64upAndDown(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "upAndDown", "My very first updown", "flips", true);

  int i = 0;

  const auto lambda = [&i](auto &&result) {
    // do any you like in here

    std::cout << "hello world" << std::endl;

    result.Set(i++, {{"from", "me"}});
  };

  i64upAndDown.SetCallback(lambda);

  for (int i =0 ; i < 100;i++) {
    i = rand() % 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100000));


}

TEST_F(ApiTest, TestGrabTrace) {
  std::map<int, std::shared_ptr<opentelemetry::trace::Span>> span_map;


  opentelemetry::trace::StartSpanOptions options;
  options.kind = opentelemetry::trace::SpanKind::kClient;
  std::string span_name = "Example Span from a client";
  auto span = Tracing::GetInstance().get_tracer()->StartSpan(span_name, {{"txn", "zila89374598y98u06bdfef12345efdg"}}, options);

  // save it somewhere to kee it alive.
  span_map.insert({1, span});


  auto context = span->GetContext();

  char trace_id[32];
  context.trace_id().ToLowerBase16(trace_id);
  char span_id[16];
  context.span_id().ToLowerBase16(span_id);
  char trace_flags[2];
  context.trace_flags().ToLowerBase16(trace_flags);

  std::string result;
  result = std::string(trace_flags) + "-" + span_id + "-" + trace_id ;

  std::cout << result << std::endl;


  span_map[1]->End();

  span_map[1] = nullptr;


}

TEST_F(ApiTest, TestTrace) {
  auto Topspan = START_SPAN(ACC_EVM, {});
  SCOPED_SPAN(ACC_EVM, Topscope, Topspan);

  ZilliqaMapCarrier<std::map<std::string, std::string>> ourCarrier{};

  // pretend these came in from the wire, although we do not need the carrier at all i suspect

  auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();
  auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
  prop->Inject(ourCarrier, current_ctx);

  // blah blah blah

  // now we pretend that we recieved a new packet

  ourCarrier.headers_.insert({"silly", "string"});
  ourCarrier.headers_.insert({"something", "else"});

  // get a new context from carrier with carriers info added.
  auto new_context = prop->Extract(ourCarrier, current_ctx);

  std::cout << "things to carry over the wire, if  you wish" << std::endl;

  for (auto &x : ourCarrier.headers_) {
    std::cout << "Key [" << x.first << "] value [" << x.second << std::endl;
  }

  opentelemetry::trace::StartSpanOptions options;
  options.kind = opentelemetry::trace::SpanKind::kClient;
  //
  // Set the parent context from what we received over the wire.
  //
  options.parent = opentelemetry::trace::GetSpan(new_context)->GetContext();

  std::string span_name = "Example Span from a client";
  auto span = Tracing::GetInstance().get_tracer()->StartSpan(span_name, {{"txn", "zila89374598y98u06bdfef12345efdg"}}, options);

  auto scope = Tracing::GetInstance().get_tracer()->WithActiveSpan(span);

  auto another_current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
  prop->Inject(ourCarrier, another_current_ctx);

  std::cout << "things to carry over the wire, if  you wish" << std::endl;

  for (auto &x : ourCarrier.headers_) {
    std::cout << "Key [" << x.first << "] value [" << x.second << std::endl;
  }

  auto latest_span = another_current_ctx.GetValue("active_span");

  bool holds_span = std::holds_alternative<std::shared_ptr<opentelemetry::v1::trace::Span>>(latest_span);

  if (holds_span) {
    auto span = std::get<std::shared_ptr<opentelemetry::v1::trace::Span>>(latest_span);

    // these are uint8_t arrays, so will need to convert to string to print or iter.

    auto spanId = span->GetContext().span_id();
    auto traceId = span->GetContext().trace_id();
  }

  span->SetStatus(opentelemetry::trace::StatusCode::kOk, "We are all good here");

  span->End();

  for (int i = 0; i < 10; i++) {
    std::cout << "Sleeping to make sure the is no more activity" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::shared_ptr<opentelemetry::metrics::MeterProvider> none;
  opentelemetry::metrics::Provider::SetMeterProvider(none);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}  // namespace otel
}  // namespace sobo