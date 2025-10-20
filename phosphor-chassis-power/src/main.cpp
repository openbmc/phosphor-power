#include <sdbusplus/async.hpp>

int main()
{
    sdbusplus::async::context ctx;
    ctx.run();
}
