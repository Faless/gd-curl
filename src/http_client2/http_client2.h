/**************************************************************************/
/*  http_client2.h                                                        */
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

#include <godot_cpp/classes/http_client.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

class HTTPRequest2 : public RefCounted {
	GDCLASS(HTTPRequest2, RefCounted);

protected:
	static void _bind_methods();

public:
	virtual PackedStringArray get_headers() const = 0;
	virtual PackedByteArray get_response() const = 0;
	virtual bool has_headers() const = 0;
	virtual bool has_response() const = 0;
	virtual int get_response_code() const = 0;
	virtual ~HTTPRequest2() {}
};

class HTTPClient2 : public RefCounted {
	GDCLASS(HTTPClient2, RefCounted);

protected:
	static HTTPClient2 *(*_create)();

	Ref<TLSOptions> tls_options;

	static void _bind_methods();

public:
	static Ref<HTTPClient2> create();

	virtual Ref<TLSOptions> get_tls_options() const { return tls_options; }
	virtual void set_tls_options(Ref<TLSOptions> p_tls) { tls_options = p_tls; }

	virtual Ref<HTTPRequest2> fetch(const String &p_url, HTTPClient::Method p_method, const PackedStringArray &p_headers, const PackedByteArray &p_body) = 0;

	virtual void cancel(uint64_t p_request_id) = 0;
	virtual void poll() = 0;
	virtual ~HTTPClient2() {}
};

}; // namespace godot
