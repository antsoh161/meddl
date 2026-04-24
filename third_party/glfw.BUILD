package(default_visibility = ["//visibility:public"])

GLFW_LINUX_SRCS = [
    "src/context.c",
    "src/init.c",
    "src/input.c",
    "src/monitor.c",
    "src/vulkan.c",
    "src/window.c",
    "src/x11_init.c",
    "src/x11_monitor.c",
    "src/x11_window.c",
    "src/xkb_unicode.c",
    "src/posix_time.c",
    "src/posix_thread.c",
    "src/glx_context.c",
    "src/egl_context.c",
    "src/osmesa_context.c",
    "src/linux_joystick.c",
]

genrule(
    name = "glfw_config_h",
    outs = ["src/glfw_config.h"],
    cmd = 'echo "#define _GLFW_X11 1" > $@',
)

cc_library(
    name = "glfw",
    srcs = GLFW_LINUX_SRCS + glob(["src/*.h"]) + [":glfw_config_h"],
    hdrs = glob(["include/GLFW/*.h"]),
    copts = ["-D_GLFW_X11"],
    includes = [
        "include",
        "src",
    ],
    linkopts = [
        "-lX11",
        "-lXcursor",
        "-lXinerama",
        "-lXrandr",
        "-lXxf86vm",
        "-ldl",
        "-lm",
    ],
)
