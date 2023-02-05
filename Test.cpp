#include "libMetrics/Api.h"

#include "gtest/gtest.h"

namespace sobo {
namespace otel {
  class ApiTest : public ::testing::Test {
   protected:
    ApiTest() {}

    ~ApiTest() override {}

    void SetUp() override {}

    void TearDown() override {}
  };

  TEST_F(ApiTest, FirstTest){

  }

  TEST_F(ApiTest, SecondTest){

  }
}
}  // namespace sobo

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}