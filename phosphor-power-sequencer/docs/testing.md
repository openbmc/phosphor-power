## Overview

The [test](../test) directory contains automated test cases written using the
[GoogleTest](https://github.com/google/googletest) framework. These tests are
run during Continuous Integration (CI) testing.

The goal is to write automated test cases for as much application code as
possible.

## Test case conventions

Each implementation source file should have a corresponding tests file. For
example, [rail.cpp](../src/rail.cpp) is tested by
[rail_tests.cpp](../test/rail_tests.cpp).

Within the tests file, there should be a TEST for each public method or global
scope function. For example, `Rail::isPresent()` is tested by
`TEST(RailTests, IsPresent)`.

## Mock framework

One of the primary challenges with automated testing is handling external
interfaces. The phosphor-power-sequencer application depends on the following
external interfaces:

- D-Bus
- Journal
- Error logging
- GPIOs
- sysfs files containing PMBus information
- BMC dump

These interfaces are either not available or do not behave properly within the
CI test environment. Thus, in automated test cases they need to be mocked.

The GoogleTest mock framework is used to implement mocking. This allows the
behavior of the external interface to be simulated and controlled within the
test case.

The mock framework typically relies on a class hierarchy of the following form:

- Abstract base class with virtual methods for external interfaces
  - Concrete sub-class with real implementation of virtual methods that call
    actual external interfaces
  - Mock sub-class that provides mock implementation of virtual methods

The phosphor-power-sequencer application follows this pattern using the
following classes:

- [Services](../src/services.hpp)
  - [BMCServices](../src/services.hpp)
  - [MockServices](../test/mock_services.hpp)
