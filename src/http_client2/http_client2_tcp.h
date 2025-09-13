/**************************************************************************/
/*  http_client2_tcp.h                                                    */
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

#include "http_client2.h"

#include <godot_cpp/classes/ip.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <godot_cpp/classes/stream_peer_tcp.hpp>
#include <godot_cpp/classes/stream_peer_tls.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace godot {

class HTTPRequest2TCP : public HTTPRequest2 {
	GDCLASS(HTTPRequest2TCP, HTTPRequest2);

private:
	friend class HTTPClient2TCP;

	static const char *methods[HTTPClient::METHOD_MAX + 1];

	struct RequestData {
		String scheme;
		String host;
		int port = 0;
		String request_string;
		HTTPClient::Method method;
		PackedStringArray headers;
		PackedByteArray body;
		String proxy;
	};

	Ref<StreamPeerTCP> tcp;
	Ref<StreamPeerTLS> tls;
	Ref<StreamPeer> stream;
	int resolver_id = IP::RESOLVER_INVALID_ID;
	Ref<TLSOptions> tls_options;
	String tls_hostname;

	bool chunked = false;
	int body_size = -1;
	int body_left = 0;
	int chunk_left = 0;
	bool read_until_eof = false;

	uint64_t request_id = 0;
	PackedStringArray headers;
	PackedByteArray header_buffer;
	int header_buffer_pos = 0;
	Ref<StreamPeerBuffer> response;
	int response_code = 0;
	Ref<StreamPeerBuffer> request;
	bool head = false;
	bool headers_over = false;
	bool complete = false;
	bool success = false;

	static void _build_request(const RequestData &p_data, Ref<StreamPeerBuffer> r_output);

	void _poll();
	void _parse_header(const String &p_header);
	void _parse_response_chunk(const PackedByteArray &p_chunk);
	void _parse_chunk_header(const PackedByteArray &p_chunk);
	void _parse_chunk_body(const PackedByteArray &p_chunk);

protected:
	static void _bind_methods() {}

	String _to_string() const {
		return "<HTTPRequest2TCP#" + itos(get_instance_id()) + ">";
	}

public:
	virtual PackedStringArray get_headers() const override;
	virtual bool has_headers() const override;
	virtual bool has_response() const override;
	virtual PackedByteArray get_response() const override;
	virtual int get_response_code() const override;

	void completed(bool p_success);
	uint64_t get_request_id() {
		return request_id;
	}

	~HTTPRequest2TCP() {}
	HTTPRequest2TCP() {}
};

class HTTPClient2TCP : public HTTPClient2 {
	GDCLASS(HTTPClient2TCP, HTTPClient2);

protected:
	static void _bind_methods() {}

	String _to_string() const {
		return "<HTTPClient2TCP#" + itos(get_instance_id()) + ">";
	}

private:
	HashMap<uint64_t, Ref<HTTPRequest2TCP>> requests;
	uint64_t last_request_id = 1;
	String http_proxy;
	String https_proxy;

	static HTTPClient2 *_create();

public:
	static void initialize();
	static void deinitialize();

	virtual void set_proxy(ProxyType p_proxy_type, const String &p_proxy) override;
	virtual Ref<HTTPRequest2> fetch(const String &p_url, HTTPClient::Method p_method, const PackedStringArray &p_headers, const PackedByteArray &p_body) override;

	virtual void cancel(uint64_t p_request_id) override;
	virtual void poll() override;
	~HTTPClient2TCP();
	HTTPClient2TCP();
};

} // namespace godot
