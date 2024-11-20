cmake_minimum_required(VERSION 3.14)
project("Order Matching Engine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

enable_testing()

# Main executable
add_executable(OrderMatchingEngine
  src/main.cpp
  src/CommandType.cpp
  src/Order.cpp
  src/OrderBook.cpp
  src/TraderBase.cpp
  src/Transaction.cpp
  src/TransactionList.cpp
)

# Include directories
target_include_directories(OrderMatchingEngine PRIVATE include)

# # Test executable
# add_executable(testMatching
#   tests/testMatching.cc
# )

# target_link_libraries(
#   testMatching GTest::gtest_main
# )

# target_include_directories(testMatching PRIVATE include)

include(GoogleTest)
# gtest_discover_tests(testMatching)