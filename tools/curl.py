def build_library(env, ssl, zlib):
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
        "ENABLE_UNIX_SOCKETS": 0,
        "CURL_CA_BUNDLE": "none",
        "CURL_CA_PATH": "none",
        "CURL_USE_LIBSSH2": 0,  # Why does curl even...
        "CURL_USE_LIBPSL": 0,  # Thanks IANA.
        "USE_LIBIDN2": 0,  # GPL
        "USE_NGHTTP2": 0,
        "CMAKE_POSITION_INDEPENDENT_CODE": 1,
        "CURL_USE_PKGCONFIG": 0,
        "CURL_BROTLI": 0,
        "CURL_ZSTD": 0,
        "CURL_ZLIB": 1,
        "ZLIB_LIBRARY": env["ZLIB_LIBRARY"],
        "ZLIB_INCLUDE_DIR": env["ZLIB_INCLUDE"],
        "CURL_STATIC_CRT": 1 if env.get("use_static_cpp", False) else 0,
    }

    use_openssl = env.get("tls_library", "") == "openssl"
    use_mbedtls = env.get("tls_library", "") == "mbedtls"
    if use_openssl:
        # OpenSSL
        curl_config["CURL_USE_OPENSSL"] = 1
        curl_config["OPENSSL_USE_STATIC_LIBS"] = 1
        curl_config["OPENSSL_INCLUDE_DIR"] = env["SSL_INCLUDE"]
        curl_config["OPENSSL_SSL_LIBRARY"] = env["SSL_LIBRARY"]
        curl_config["OPENSSL_CRYPTO_LIBRARY"] = env["SSL_CRYPTO_LIBRARY"]
        curl_config["OPENSSL_ROOT_DIR"] = env["SSL_INSTALL"]
    elif use_mbedtls:
        curl_config["CURL_USE_MBEDTLS"] = 1 if not use_openssl else 0
        curl_config["MBEDTLS_LIBRARY"] = env["MBEDTLS_LIBRARY"]
        curl_config["MBEDCRYPTO_LIBRARY"] = env["MBEDTLS_CRYPTO_LIBRARY"]
        curl_config["MBEDX509_LIBRARY"] = env["MBEDTLS_X509_LIBRARY"]
        curl_config["MBEDTLS_INCLUDE_DIR"] = env["MBEDTLS_INCLUDE"]
    else:
        raise ValueError("Invalid 'tls_library': " + env.get("tls_library", ""))

    is_msvc = env.get("is_msvc", False)
    lib_ext = ".lib" if is_msvc else ".a"
    curl_libs = [
        "lib/libcurl{}".format(lib_ext),
    ]
    # Build cURL
    curl = env.CMakeBuild(
        env.Dir("#bin/thirdparty/curl/"),
        env.Dir("#thirdparty/curl"),
        cmake_options=curl_config,
        cmake_outputs=curl_libs,
        cmake_targets=["libcurl_static"],
        dependencies=ssl + zlib,
    )

    # Configure env.
    if env["platform"] == "windows":
        env.PrependUnique(LIBS=["ws2_32", "bcrypt", "advapi32"])
    if env["platform"] == "linux":
        env.PrependUnique(LIBS=["pthread"])
    env.Prepend(LIBS=list(filter(lambda f: str(f).endswith(lib_ext), curl)))
    env.Append(CPPPATH=[env.Dir("#thirdparty/curl/include")])
    env.Append(CPPDEFINES=["CURL_STATICLIB"])  # For Windows MSVC

    return curl


def exists(env):
    return "CMake" in env


def generate(env):
    env.AddMethod(build_library, "BuildCURL")
