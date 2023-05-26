#pragma once

#include <aws/s3/model/GetObjectRequest.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>

namespace DB
{

class S3Recorder
{
public:
    S3Recorder()
        : outfile(fmt::format("s3_record_{}.log", std::chrono::system_clock::now().time_since_epoch().count()))
    {
    }

    void record(
        const Aws::S3::Model::GetObjectRequest & req,
        std::chrono::system_clock::time_point start_at,
        double duration_sec)
    {
        std::unique_lock lock(mu);
        auto line = fmt::format(
            "{} {} {} {} {}\n",
            std::chrono::duration_cast<std::chrono::milliseconds>(start_at.time_since_epoch()).count(),
            duration_sec,
            req.GetBucket(),
            req.GetKey(),
            req.GetRange());
        outfile << line;
        outfile.flush();
    }

    inline static std::unique_ptr<S3Recorder> global_recorder = std::make_unique<S3Recorder>();

private:
    std::mutex mu;
    std::ofstream outfile;
};


} // namespace DB
