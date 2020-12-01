cmake_minimum_required(VERSION 3.9)
project(ScrapeNasd)

set(CMAKE_CXX_STANDARD 11)

add_executable(ScrapeNasd main.cpp Scrape.h Stocks.cpp Scrape.cpp)

include(/usr/local/lib/ktrade/cmake/ktrade-targets.cmake)
target_link_libraries(ScrapeNasd Ktrd)

find_package(Boost 1.64 COMPONENTS date_time system filesystem)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(ScrapeNasd ${Boost_LIBRARIES})
add_executable(TestScrape test_scrape.cpp Scrape_test.h Stocks.cpp Scrape_test.cpp)
target_link_libraries(TestScrape Ktrd ${Boost_LIBRARIES})
#add_definitions(-DBOOST_NETWORK_ENABLE_HTTPS)
