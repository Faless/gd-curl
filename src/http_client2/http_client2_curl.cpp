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

#include "godot_cpp/classes/os.hpp"
#include <godot_cpp/templates/list.hpp>

#define VERSION_BRANCH _MKSTR(VERSION_MAJOR) "." _MKSTR(VERSION_MINOR)
#define VERSION_NUMBER VERSION_BRANCH "." _MKSTR(VERSION_PATCH)
#define VERSION_FULL_BUILD VERSION_NUMBER

namespace godot {

PackedStringArray HTTPRequest2Curl::get_headers() const {
	return headers;
}

bool HTTPRequest2Curl::has_headers() const {
	return headers.size();
}

bool HTTPRequest2Curl::has_response() const {
	return status == STATUS_COMPLETE && response->get_size() > 0;
}

PackedByteArray HTTPRequest2Curl::get_response() const {
	return response->get_data_array();
}

char const *HTTPClient2Curl::methods[10] = {
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

CharString HTTPClient2Curl::system_cas;

void HTTPClient2Curl::initialize() {
	// FIXME upstream
#if 0
	system_cas = OS::get_singleton()->get_system_ca_certificates().utf8();
#endif
}

size_t HTTPClient2Curl::_read_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	size_t size = p_size * p_nitems;
	if (size == 0) {
		return 0;
	}

	CURL *eh = (CURL *)p_userdata;
	HTTPRequest2Curl *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	// From new to requesting.
	if (req->status == HTTPRequest2Curl::STATUS_NEW) {
		req->status = HTTPRequest2Curl::STATUS_REQUESTING;
	}
	ERR_FAIL_COND_V(req->status != HTTPRequest2Curl::STATUS_REQUESTING, 0);

	Array arr = req->request->get_partial_data(size);
	PackedByteArray pba = arr[1].operator PackedByteArray();
	if (pba.size()) {
		memcpy(p_buffer, pba.ptr(), pba.size());
	}

	return curl_off_t(pba.size());
}

size_t HTTPClient2Curl::_header_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	size_t size = p_size * p_nitems;
	if (size == 0) {
		return 0;
	}

	CURL *eh = (CURL *)p_userdata;
	HTTPRequest2Curl *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	// From requesting to headers.
	if (req->status == HTTPRequest2Curl::STATUS_REQUESTING) {
		req->status = HTTPRequest2Curl::STATUS_HEADERS;
	}
	ERR_FAIL_COND_V(req->status != HTTPRequest2Curl::STATUS_HEADERS, 0);

	PackedByteArray pba;
	pba.resize(size);
	memcpy(pba.ptrw(), p_buffer, size);
	req->response->put_data(pba);
	return curl_off_t(pba.size());
}

size_t HTTPClient2Curl::_write_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	size_t size = p_size * p_nitems;
	if (size == 0) {
		return 0;
	}

	CURL *eh = (CURL *)p_userdata;
	HTTPRequest2Curl *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	// From headers to body.
	if (req->status == HTTPRequest2Curl::STATUS_HEADERS) {
		req->status = HTTPRequest2Curl::STATUS_BODY;
		req->headers = req->response->get_data_array().get_string_from_utf8().split("\r\n");
		req->response->clear();
		req->status = HTTPRequest2Curl::STATUS_BODY;
	}
	ERR_FAIL_COND_V(req->status != HTTPRequest2Curl::STATUS_BODY, 0);

	PackedByteArray pba;
	pba.resize(size);
	memcpy(pba.ptrw(), p_buffer, size);
	req->response->put_data(pba);
	return curl_off_t(pba.size());
}

Error HTTPClient2Curl::_init_request_headers(CURL *p_handle, const PackedStringArray &p_headers, int p_clen) {
	bool add_clen = p_clen > 0;
	bool add_uagent = true;
	bool add_accept = true;
	curl_slist *curl_headers = nullptr;
	for (int i = 0; i < p_headers.size(); i++) {
		curl_headers = curl_slist_append(curl_headers, p_headers[i].ascii().get_data());
		String h = p_headers[i].to_lower();

		if (add_clen && h.findn("content-length:") == 0) {
			add_clen = false;
		}
		if (add_uagent && h.findn("user-agent:") == 0) {
			add_uagent = false;
		}
		if (add_accept && h.findn("accept:") == 0) {
			add_accept = false;
		}
	}

	// Add default headers.
	if (add_clen) {
		curl_headers = curl_slist_append(curl_headers, ("Content-Length: " + itos(p_clen)).ascii().get_data());
	}

	if (add_uagent) {
		const String uagent = "User-Agent: GodotEngine/" + String(VERSION_FULL_BUILD) + " (" + OS::get_singleton()->get_name() + ")";
		curl_headers = curl_slist_append(curl_headers, uagent.ascii().get_data());
	}

	if (add_accept) {
		curl_headers = curl_slist_append(curl_headers, String("Accept: */*").ascii().get_data());
	}

	if (curl_headers) {
		CURLcode return_code = curl_easy_setopt(p_handle, CURLOPT_HTTPHEADER, curl_headers);
		if (return_code != CURLE_OK) {
			curl_slist_free_all(curl_headers);
			ERR_PRINT("failed to set request headers: " + String::num_uint64(return_code));
			return FAILED;
		}
	}
	return OK;
}

