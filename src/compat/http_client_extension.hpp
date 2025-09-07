/**************************************************************************/
/*  http_client_extension.hpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

// THIS FILE IS GENERATED. EDITS WILL BE LOST.

#ifndef GODOT_CPP_HTTP_CLIENT_EXTENSION_COMPAT_HPP
#define GODOT_CPP_HTTP_CLIENT_EXTENSION_COMPAT_HPP

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/http_client.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <godot_cpp/core/class_db.hpp>

#include "tls_options.hpp"

namespace godot {

class StreamPeer;
class String;

class HTTPClientExtensionCompat : public RefCounted {
	GDCLASS(HTTPClientExtensionCompat, RefCounted)

public:
	HTTPClientExtensionCompat() {}

	// Compat for testing pre extension class PR.
	Error _request_raw(HTTPClient::Method p_method, const String &p_url, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
		int size = p_body.size();
		return request(p_method, p_url, p_headers, size > 0 ? p_body.ptr() : nullptr, size);
	}

	Error _request_string(HTTPClient::Method p_method, const String &p_url, const PackedStringArray &p_headers, const String &p_body) {
		CharString body_utf8 = p_body.utf8();
		int size = body_utf8.length();
		return request(p_method, p_url, p_headers, size > 0 ? (const uint8_t *)body_utf8.get_data() : nullptr, size);
	}

	virtual Error request(HTTPClient::Method method, const String &url, const PackedStringArray &headers, const uint8_t *body, int32_t body_size) { return _request(method, url, headers, body, body_size); }
	virtual Error connect_to_host(const String &host, int32_t port, const Ref<TLSOptionsCompat> &tls_options) { return _connect_to_host(host, port, tls_options); }
	virtual void set_connection(const Ref<StreamPeer> &connection) { _set_connection(connection); }
	virtual Ref<StreamPeer> get_connection() const { return _get_connection(); }
	virtual void close() { _close(); }
	virtual HTTPClient::Status get_status() const { return _get_status(); }
	virtual bool has_response() const { return _has_response(); }
	virtual bool is_response_chunked() const { return _is_response_chunked(); }
	virtual int32_t get_response_code() const { return _get_response_code(); }
	virtual PackedStringArray get_response_headers() { return _get_response_headers(); }
	virtual int64_t get_response_body_length() const { return _get_response_body_length(); }
	virtual PackedByteArray read_response_body_chunk() { return _read_response_body_chunk(); }
	virtual void set_blocking_mode(bool enabled) { _set_blocking_mode(enabled); }
	virtual bool is_blocking_mode_enabled() const { return _is_blocking_mode_enabled(); }
	virtual void set_read_chunk_size(int32_t chunk_size) { return _set_read_chunk_size(chunk_size); }
	virtual int32_t get_read_chunk_size() const { return _get_read_chunk_size(); }
	virtual Error poll() { return _poll(); }
	virtual void set_http_proxy(const String &host, int32_t port) { _set_http_proxy(host, port); }
	virtual void set_https_proxy(const String &host, int32_t port) { _set_https_proxy(host, port); }

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("connect_to_host", "host", "port", "tls_options"), &HTTPClientExtensionCompat::connect_to_host, DEFVAL(-1), DEFVAL(Ref<TLSOptionsCompat>()));
		ClassDB::bind_method(D_METHOD("set_connection", "connection"), &HTTPClientExtensionCompat::set_connection);
		ClassDB::bind_method(D_METHOD("get_connection"), &HTTPClientExtensionCompat::get_connection);
		ClassDB::bind_method(D_METHOD("request_raw", "method", "url", "headers", "body"), &HTTPClientExtensionCompat::_request_raw);
		ClassDB::bind_method(D_METHOD("request", "method", "url", "headers", "body"), &HTTPClientExtensionCompat::_request_string, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("close"), &HTTPClientExtensionCompat::close);

		ClassDB::bind_method(D_METHOD("has_response"), &HTTPClientExtensionCompat::has_response);
		ClassDB::bind_method(D_METHOD("is_response_chunked"), &HTTPClientExtensionCompat::is_response_chunked);
		ClassDB::bind_method(D_METHOD("get_response_code"), &HTTPClientExtensionCompat::get_response_code);
		ClassDB::bind_method(D_METHOD("get_response_headers"), &HTTPClientExtensionCompat::get_response_headers);
		ClassDB::bind_method(D_METHOD("get_response_body_length"), &HTTPClientExtensionCompat::get_response_body_length);
		ClassDB::bind_method(D_METHOD("read_response_body_chunk"), &HTTPClientExtensionCompat::read_response_body_chunk);
		ClassDB::bind_method(D_METHOD("set_read_chunk_size", "bytes"), &HTTPClientExtensionCompat::set_read_chunk_size);
		ClassDB::bind_method(D_METHOD("get_read_chunk_size"), &HTTPClientExtensionCompat::get_read_chunk_size);

		ClassDB::bind_method(D_METHOD("set_blocking_mode", "enabled"), &HTTPClientExtensionCompat::set_blocking_mode);
		ClassDB::bind_method(D_METHOD("is_blocking_mode_enabled"), &HTTPClientExtensionCompat::is_blocking_mode_enabled);

		ClassDB::bind_method(D_METHOD("get_status"), &HTTPClientExtensionCompat::get_status);
		ClassDB::bind_method(D_METHOD("poll"), &HTTPClientExtensionCompat::poll);

		ClassDB::bind_method(D_METHOD("set_http_proxy", "host", "port"), &HTTPClientExtensionCompat::set_http_proxy);
		ClassDB::bind_method(D_METHOD("set_https_proxy", "host", "port"), &HTTPClientExtensionCompat::set_https_proxy);

		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "blocking_mode_enabled"), "set_blocking_mode", "is_blocking_mode_enabled");
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "connection", PROPERTY_HINT_RESOURCE_TYPE, "StreamPeer", PROPERTY_USAGE_NONE), "set_connection", "get_connection");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "read_chunk_size", PROPERTY_HINT_RANGE, "256,16777216"), "set_read_chunk_size", "get_read_chunk_size");
	}

public:
	// Actual overrides (auto-generated from post extension class PR).
	virtual Error _request(HTTPClient::Method method, const String &url, const PackedStringArray &headers, const uint8_t *body, int32_t body_size);
	virtual Error _connect_to_host(const String &host, int32_t port, const Ref<TLSOptionsCompat> &tls_options);
	virtual void _set_connection(const Ref<StreamPeer> &connection);
	virtual Ref<StreamPeer> _get_connection() const;
	virtual void _close();
	virtual HTTPClient::Status _get_status() const;
	virtual bool _has_response() const;
	virtual bool _is_response_chunked() const;
	virtual int32_t _get_response_code() const;
	virtual PackedStringArray _get_response_headers();
	virtual int64_t _get_response_body_length() const;
	virtual PackedByteArray _read_response_body_chunk();
	virtual void _set_blocking_mode(bool enabled);
	virtual bool _is_blocking_mode_enabled() const;
	virtual void _set_read_chunk_size(int32_t chunk_size);
	virtual int32_t _get_read_chunk_size() const;
	virtual Error _poll();
	virtual void _set_http_proxy(const String &host, int32_t port);
	virtual void _set_https_proxy(const String &host, int32_t port);
};

} // namespace godot

#endif // ! GODOT_CPP_HTTP_CLIENT_EXTENSION_HPP
