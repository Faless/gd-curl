/**************************************************************************/
/*  http_client2_curl.h                                                   */
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

#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include <curl/curl.h>

namespace godot {

class HTTPRequest2Curl : public HTTPRequest2 {
	GDCLASS(HTTPRequest2Curl, HTTPRequest2);

private:
	friend class HTTPClient2Curl;

	CURL *handle = nullptr;

	uint64_t request_id = 0;
	PackedStringArray headers;
	Ref<StreamPeerBuffer> response;
	int response_code = 0;
	Ref<StreamPeerBuffer> request;
	bool headers_over = false;
	bool complete = false;
	bool success = false;

protected:
	static void _bind_methods() {}

	String _to_string() const {
		return "<HTTPRequest2Curl#" + itos(get_instance_id()) + ">";
	}

public:
	virtual PackedStringArray get_headers() const override;
	virtual bool has_headers() const override;
	virtual bool has_response() const override;
	virtual PackedByteArray get_response() const override;
	virtual int get_response_code() const override;

	void completed();
	uint64_t get_request_id() {
		return request_id;
	}

	~HTTPRequest2Curl() {}
	HTTPRequest2Curl() {}
};

class HTTPClient2Curl : public HTTPClient2 {
	GDCLASS(HTTPClient2Curl, HTTPClient2);

protected:
	static void _bind_methods() {}

	String _to_string() const {
		return "<HTTPClient2Curl#" + itos(get_instance_id()) + ">";
	}

private:
	static const char *methods[HTTPClient::METHOD_MAX + 1];
	static CharString system_cas;
	static CharString user_agent;
	static bool enable_http3;

	static size_t _read_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata);
	static size_t _header_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata);
	static size_t _write_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata);

	CURLM *curl = nullptr;
	HashMap<uint64_t, Ref<HTTPRequest2Curl>> requests;
	uint64_t last_request_id = 1;

	bool _init_request_headers(CURL *p_handle, const PackedStringArray &p_headers);

	static HTTPClient2 *_create();

public:
	static void initialize(bool p_enable_http3);
	static void deinitialize();

	virtual Ref<HTTPRequest2> fetch(const String &p_url, HTTPClient::Method p_method, const PackedStringArray &p_headers, const PackedByteArray &p_body) override;

	virtual void cancel(uint64_t p_request_id) override;
	virtual void poll() override;
	~HTTPClient2Curl();
	HTTPClient2Curl();
};

}; // namespace godot