Ref<HTTPRequest2> HTTPClient2Curl::fetch(const String &p_url, HTTPClient::Method p_method, const PackedStringArray &p_headers, const PackedByteArray &p_request) {
	int request_body_size = p_request.size();
	ERR_FAIL_COND_V_MSG(request_body_size && p_method != HTTPClient::METHOD_POST && p_method != HTTPClient::METHOD_PUT, Ref<HTTPRequest2>(), "Selected method does not support request body");
	ERR_FAIL_COND_V_MSG(p_method < 0 || p_method > 9, Ref<HTTPRequest2>(), "Invalid method.");
	CURL *eh = curl_easy_init();
	curl_easy_setopt(eh, CURLOPT_URL, p_url.utf8().get_data());
	curl_easy_setopt(eh, CURLOPT_CUSTOMREQUEST, methods[p_method]);
	if (p_method == HTTPClient::Method::METHOD_HEAD) {
		curl_easy_setopt(eh, CURLOPT_NOBODY, 1L);
	}
	// curl_easy_setopt(eh, CURLOPT_BUFFERSIZE, read_chunk_size); // TODO As needed

	bool ssl = p_url.begins_with("https://");

	if (ssl) {
		curl_easy_setopt(eh, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	}

	if (tls_options.is_valid()) {
		// FIXME Upstream
		bool insecure = false;
		bool skip_host_verify = false;
		Ref<X509Certificate> ca_cert;
		if (insecure) {
			curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0L);
		}
		if (skip_host_verify) {
			curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, 0L);
		}
		if (ca_cert.is_valid()) {
			CharString ca_cert_cs = ca_cert->save_to_string().utf8();
			curl_blob ca_blob;
			ca_blob.data = (uint8_t *)ca_cert_cs.get_data();
			ca_blob.len = ca_cert_cs.size();
			ca_blob.flags = CURL_BLOB_COPY;
			curl_easy_setopt(eh, CURLOPT_CAINFO_BLOB, &ca_blob);
		}
	} else if (system_cas.size() > 0) {
		// Default static CAs
		curl_blob ca_blob;
		ca_blob.data = (uint8_t *)system_cas.get_data();
		ca_blob.len = system_cas.size();
		ca_blob.flags = CURL_BLOB_NOCOPY;
		curl_easy_setopt(eh, CURLOPT_CAINFO_BLOB, &ca_blob);
	}

	Error err = _init_request_headers(eh, p_headers, request_body_size);
	if (err != OK) {
		curl_easy_cleanup(eh);
		ERR_FAIL_V(Ref<HTTPRequest2>());
	}

	Ref<HTTPRequest2Curl> req;
	req.instantiate();
	req->handle = eh;
	req->status = HTTPRequest2Curl::STATUS_REQUESTING; // TODO
	req->response.instantiate();
	requests[req->get_instance_id()] = req;

	// Initialize callbacks.
	curl_easy_setopt(eh, CURLOPT_PRIVATE, req.ptr());
	curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, _header_callback);
	curl_easy_setopt(eh, CURLOPT_HEADERDATA, eh);
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, _write_callback);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, eh);
	if (request_body_size) {
		req->request.instantiate();
		req->request->set_data_array(p_request);
		// Set request read callback.
		curl_easy_setopt(eh, CURLOPT_READFUNCTION, _read_callback);
		curl_easy_setopt(eh, CURLOPT_READDATA, eh);
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
	req->status = HTTPRequest2Curl::STATUS_COMPLETE;
	req->emit_signal("completed");
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
			req->status = HTTPRequest2Curl::STATUS_COMPLETE;
			req->success = true;
			completed.push_back(req);
			curl_multi_remove_handle(curl, eh);
			curl_easy_cleanup(eh);
		}
	} while (m);

	for (HTTPRequest2Curl *req : completed) {
		req->emit_signal("completed");
		requests.erase(req->get_instance_id());
	}
}

HTTPClient2Curl::HTTPClient2Curl() {
	curl = curl_multi_init();
}

HTTPClient2Curl::~HTTPClient2Curl() {
	if (curl != nullptr) {
		curl_multi_cleanup(curl);
	}
	for (KeyValue<uint64_t, Ref<HTTPRequest2Curl>> &E : requests) {
		E.value->status = HTTPRequest2Curl::STATUS_COMPLETE;
		E.value->emit_signal("completed");
		curl_easy_cleanup(E.value->handle);
	}
	requests.clear();
}

}; //namespace godot
