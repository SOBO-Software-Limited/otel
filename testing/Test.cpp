#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "libMetrics/Api.h"

// These will be ssummed into the cpp files of the API and not exposed once testing completed

#include "common/Constants.h"
#include "libUtils/Logger.h"
#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/trace/context.h"
#include "opentelemetry/trace/propagation/b3_propagator.h"
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
  std::vector<double> boundary{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
  Z_DBLHIST histogram(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "dblHistogram", boundary, "the first Histogram counter",
                      "seconds");

  for (int i = 0; i < 100; i++) {
    histogram.Record((i++ % 9));
    histogram.Record(((double)(rand() % 9)), {{"counter", i}});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

TEST_F(ApiTest, TestDoubleGauge) {
  Z_DBLGAUGE dGauge(zil::metrics::FilterClass::ACCOUNTSTORE_EVM, "dblGauge", "My very first gauge", "seconds", true);

  double a, b;

  dGauge.SetCallback([&a, &b](auto &&result) {
    std::cout << "Holds double data" << std::endl;
    result.Set(a, {{"counter", "BlockNumber"}});
    result.Set(b, {{"counter", "DSBlockNumber"}});
  });

  for (int i = 0; i < 100; i++) {
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

  for (int i = 0; i < 100; i++) {
    i = rand() % 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100000));
}

TEST_F(ApiTest, TestGrabTrace) {
  // Test to see if we have any spans

  auto cSpan = Tracing::GetInstance().get_tracer()->GetCurrentSpan();

  if (not cSpan->GetContext().IsValid()) {
    LOG_GENERAL(INFO, "no spans active");
  }

  // Now create a Span with ...

  opentelemetry::trace::StartSpanOptions options;
  options.kind = opentelemetry::trace::SpanKind::kClient;
  std::string span_name = "Example Span from a client";
  auto span = Tracing::GetInstance().get_tracer()->StartSpan(span_name, {{"txn", "zila89374598y98u06bdfef12345efdg"}}, options);

  // or use our own macro

  auto fastSpan = START_SPAN(EVM_RPC, {});

  // make it active by sticking in a scope.

  SCOPED_SPAN(ACC_EVM, scope, span);
  // save it somewhere to keep it alive.

  auto activeSpan = Tracing::GetInstance().get_tracer()->GetCurrentSpan();
  auto spanContext = activeSpan->GetContext();
  if (spanContext.trace_id().IsValid() && spanContext.span_id().IsValid()) {
    // we have an active trace.
    LOG_GENERAL(INFO, "we have spans active");
    char trace_id[32];
    spanContext.trace_id().ToLowerBase16(trace_id);
    char span_id[16];
    spanContext.span_id().ToLowerBase16(span_id);
    char trace_flags[2];
    spanContext.trace_flags().ToLowerBase16(trace_flags);
    std::string result;
    result = std::string(trace_flags, 2) + "-" + std::string(span_id, 16) + "-" + std::string(trace_id, 32);

    std::cout << result << std::endl;
  }

  span->End();

  // fast span should dissapear when function goes out of scope.
}

namespace {
constexpr size_t FLAGS_OFFSET = 0;
constexpr size_t FLAGS_SIZE = 2;
constexpr size_t SPAN_ID_OFFSET = FLAGS_SIZE + 1;
constexpr size_t SPAN_ID_SIZE = 16;
constexpr size_t TRACE_ID_OFFSET = SPAN_ID_OFFSET + SPAN_ID_SIZE + 1;
constexpr size_t TRACE_ID_SIZE = 32;
constexpr size_t TRACE_INFO_SIZE = FLAGS_SIZE + 1 + SPAN_ID_SIZE + 1 + TRACE_ID_SIZE;
}  // namespace

std::string ExtractTraceInfoFromActiveSpan() {
  auto activeSpan = Tracing::GetInstance().get_tracer()->GetCurrentSpan();
  auto spanContext = activeSpan->GetContext();
  if (!spanContext.IsValid()) {
    LOG_GENERAL(WARNING, "No active spans");
    return {};
  }

  std::string result(TRACE_INFO_SIZE, '-');
  spanContext.trace_flags().ToLowerBase16(std::span<char, FLAGS_SIZE>(result.data() + FLAGS_OFFSET, FLAGS_SIZE));
  spanContext.span_id().ToLowerBase16(std::span<char, SPAN_ID_SIZE>(result.data() + SPAN_ID_OFFSET, SPAN_ID_SIZE));
  spanContext.trace_id().ToLowerBase16(std::span<char, TRACE_ID_SIZE>(result.data() + TRACE_ID_OFFSET, TRACE_ID_SIZE));

  return result;
}

trace_api::SpanContext ExtractSpanContextFromTraceInfo(const std::string &traceInfo) {
  if (traceInfo.size() != TRACE_INFO_SIZE) {
    LOG_GENERAL(WARNING, "Unexpected trace info size " << traceInfo.size());
    return trace_api::SpanContext::GetInvalid();
  }

  if (traceInfo[SPAN_ID_OFFSET - 1] != '-' || traceInfo[TRACE_ID_OFFSET - 1] != '-') {
    LOG_GENERAL(WARNING, "Invalid format of trace info " << traceInfo);
    return trace_api::SpanContext::GetInvalid();
  }

  std::string_view trace_id_hex(traceInfo.data() + TRACE_ID_OFFSET, TRACE_ID_SIZE);
  std::string_view span_id_hex(traceInfo.data() + SPAN_ID_OFFSET, SPAN_ID_SIZE);
  std::string_view trace_flags_hex(traceInfo.data() + FLAGS_OFFSET, FLAGS_SIZE);

  using trace_api::propagation::detail::IsValidHex;

  if (!IsValidHex(trace_id_hex) || !IsValidHex(span_id_hex) || !IsValidHex(trace_flags_hex)) {
    LOG_GENERAL(WARNING, "Invalid hex of trace info fields: " << traceInfo);
    return trace_api::SpanContext::GetInvalid();
  }

  using trace_api::propagation::B3PropagatorExtractor;

  auto trace_id = B3PropagatorExtractor::TraceIdFromHex(trace_id_hex);
  auto span_id = B3PropagatorExtractor::SpanIdFromHex(span_id_hex);
  auto trace_flags = B3PropagatorExtractor::TraceFlagsFromHex(trace_flags_hex);

  if (!trace_id.IsValid() || !span_id.IsValid()) {
    LOG_GENERAL(WARNING, "Invalid trace_id or span_id in " << traceInfo);
    return trace_api::SpanContext::GetInvalid();
  }

  return trace_api::SpanContext(trace_id, span_id, trace_flags, true);
}

std::shared_ptr<trace_api::Span> CreateChildSpan(std::string_view name, const std::string &traceInfo) {
  auto spanCtx = ExtractSpanContextFromTraceInfo(traceInfo);
  if (!spanCtx.IsValid()) {
    return trace_api::Tracer::GetCurrentSpan();
  }
  //  std::shared_ptr<trace_api::Span> parent = std::make_shared<trace_api::DefaultSpan>(spanCtx);

  trace_api::StartSpanOptions options;

  // child spans from deserialized parent  are of server kind
  options.kind = trace_api::SpanKind::kServer;
  options.parent = spanCtx;  // trace_api::GetSpan(new_context)->GetContext();

  return Tracing::GetInstance().get_tracer()->StartSpan(name, options);
}

TEST_F(ApiTest, TestTraceChildSpans) {
  constexpr size_t N_THREADS = 32;
  std::atomic<int> count = 0;

  auto activeSpan = Tracing::GetInstance().get_tracer()->GetCurrentSpan();
  ASSERT_FALSE(activeSpan->GetContext().IsValid());

  auto span = Tracing::GetInstance().get_tracer()->StartSpan("ParentSpan");
  span->SetAttribute("TheCounter", ++count);
  auto scope = trace_api::Scope(span);
  ASSERT_TRUE(span->GetContext().IsValid());

  activeSpan = Tracing::GetInstance().get_tracer()->GetCurrentSpan();
  ASSERT_TRUE(activeSpan->GetContext().IsValid());
  ASSERT_EQ(activeSpan->GetContext(), span->GetContext());

  auto worker = [&count](std::string traceInfo) {
    auto activeSpan = Tracing::GetInstance().get_tracer()->GetCurrentSpan();
    ASSERT_FALSE(activeSpan->GetContext().IsValid());

    auto span = CreateChildSpan("ChildSpan", traceInfo);
    span->SetAttribute("TheCounter", ++count);
    auto scope = trace_api::Scope(span);
    ASSERT_TRUE(span->GetContext().IsValid());

    span->AddEvent("Ololo", {{ "Counter", count.load() }});
  };

  auto traceInfo = ExtractTraceInfoFromActiveSpan();
  ASSERT_FALSE(traceInfo.empty());

  std::vector<std::thread> workers;
  workers.reserve(N_THREADS);
  for (size_t i = 0; i < N_THREADS; ++i) {
    workers.emplace_back(worker, traceInfo);
  }

  span->AddEvent("Ololo", {{ "Counter", count.load() }});

  for (auto &t : workers) {
    t.join();
  }
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
