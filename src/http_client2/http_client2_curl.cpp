/**************************************************************************/
/*  http_client2_curl.cpp                                                 */
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

#include "http_client2_curl.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/version.hpp>
#include <godot_cpp/templates/list.hpp>

#define VERSION_BRANCH _MKSTR(GODOT_VERSION_MAJOR) "." _MKSTR(GODOT_VERSION_MINOR)
#define VERSION_NUMBER VERSION_BRANCH "." _MKSTR(GODOT_VERSION_PATCH)
#define VERSION_FULL_BUILD VERSION_NUMBER

namespace godot {

CharString HTTPClient2Curl::system_cas;
CharString HTTPClient2Curl::user_agent;
bool HTTPClient2Curl::enable_http3 = false;

PackedStringArray HTTPRequest2Curl::get_headers() const {
	return headers;
}

bool HTTPRequest2Curl::has_headers() const {
	return headers_over && headers.size();
}

bool HTTPRequest2Curl::has_response() const {
	return complete && response->get_size() > 0;
}

PackedByteArray HTTPRequest2Curl::get_response() const {
	return response->get_data_array();
}

int HTTPRequest2Curl::get_response_code() const {
	return response_code;
}

char const *HTTPClient2Curl::methods[HTTPClient::METHOD_MAX + 1] = {
	"GET",
	"HEAD",
	"POST",
	"PUT",
	"DELETE",
	"OPTIONS",
	"TRACE",
	"CONNECT",
	"PATCH",
	"MAX",
};

HTTPClient2 *HTTPClient2Curl::_create() {
	return memnew(HTTPClient2Curl);
}

void HTTPClient2Curl::initialize(bool p_enable_http3) {
	system_cas = OS::get_singleton()->get_system_ca_certificates().utf8();
	user_agent = ("User-Agent: GodotEngine/" + String(VERSION_FULL_BUILD) + +" (" + OS::get_singleton()->get_name() + ") (cURL)").utf8();
	enable_http3 = p_enable_http3;
	HTTPClient2::_create = HTTPClient2Curl::_create;
}

void HTTPClient2Curl::deinitialize() {
	system_cas = CharString();
	user_agent = CharString();
}

size_t HTTPClient2Curl::_read_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	size_t size = p_size * p_nitems;
	if (size == 0) {
		return 0;
	}

	CURL *eh = (CURL *)p_userdata;
	HTTPRequest2Curl *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	ERR_FAIL_COND_V(req->headers_over, 0); // Likely a bug.

	Array arr = req->request->get_partial_data(size);
	PackedByteArray pba = arr[1].operator PackedByteArray();
	if (pba.size()) {
		memcpy(p_buffer, pba.ptrw(), pba.size());
	}

	return curl_off_t(pba.size());
}

size_t HTTPClient2Curl::_header_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	// https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html
	size_t size = p_size * p_nitems;

	CURL *eh = (CURL *)p_userdata;
	HTTPRequest2Curl *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	if (size == 2) {
		int rc = 0;
		if (req->headers.size() && req->headers[0].to_lower().begins_with("http")) {
			rc = req->headers[0].split(" ")[1].to_int();
		}
		if (rc == 100) {
			req->headers.clear(); // A new request will follow.
		} else {
			req->response_code = rc;
			req->headers_over = true;
		}
	} else {
		req->headers.push_back(String::utf8(p_buffer, size - 2)); // Strip "\r\n"
	}

	return curl_off_t(size);
}

size_t HTTPClient2Curl::_write_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	size_t size = p_size * p_nitems;
	if (size == 0) {
		return 0;
	}

	CURL *eh = (CURL *)p_userdata;
	HTTPRequest2Curl *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	PackedByteArray pba;
	pba.resize(size);
	memcpy(pba.ptrw(), p_buffer, size);
	req->response->put_data(pba);
	return curl_off_t(pba.size());
}

bool HTTPClient2Curl::_init_request_headers(CURL *p_handle, const PackedStringArray &p_headers) {
	bool add_uagent = true;

	curl_slist *curl_headers = nullptr;
	for (int i = 0; i < p_headers.size(); i++) {
		String h = p_headers[i];
		curl_headers = curl_slist_append(curl_headers, h.utf8().get_data());
		h = h.to_lower();
	}

	if (curl_headers) {
		CURLcode return_code = curl_easy_setopt(p_handle, CURLOPT_HTTPHEADER, curl_headers);
		if (return_code != CURLE_OK) {
			curl_slist_free_all(curl_headers);
			ERR_PRINT("failed to set request headers: " + itos(return_code));
			return true;
		}
	}
	return false;
}

