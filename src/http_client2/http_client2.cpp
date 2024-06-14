/**************************************************************************/
/*  http_client2.cpp                                                      */
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

#include "http_client2.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void HTTPRequest2::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_headers"), &HTTPRequest2::get_headers);
	ClassDB::bind_method(D_METHOD("has_headers"), &HTTPRequest2::has_headers);
	ClassDB::bind_method(D_METHOD("has_response"), &HTTPRequest2::has_response);
	ClassDB::bind_method(D_METHOD("get_response"), &HTTPRequest2::get_response);
	ADD_SIGNAL(MethodInfo("completed"));
}

void HTTPClient2::_bind_methods() {
	ClassDB::bind_method(D_METHOD("fetch", "url", "method", "headers", "request_data"), &HTTPClient2::fetch, DEFVAL(HTTPClient::METHOD_GET), DEFVAL(PackedStringArray()), DEFVAL(PackedByteArray()));
	ClassDB::bind_method(D_METHOD("cancel"), &HTTPClient2::cancel);
	ClassDB::bind_method(D_METHOD("poll"), &HTTPClient2::poll);
	ClassDB::bind_method(D_METHOD("set_tls_options", "tls_options"), &HTTPClient2::set_tls_options);
	ClassDB::bind_method(D_METHOD("get_tls_options"), &HTTPClient2::get_tls_options);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tls_options", PROPERTY_HINT_RESOURCE_TYPE, "TLSOptions"), "set_tls_options", "get_tls_options");
}
