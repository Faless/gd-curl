/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include <gdextension_interface.h>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "http_client_curl.h"

#include "http_client2/http_client2_curl.h"

#ifdef HTTP_CLIENT_EXTENSION_COMPAT
#include "compat/http_request.hpp"
#endif

#ifdef _WIN32
// See upstream godot-cpp GH-771.
#undef GDN_EXPORT
#define GDN_EXPORT __declspec(dllexport)
#endif

using namespace godot;

static bool curl_ok = false;

void register_gdcurl_extension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (code != CURLE_OK) {
		ERR_PRINT("Curl initialization failure");
	} else {
		curl_ok = true;
	}

#ifdef HTTP_CLIENT_EXTENSION_COMPAT
	GDREGISTER_ABSTRACT_CLASS(TLSOptionsCompat);
	GDREGISTER_ABSTRACT_CLASS(HTTPClientExtensionCompat);
#endif

	GDREGISTER_ABSTRACT_CLASS(HTTPClient2);
	GDREGISTER_ABSTRACT_CLASS(HTTPRequest2);

	HTTPClient2Curl::initialize();
	GDREGISTER_ABSTRACT_CLASS(HTTPRequest2Curl); // TODO Not needed?
	GDREGISTER_CLASS(HTTPClient2Curl);

	HTTPClientCurl::initialize();
	GDREGISTER_CLASS(HTTPClientCurl);
#ifndef HTTP_CLIENT_EXTENSION_COMPAT
	if (!Engine::get_singleton()->is_editor_hint()) {
		WARN_PRINT("Enabling cURL as default HTTPClient");
		HTTPClient::set_default_extension("HTTPClientCurl");
	}
#else
	GDREGISTER_CLASS(HTTPRequestCompat);
#endif
}

void unregister_gdcurl_extension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	HTTPClient2Curl::deinitialize();
	HTTPClientCurl::deinitialize();
	if (curl_ok) {
		curl_global_cleanup();
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT gdcurl_extension_init(const GDExtensionInterfaceGetProcAddress p_interface, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

	init_obj.register_initializer(register_gdcurl_extension_types);
	init_obj.register_terminator(unregister_gdcurl_extension_types);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
