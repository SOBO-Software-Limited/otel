#include <chrono>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "libMetrics/Tracing2.h"

// These will be ssummed into the cpp files of the API and not exposed once
// testing completed

#include "libUtils/Logger.h"

namespace sobo {
namespace otel {

using zil::trace2::Span;
using zil::trace2::Tracing;

static const zil::trace2::FilterClass NODE_FILTER =
    zil::trace2::FilterClass::NODE;

class ApiTest : public ::testing::Test {
 protected:
  ApiTest() { Tracing::Initialize("xxxx", "ALL"); }

  ~ApiTest() override {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(ApiTest, TestGrabTrace) {
  // Test to see if we have any spans

  auto cSpan = Tracing::GetActiveSpan();

  if (not cSpan.IsRecording()) {
    LOG_GENERAL(INFO, "no spans active");
  }

  // Now create a Span with ...

  std::string span_name = "Example Span from a client";
  auto span = Tracing::CreateSpan(NODE_FILTER, span_name);

  auto activeSpan = Tracing::GetActiveSpan();
  if (activeSpan.GetTraceId().IsValid() && activeSpan.GetSpanId().IsValid()) {
    // we have an active trace.
    LOG_GENERAL(INFO, "we have spans active: " << activeSpan.GetIds());
  }

  span.End(zil::trace2::StatusCode::OK);
}

TEST_F(ApiTest, TestLocalChildSpans) {
  int count = 0;

  auto EnsureSpanIsActive = [](const Span &s) {
    ASSERT_FALSE(s.GetIds().empty());
    ASSERT_EQ(Tracing::GetActiveSpan().GetIds(), s.GetIds());
  };

  auto span = Tracing::CreateSpan(NODE_FILTER, "ParentSpan");
  span.SetAttribute("TheCounter", ++count);
  ASSERT_TRUE(span.IsRecording());
  EnsureSpanIsActive(span);

  {
    auto childSpan1 = Tracing::CreateSpan(NODE_FILTER, "childSpan1");
    childSpan1.SetAttribute("TheCounter", ++count);
    ASSERT_TRUE(childSpan1.IsRecording());
    EnsureSpanIsActive(childSpan1);

    auto childSpan2 = Tracing::CreateSpan(NODE_FILTER, "childSpan2");
    childSpan2.SetAttribute("TheCounter", ++count);
    ASSERT_TRUE(childSpan2.IsRecording());
    EnsureSpanIsActive(childSpan2);

    childSpan2.End(zil::trace2::StatusCode::OK);
    ASSERT_FALSE(childSpan2.IsRecording());
    EnsureSpanIsActive(childSpan1);
  }

  EnsureSpanIsActive(span);
}

TEST_F(ApiTest, TestRemoteChildSpans) {
  constexpr size_t N_THREADS = 4;
  std::atomic<int> count = 0;

  auto activeSpan = Tracing::GetActiveSpan();
  ASSERT_FALSE(activeSpan.IsRecording());

  auto span = Tracing::CreateSpan(NODE_FILTER, "ParentSpan");
  span.SetAttribute("TheCounter", ++count);
  ASSERT_TRUE(span.IsRecording());

  auto activeSpan2 = Tracing::GetActiveSpan();
  ASSERT_TRUE(activeSpan2.IsRecording());
  ASSERT_FALSE(activeSpan2.GetIds().empty());
  ASSERT_EQ(activeSpan2.GetIds(), span.GetIds());
  ASSERT_EQ(activeSpan2.GetSpanId(), span.GetSpanId());
  ASSERT_EQ(activeSpan2.GetTraceId(), span.GetTraceId());

  auto worker = [&count](std::string traceInfo) {
    auto noActiveSpan = Tracing::GetActiveSpan();
    ASSERT_FALSE(noActiveSpan.IsRecording());

    auto span = Tracing::CreateChildSpanOfRemoteTrace(NODE_FILTER, "ChildSpan",
                                                      traceInfo);
    ASSERT_TRUE(span.IsRecording());
    span.SetAttribute("TheCounter", ++count);

    span.AddEvent("Ololo", {{"Counter", count.load()}});
  };

  auto traceInfo = span.GetIds();
  ASSERT_FALSE(traceInfo.empty());

  std::vector<std::thread> workers;
  workers.reserve(N_THREADS);
  for (size_t i = 0; i < N_THREADS; ++i) {
    workers.emplace_back(worker, traceInfo);
  }

  span.AddEvent("Ololo", {{"Counter", count.load()}});

  for (auto &t : workers) {
    t.join();
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}  // namespace otel
}  // namespace sobo
