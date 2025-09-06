def build_library(env, ssl):
    ngtcp2_bin = env.Dir("#bin/thirdparty/ngtcp2/{}/{}/install".format(env["platform"], env["arch"]))
    is_msvc = env.get("is_msvc", False)
    ngtcp2_config = {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo" if env["debug_symbols"] else "Release",
        "BUILD_TESTING": 0,
        "ENABLE_WERROR": 0,
        "ENABLE_DEBUG": 0,
        "ENABLE_ASAN": 0,
        "ENABLE_JEMALLOC": 0,
        "ENABLE_LIB_ONLY": 1,
        "ENABLE_SHARED_LIB": 0,
        "ENABLE_STATIC_LIB": 1,
        "ENABLE_STATIC_CRT": 1 if env.get("use_static_cpp", False) else 0,
        "ENABLE_GNUTLS": 0,
        "ENABLE_OPENSSL": 1,
        "ENABLE_BORINGSSL": 0,
        "ENABLE_PICOTLS": 0,
        "ENABLE_WOLFSSL": 0,
        "CMAKE_POSITION_INDEPENDENT_CODE": 1,
        "CMAKE_INSTALL_PREFIX": env.Dir(ngtcp2_bin).abspath,
        "CMAKE_INSTALL_LIBDIR": "lib",
        "OPENSSL_USE_STATIC_LIBS": 1,
        "OPENSSL_INCLUDE_DIR": env["SSL_INCLUDE"],
        "OPENSSL_SSL_LIBRARY": env["SSL_LIBRARY"],
        "OPENSSL_LIBRARIES": env["SSL_LIBRARY"].abspath + ":" + env["SSL_CRYPTO_LIBRARY"].abspath,
        "OPENSSL_CRYPTO_LIBRARY": env["SSL_CRYPTO_LIBRARY"],
        "OPENSSL_ROOT_DIR": env["SSL_INSTALL"],
        "CMAKE_DISABLE_FIND_PACKAGE_PkgConfig": 1,
    }
    lib_ext = ".lib" if is_msvc else ".a"
    lib_prefix = "" if is_msvc else "lib"
    ngtcp2_libs = [
        "/install/lib/{}ngtcp2{}".format(lib_prefix, lib_ext),
        "/install/lib/{}ngtcp2_crypto_ossl{}".format(lib_prefix, lib_ext),
    ]

    ngtcp2_includes = [
        "/install/include/ngtcp2/ngtcp2.h",
        "/install/include/ngtcp2/ngtcp2_crypto.h",
        "/install/include/ngtcp2/ngtcp2_crypto_ossl.h",
        "/install/include/ngtcp2/version.h",
    ]

    ngtcp2_cmake_config = [
        "/install/lib/cmake/ngtcp2/ngtcp2Targets.cmake",
        "/install/lib/cmake/ngtcp2/ngtcp2Targets-{}.cmake".format(
            "relwithdebinfo" if env["debug_symbols"] else "release"
        ),
        "/install/lib/cmake/ngtcp2/ngtcp2Config.cmake",
        "/install/lib/cmake/ngtcp2/ngtcp2ConfigVersion.cmake",
    ]

    # Build libdatachannel
    ngtcp2 = env.CMakeBuild(
        env.Dir("#bin/thirdparty/ngtcp2/"),
        env.Dir("#thirdparty/ngtcp2"),
        cmake_options=ngtcp2_config,
        cmake_outputs=ngtcp2_libs + ngtcp2_cmake_config + ngtcp2_includes,
        dependencies=ssl,
        install=True,
    )
    env.Prepend(LIBS=list(filter(lambda f: str(f).endswith(lib_ext), ngtcp2)))
    env.Append(CPPPATH=[env.Dir("#thirdparty/ngtcp2/include")])

    return ngtcp2


def exists(env):
    return "CMake" in env


def generate(env):
    ngtcp2_install_dir = "#bin/thirdparty/ngtcp2/{}/{}/install".format(env["platform"], env["arch"])
    is_msvc = env.get("is_msvc", False)
    lib_prefix = "" if is_msvc else "lib"
    lib_ext = ".lib" if env.get("is_msvc", False) else ".a"
    ngtcp2 = env.File(ngtcp2_install_dir + "/lib/{}ngtcp2{}".format(lib_prefix, lib_ext))
    ngtcp2_crypto = env.File(ngtcp2_install_dir + "/lib/{}ngtcp2_crypto_ossl{}".format(lib_prefix, lib_ext))
    includes = env.Dir(ngtcp2_install_dir + "/include")
    env.AddMethod(build_library, "BuildNGTCP2")
    env["NGTCP2_LIBRARY"] = ngtcp2.abspath
    env["NGTCP2_CRYPTO_LIBRARY"] = ngtcp2_crypto.abspath
    env["NGTCP2_INCLUDE"] = includes.abspath
