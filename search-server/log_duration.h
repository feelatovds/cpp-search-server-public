#pragma once

#include <chrono>
#include <iostream>
#include <string>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, stream) LogDuration UNIQUE_VAR_NAME_PROFILE(x, stream);

class LogDuration {
public:
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& operation_name, std::ostream& out = std::cerr)
    : operation_name_(operation_name)
    , out_(out)
    {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        const auto time_operation = duration_cast<microseconds>(dur).count();
        std::cerr << operation_name_ << ": "s << time_operation << " mks"s << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
    const std::string operation_name_;
    std::ostream& out_;
};