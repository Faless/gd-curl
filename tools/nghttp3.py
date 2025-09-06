def build_library(env):
    nghttp3_bin = env.Dir("#bin/thirdparty/nghttp3/{}/{}/install".format(env["platform"], env["arch"]))
    is_msvc = env.get("is_msvc", False)
    nghttp3_config = {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo" if env["debug_symbols"] else "Release",
        "BUILD_TESTING": 0,
        "ENABLE_WERROR": 0,
        "ENABLE_DEBUG": 0,
        "ENABLE_ASAN": 0,
        "ENABLE_LIB_ONLY": 1,
        "ENABLE_SHARED_LIB": 0,
        "ENABLE_STATIC_LIB": 1,
        "ENABLE_STATIC_CRT": 1 if env.get("use_static_cpp", False) else 0,
        "ENABLE_POPCNT": 0,
        "CMAKE_POSITION_INDEPENDENT_CODE": 1,
        "CMAKE_INSTALL_PREFIX": env.Dir(nghttp3_bin).abspath,
        "CMAKE_INSTALL_LIBDIR": "lib",
    }
    lib_ext = ".lib" if is_msvc else ".a"
    lib_prefix = "" if is_msvc else "lib"
    nghttp3_libs = [
        "/install/lib/{}nghttp3{}".format(lib_prefix, lib_ext),
    ]

    nghttp3_includes = [
        "/install/include/nghttp3/nghttp3.h",
        "/install/include/nghttp3/version.h",
    ]

    nghttp3_cmake_config = [
        "/install/lib/cmake/nghttp3/nghttp3Targets.cmake",
        "/install/lib/cmake/nghttp3/nghttp3Targets-{}.cmake".format(
            "relwithdebinfo" if env["debug_symbols"] else "release"
        ),
        "/install/lib/cmake/nghttp3/nghttp3Config.cmake",
        "/install/lib/cmake/nghttp3/nghttp3ConfigVersion.cmake",
    ]

    # Build libdatachannel
    nghttp3 = env.CMakeBuild(
        env.Dir("#bin/thirdparty/nghttp3/"),
        env.Dir("#thirdparty/nghttp3"),
        cmake_options=nghttp3_config,
        cmake_outputs=nghttp3_libs + nghttp3_cmake_config + nghttp3_includes,
        install=True,
    )
    env.Prepend(LIBS=list(filter(lambda f: str(f).endswith(lib_ext), nghttp3)))
    env.Append(CPPPATH=[env.Dir("#thirdparty/nghttp3/include")])

    return nghttp3


def exists(env):
    return "CMake" in env


def generate(env):
    nghttp3_install_dir = "#bin/thirdparty/nghttp3/{}/{}/install".format(env["platform"], env["arch"])
    is_msvc = env.get("is_msvc", False)
    lib_prefix = "" if is_msvc else "lib"
    lib_ext = ".lib" if env.get("is_msvc", False) else ".a"
    nghttp3 = env.File(nghttp3_install_dir + "/lib/{}nghttp3{}".format(lib_prefix, lib_ext))
    includes = env.Dir(nghttp3_install_dir + "/include")
    env.AddMethod(build_library, "BuildNGHTTP3")
    env["NGHTTP3_LIBRARY"] = nghttp3.abspath
    env["NGHTTP3_INCLUDE"] = includes.abspath
