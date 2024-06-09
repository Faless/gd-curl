/**************************************************************************/
/*  http_client_extension.cpp                                             */
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

// THIS FILE IS GENERATED. EDITS WILL BE LOST.

#include "http_client_extension.hpp"

#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

Error HTTPClientExtensionCompat::_request(HTTPClient::Method method, const String &url, const PackedStringArray &headers, const uint8_t *body, int32_t body_size) {
	return Error(0);
}

Error HTTPClientExtensionCompat::_connect_to_host(const String &host, int32_t port, const Ref<TLSOptionsCompat> &tls_options) {
	return Error(0);
}

void HTTPClientExtensionCompat::_set_connection(const Ref<StreamPeer> &connection) {}

Ref<StreamPeer> HTTPClientExtensionCompat::_get_connection() const {
	return Ref<StreamPeer>();
}

void HTTPClientExtensionCompat::_close() {}

HTTPClient::Status HTTPClientExtensionCompat::_get_status() const {
	return HTTPClient::Status(0);
}

bool HTTPClientExtensionCompat::_has_response() const {
	return false;
}

bool HTTPClientExtensionCompat::_is_response_chunked() const {
	return false;
}

int32_t HTTPClientExtensionCompat::_get_response_code() const {
	return 0;
}

PackedStringArray HTTPClientExtensionCompat::_get_response_headers() {
	return PackedStringArray();
}

int64_t HTTPClientExtensionCompat::_get_response_body_length() const {
	return 0;
}

PackedByteArray HTTPClientExtensionCompat::_read_response_body_chunk() {
	return PackedByteArray();
}

void HTTPClientExtensionCompat::_set_blocking_mode(bool enabled) {}

bool HTTPClientExtensionCompat::_is_blocking_mode_enabled() const {
	return false;
}

void HTTPClientExtensionCompat::_set_read_chunk_size(int32_t chunk_size) {}

int32_t HTTPClientExtensionCompat::_get_read_chunk_size() const {
	return 0;
}

Error HTTPClientExtensionCompat::_poll() {
	return Error(0);
}

void HTTPClientExtensionCompat::_set_http_proxy(const String &host, int32_t port) {}

void HTTPClientExtensionCompat::_set_https_proxy(const String &host, int32_t port) {}

} // namespace godot
