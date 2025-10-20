// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2025 IBM Corporation

#include <sdbusplus/async.hpp>

int main()
{
    sdbusplus::async::context ctx;
    ctx.run();
}
