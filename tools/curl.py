def build_library(env, ssl):
    curl_config = {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo" if env["debug_symbols"] else "Release",
        "BUILD_CURL_EXE": 0,
        "BUILD_SHARED_LIBS": 0,
        "BUILD_STATIC_LIBS": 1,
        "CURL_DISABLE_INSTALL": 1,
        "CURL_LTO": 1 if env.get("use_lto", False) else 0,
        "HTTP_ONLY": 1,
        "BUILD_LIBCURL_DOCS": 0,
        "BUILD_MISC_DOCS": 0,
        "ENABLE_CURL_MANUAL": 0,
        "CURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG": 0,  # Should this be 1? Nah, probably use mbedTLS in the long run!
        "CURL_DISABLE_TESTS": 1,
        "ENABLE_UNIX_SOCKETS": 0,
        "CURL_CA_BUNDLE": "none",
        "CURL_CA_PATH": "none",
        "CURL_USE_LIBSSH2": 0,  # Why does curl even...
        "CURL_USE_LIBPSL": 0,  # Thanks IANA.
        "USE_LIBIDN2": 0,  # GPL
        "CMAKE_POSITION_INDEPENDENT_CODE": 1,
        "CMAKE_DISABLE_FIND_PACKAGE_ZLIB": 1,
        "OPENSSL_USE_STATIC_LIBS": 1,
        "OPENSSL_INCLUDE_DIR": env["SSL_INCLUDE"],
        "OPENSSL_SSL_LIBRARY": env["SSL_LIBRARY"],
        "OPENSSL_CRYPTO_LIBRARY": env["SSL_CRYPTO_LIBRARY"],
        "OPENSSL_ROOT_DIR": env["SSL_INSTALL"],
    }
    is_msvc = env.get("is_msvc", False)
    lib_ext = ".lib" if is_msvc else ".a"
    lib_prefix = "" if is_msvc else "lib"
    curl_libs = [
        "lib/{}curl{}".format(lib_prefix, lib_ext),
    ]
    # Build cURL
    curl = env.CMakeBuild(
        "#bin/thirdparty/curl/",
        "#thirdparty/curl",
        cmake_options=curl_config,
        cmake_outputs=curl_libs,
        cmake_targets=["libcurl_static"],
        dependencies=ssl,
    )

    # Configure env.
    if env["platform"] == "windows":
        env.PrependUnique(LIBS=["ws2_32", "bcrypt"])
    if env["platform"] == "linux":
        env.PrependUnique(LIBS=["pthread"])
    env.Prepend(LIBS=list(filter(lambda f: str(f).endswith(lib_ext), curl)))
    env.Append(CPPPATH=["#thirdparty/curl/include"])

    return curl


def exists(env):
    return "CMake" in env


def generate(env):
    env.AddMethod(build_library, "BuildCURL")
