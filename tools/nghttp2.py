def build_library(env):
    nghttp2_bin = env.Dir("#bin/thirdparty/nghttp2/{}/{}/install".format(env["platform"], env["arch"]))
    is_msvc = env.get("is_msvc", False)
    nghttp2_config = {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo" if env["debug_symbols"] else "Release",
        "BUILD_TESTING": 0,
        "ENABLE_WERROR": 0,
        "ENABLE_DEBUG": 0,
        "ENABLE_APP": 0,
        "ENABLE_HPACK_TOOLS": 0,
        "ENABLE_EXAMPLES": 0,
        "ENABLE_FAILMALLOC": 0,
        "ENABLE_LIB_ONLY": 1,
        "BUILD_SHARED_LIBS": 0,
        "BUILD_STATIC_LIBS": 1,
        "ENABLE_STATIC_CRT": 1 if env.get("use_static_cpp", False) else 0,
        "ENABLE_HTTP3": 0,
        "ENABLE_DOC": 0,
        "WITH_LIBXML2": 0,
        "WITH_JEMALLOC": 0,
        "WITH_MRUBY": 0,
        "WITH_NEVERBLEED": 0,
        "WITH_LIBBPF": 0,
        "WITH_WOLFSSL": 0,
        "CMAKE_POSITION_INDEPENDENT_CODE": 1,
        "CMAKE_INSTALL_PREFIX": env.Dir(nghttp2_bin).abspath,
        "CMAKE_INSTALL_LIBDIR": "lib",
    }
    lib_ext = ".lib" if is_msvc else ".a"
    lib_prefix = "" if is_msvc else "lib"
    nghttp2_libs = [
        "/install/lib/{}nghttp2{}".format(lib_prefix, lib_ext),
    ]

    nghttp2_includes = [
        "/install/include/nghttp2/nghttp2.h",
        "/install/include/nghttp2/nghttp2ver.h",
    ]

    nghttp2_cmake_config = [
        "/install/lib/cmake/nghttp2/nghttp2Targets.cmake",
        "/install/lib/cmake/nghttp2/nghttp2Targets-{}.cmake".format(
            "relwithdebinfo" if env["debug_symbols"] else "release"
        ),
        "/install/lib/cmake/nghttp2/nghttp2Config.cmake",
        "/install/lib/cmake/nghttp2/nghttp2ConfigVersion.cmake",
    ]

    # Build libdatachannel
    nghttp2 = env.CMakeBuild(
        env.Dir("#bin/thirdparty/nghttp2/"),
        env.Dir("#thirdparty/nghttp2"),
        cmake_targets=["nghttp2_static"],
        cmake_options=nghttp2_config,
        cmake_outputs=nghttp2_libs + nghttp2_cmake_config + nghttp2_includes,
        install=True,
    )
    env.Prepend(LIBS=list(filter(lambda f: str(f).endswith(lib_ext), nghttp2)))
    env.Append(CPPPATH=[env.Dir("#thirdparty/nghttp2/include")])

    return nghttp2


def exists(env):
    return "CMake" in env


def generate(env):
    nghttp2_install_dir = "#bin/thirdparty/nghttp2/{}/{}/install".format(env["platform"], env["arch"])
    is_msvc = env.get("is_msvc", False)
    lib_prefix = "" if is_msvc else "lib"
    lib_ext = ".lib" if env.get("is_msvc", False) else ".a"
    nghttp2 = env.File(nghttp2_install_dir + "/lib/{}nghttp2{}".format(lib_prefix, lib_ext))
    includes = env.Dir(nghttp2_install_dir + "/include")
    env.AddMethod(build_library, "BuildNGHTTP2")
    env["NGHTTP2_LIBRARY"] = nghttp2.abspath
    env["NGHTTP2_INCLUDE"] = includes.abspath
