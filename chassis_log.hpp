#pragma once
#include <phosphor-logging/lg2.hpp>

#include <string>
#include <utility>

class ChassisLogContext
{
  public:
    explicit ChassisLogContext(std::string chassis) :
        chassisName(std::move(chassis)),
        logPrefix("Chassis [" + chassisName + "] ")
    {}

    template <typename Msg, typename... Args>
    void info(Msg&& msg, Args&&... args) const
    {
        lg2::info(logPrefix + std::forward<Msg>(msg),
                  std::forward<Args>(args)...);
    }

    template <typename Msg, typename... Args>
    void error(Msg&& msg, Args&&... args) const
    {
        lg2::error(logPrefix + std::forward<Msg>(msg),
                   std::forward<Args>(args)...);
    }

    template <typename Msg, typename... Args>
    void debug(Msg&& msg, Args&&... args) const
    {
        lg2::debug(logPrefix + std::forward<Msg>(msg),
                   std::forward<Args>(args)...);
    }

    template <typename Msg, typename... Args>
    void warning(Msg&& msg, Args&&... args) const
    {
        lg2::warning(logPrefix, std::forward<Msg>(msg),
                     std::forward<Args>(args)...);
    }

  private:
    std::string chassisName;
    std::string logPrefix;
};
