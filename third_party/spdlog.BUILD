package(default_visibility = ["//visibility:public"])

cc_library(
    name = "spdlog",
    hdrs = glob(["include/**/*.h"]),
    defines = [
        "SPDLOG_HEADER_ONLY",
        "SPDLOG_USE_STD_FORMAT",
        "SPDLOG_ENABLE_THREAD_POOL",
    ],
    includes = ["include"],
)
