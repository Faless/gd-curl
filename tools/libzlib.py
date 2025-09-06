def build_library(env):
    zlib_bin = env.Dir("#bin/thirdparty/zlib/{}/{}/install".format(env["platform"], env["arch"]))
    is_msvc = env.get("is_msvc", False)
    zlib_config = {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo" if env["debug_symbols"] else "Release",
        "ZLIB_BUILD_EXAMPLES": 0,
        "CMAKE_INSTALL_PREFIX": env.Dir(zlib_bin).abspath,
        "CMAKE_INSTALL_LIBDIR": "lib",
    }

    if env["platform"] == "linux":
        # This is needed on some arch when building with the godot buildroot toolchain
        zlib_config["CMAKE_C_FLAGS"] = " -fPIC"

    lib_name = "zlibstatic" if env["platform"] == "windows" else "z"
    lib_ext = ".lib" if is_msvc else ".a"
    lib_prefix = "" if is_msvc else "lib"
    zlib_libs = [
        "/install/lib/{}{}{}".format(lib_prefix, lib_name, lib_ext),
    ]

    zlib_includes = [
        "/install/include/zconf.h",
        "/install/include/zlib.h",
    ]

    # Build libdatachannel
    zlib = env.CMakeBuild(
        env.Dir("#bin/thirdparty/zlib/"),
        env.Dir("#thirdparty/zlib"),
        cmake_options=zlib_config,
        cmake_outputs=zlib_libs + zlib_includes,
        install=True,
    )
    env.Prepend(LIBS=list(filter(lambda f: str(f).endswith(lib_ext), zlib)))
    env.Append(CPPPATH=[env.Dir("#thirdparty/zlib/include")])

    return zlib


def exists(env):
    return "CMake" in env


def generate(env):
    zlib_install_dir = "#bin/thirdparty/zlib/{}/{}/install".format(env["platform"], env["arch"])
    is_msvc = env.get("is_msvc", False)
    lib_name = "zlibstatic" if env["platform"] == "windows" else "z"
    lib_prefix = "" if is_msvc else "lib"
    lib_ext = ".lib" if is_msvc else ".a"
    zlib = env.File(zlib_install_dir + "/lib/{}{}{}".format(lib_prefix, lib_name, lib_ext))
    includes = env.Dir(zlib_install_dir + "/include")
    env.AddMethod(build_library, "BuildZlib")
    env["ZLIB_LIBRARY"] = zlib.abspath
    env["ZLIB_INCLUDE"] = includes.abspath
