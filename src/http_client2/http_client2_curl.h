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

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>

#include <godot_cpp/classes/stream_peer_buffer.hpp>

#include <curl/curl.h>
#include <stdio.h>

namespace godot {

class HTTPRequest2Curl : public HTTPRequest2 {
	GDCLASS(HTTPRequest2Curl, HTTPRequest2);

private:
	friend class HTTPClient2Curl;

	enum Status {
		STATUS_NEW,
		STATUS_REQUESTING,
		STATUS_HEADERS,
		STATUS_BODY,
		STATUS_COMPLETE,
	};

	CURL *handle = nullptr;

	Status status = STATUS_NEW;
	PackedStringArray headers;
	Ref<StreamPeerBuffer> response;
	Ref<StreamPeerBuffer> request;
	bool success = false;

protected:
	static void _bind_methods() {}

public:
	virtual PackedStringArray get_headers() const override;
	virtual bool has_headers() const override;
	virtual bool has_response() const override;
	virtual PackedByteArray get_response() const override;
	~HTTPRequest2Curl() {}
	HTTPRequest2Curl() {}
};

class HTTPClient2Curl : public HTTPClient2 {
	GDCLASS(HTTPClient2Curl, HTTPClient2);

private:
	static const char *methods[10];
	static CharString system_cas;

	static size_t _read_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata);
	static size_t _header_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata);
	static size_t _write_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata);

	CURLM *curl = nullptr;
	HashMap<uint64_t, Ref<HTTPRequest2Curl>> requests;

	Error _init_request_headers(CURL *p_handle, const PackedStringArray &p_headers, int p_clen);

protected:
	static void _bind_methods() {}

public:
	static void initialize();

	virtual Ref<HTTPRequest2> fetch(const String &p_url, HTTPClient::Method p_method, const PackedStringArray &p_headers, const PackedByteArray &p_request) override;

	virtual void cancel(uint64_t p_request_id) override;
	virtual void poll() override;
	~HTTPClient2Curl();
	HTTPClient2Curl();
};

}; //namespace godot