Ref<HTTPRequest2> HTTPClient2Curl::fetch(const String &p_url, HTTPClient::Method p_method, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
	int request_body_size = p_body.size();
	ERR_FAIL_COND_V_MSG(request_body_size && p_method != HTTPClient::METHOD_POST && p_method != HTTPClient::METHOD_PUT && p_method != HTTPClient::METHOD_PATCH, Ref<HTTPRequest2>(), "Selected method does not support request body");
	ERR_FAIL_COND_V_MSG(p_method < 0 || p_method > 9, Ref<HTTPRequest2>(), "Invalid method.");
	CURL *eh = curl_easy_init();
	curl_easy_setopt(eh, CURLOPT_URL, p_url.utf8().get_data());
	curl_easy_setopt(eh, CURLOPT_USERAGENT, user_agent.get_data());
	curl_easy_setopt(eh, CURLOPT_CUSTOMREQUEST, methods[p_method]);
	if (enable_http3) {
		curl_easy_setopt(eh, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3);
	}
	curl_easy_setopt(eh, CURLOPT_ACCEPT_ENCODING, ""); // Enables built-in decompressions.
	if (p_method == HTTPClient::METHOD_HEAD) {
		curl_easy_setopt(eh, CURLOPT_NOBODY, 1L);
	}

	bool ssl = p_url.begins_with("https://");

	if (ssl) {
		curl_easy_setopt(eh, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	}

	if (tls_options.is_valid() && tls_options->get_trusted_ca_chain().is_valid()) {
		// Custom CAs for request
		String ca_cert = tls_options->get_trusted_ca_chain()->save_to_string();
		curl_blob ca_blob;
		const CharString cs = ca_cert.utf8();
		ca_blob.data = (void *)cs.get_data();
		ca_blob.len = cs.size();
		ca_blob.flags = CURL_BLOB_COPY;
		curl_easy_setopt(eh, CURLOPT_CAINFO_BLOB, &ca_blob);

	} else if (system_cas.size() > 0) {
		// Default static CAs
		curl_blob ca_blob;
		ca_blob.data = (void *)system_cas.get_data();
		ca_blob.len = system_cas.size();
		ca_blob.flags = CURL_BLOB_NOCOPY;
		curl_easy_setopt(eh, CURLOPT_CAINFO_BLOB, &ca_blob);
	}

	bool err = _init_request_headers(eh, p_headers);
	if (err) {
		curl_easy_cleanup(eh);
		ERR_FAIL_V_MSG(nullptr, "Failed to set request headers.");
	}

	Ref<HTTPRequest2Curl> req;
	req.instantiate();
	req->request_id = last_request_id++;
	req->handle = eh;
	req->response.instantiate();
	requests[req->get_request_id()] = req;

	// Initialize callbacks.
	curl_easy_setopt(eh, CURLOPT_PRIVATE, req.ptr());
	curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, _header_callback);
	curl_easy_setopt(eh, CURLOPT_HEADERDATA, eh);
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, _write_callback);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, eh);
#ifdef DEV_ENABLED
	curl_easy_setopt(eh, CURLOPT_VERBOSE, 1);
#endif
	if (request_body_size) {
		req->request.instantiate();
		req->request->set_data_array(p_body);
		// Set request read callback.
		curl_easy_setopt(eh, CURLOPT_READFUNCTION, _read_callback);
		curl_easy_setopt(eh, CURLOPT_READDATA, eh);
		// Tell curl we want to "POST" so it calls the read function.
		// The real HTTP verb is set via CURLOPT_CUSTOMREQUEST above.
		curl_easy_setopt(eh, CURLOPT_POST, 1);
		curl_easy_setopt(eh, CURLOPT_POSTFIELDSIZE, request_body_size);
	}

	curl_multi_add_handle(curl, eh);

	return req;
}

void HTTPClient2Curl::cancel(uint64_t p_request_id) {
	if (!requests.has(p_request_id)) {
		return;
	}
	Ref<HTTPRequest2Curl> req = requests[p_request_id];
	curl_multi_remove_handle(curl, req->handle);
	curl_easy_cleanup(req->handle);
	req->completed();
	requests.erase(p_request_id);
}

void HTTPClient2Curl::poll() {
	if (curl == nullptr) {
		return;
	}
	int running_handles = 0;
	CURLMcode rc = curl_multi_perform(curl, &running_handles);
#if 0
	if (still_running) {
		rc = curl_multi_wait(curl, nullptr, 0, 0, nullptr);
	}
#endif

	List<HTTPRequest2Curl *> completed;
	struct CURLMsg *m;
	do {
		int msgq = 0;
		m = curl_multi_info_read(curl, &msgq);
		if (m && (m->msg == CURLMSG_DONE)) {
			CURL *eh = m->easy_handle;
			/* m->data.result holds the error code for the transfer */
			HTTPRequest2Curl *req = nullptr;
			curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);
			req->success = true;
			completed.push_back(req);
			curl_multi_remove_handle(curl, eh);
			curl_easy_cleanup(eh);
		}
	} while (m);

	for (HTTPRequest2Curl *req : completed) {
		req->completed();
		requests.erase(req->get_request_id());
	}
}

void HTTPRequest2Curl::completed() {
	if (complete) {
		return;
	}
	complete = true;
	emit_signal("completed");
}

HTTPClient2Curl::HTTPClient2Curl() {
	curl = curl_multi_init();
}

HTTPClient2Curl::~HTTPClient2Curl() {
	if (curl != nullptr) {
		curl_multi_cleanup(curl);
	}
	for (KeyValue<uint64_t, Ref<HTTPRequest2Curl>> &E : requests) {
		E.value->completed();
		curl_easy_cleanup(E.value->handle);
	}
	requests.clear();
}

} //namespace godot
