/**************************************************************************/
/*  http_client_curl.h                                                    */
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

#ifdef HTTP_CLIENT_EXTENSION_COMPAT
#include "compat/http_client_extension.hpp"
#include "compat/tls_options.hpp"
#define HTTPClientExtension HTTPClientExtensionCompat
#define TLSOptions TLSOptionsCompat
#else
#include "godot_cpp/classes/http_client_extension.hpp"
#include "godot_cpp/classes/tls_options.hpp"
#endif

#include "godot_cpp/classes/ip.hpp"
#include "godot_cpp/classes/stream_peer_buffer.hpp"
#include "ring_buffer.h"

#include <curl/curl.h>
#include <stdio.h>

namespace godot {

class HTTPClientCurl : public HTTPClientExtension {
	GDCLASS(HTTPClientCurl, HTTPClientExtension);

protected:
	static void _bind_methods() {}

	godot::String _to_string() const {
		return "HTTPClientCurl";
	}

private:
	class CURLRequest : public RefCounted {
	public:
		enum Status {
			STATUS_NEW,
			STATUS_REQUESTING,
			STATUS_HEADERS,
			STATUS_BODY,
			STATUS_COMPLETE,
		};

		int body_size = -1;
		int response_code = 0;
		bool chunked = false;
		bool head = false;
		bool error = false;
		CURL *handle = nullptr;
		Status status = STATUS_NEW;
		PackedStringArray headers;
		Ref<StreamPeerBuffer> request;
		Ref<StreamPeerBuffer> response;
		RingBuffer<uint8_t> rb;
	};

	static CharString system_cas;
	static CharString user_agent;
	static bool enable_http3;

	static const char *methods[10];
	static size_t _header_callback(char *buffer, size_t size, size_t nitems, void *userdata);
	static size_t _read_callback(char *buffer, size_t size, size_t nitems, void *userdata);
	static size_t _write_callback(char *buffer, size_t size, size_t nitems, void *userdata);

	Ref<CURLRequest> current_request;
	Ref<TLSOptions> tls_options;
	CURLM *curl = nullptr;
	struct curl_slist *resolve_list = nullptr;
	String connected_host;
	uint16_t connected_port;
	int read_chunk_size = 65536;
	bool blocking_mode = false;
	int resolver_id = IP::RESOLVER_INVALID_ID;

	Error _init_request_headers(CURL *p_handle, const PackedStringArray &p_headers, int p_clen);
	void _curl_transfer(int &running_handles);

public:
	static void initialize(bool p_enable_http3);
	static void deinitialize();

	virtual Error _request(HTTPClient::Method p_method, const String &p_url, const PackedStringArray &p_headers, const uint8_t *p_body, int32_t p_body_size) override;
	virtual Error _connect_to_host(const String &p_host, int32_t p_port, const Ref<TLSOptions> &p_options) override;
	virtual void _close() override;
	virtual void _set_connection(const Ref<StreamPeer> &p_connection) override { ERR_FAIL_MSG("Accessing an HTTPClientCurl's StreamPeer is not supported."); }
	virtual Ref<StreamPeer> _get_connection() const override { ERR_FAIL_V_MSG(Ref<StreamPeer>(), "Accessing an HTTPClientCurl's StreemPeer is not supported."); }

	HTTPClient::Status _get_status() const override;
	virtual bool _has_response() const override;
	virtual bool _is_response_chunked() const override;
	virtual int _get_response_code() const override;
	virtual PackedStringArray _get_response_headers() override;
	virtual int64_t _get_response_body_length() const override;
	virtual PackedByteArray _read_response_body_chunk() override;
	virtual void _set_blocking_mode(bool p_enabled) override { blocking_mode = p_enabled; }
	virtual bool _is_blocking_mode_enabled() const override { return blocking_mode; }
	virtual void _set_read_chunk_size(int p_size) override { read_chunk_size = CLAMP(p_size, 1024, CURL_MAX_READ_SIZE); }
	virtual int _get_read_chunk_size() const override { return read_chunk_size; }

	virtual Error _poll() override;

	virtual void _set_http_proxy(const String &host, int32_t port) override {} /* TODO */
	virtual void _set_https_proxy(const String &host, int32_t port) override {} /* TODO */

	HTTPClientCurl();
	virtual ~HTTPClientCurl();
};

}; // namespace godot
