/**************************************************************************/
/*  http_client_curl.cpp                                                  */
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

#include "godot_cpp/classes/ip.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/variant/string.hpp"

#include "http_client_curl.h"

#define VERSION_BRANCH _MKSTR(VERSION_MAJOR) "." _MKSTR(VERSION_MINOR)
#define VERSION_NUMBER VERSION_BRANCH "." _MKSTR(VERSION_PATCH)
#define VERSION_FULL_BUILD VERSION_NUMBER

namespace godot {

char const *HTTPClientCurl::methods[10] = {
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

CharString HTTPClientCurl::system_cas;

void HTTPClientCurl::initialize() {
#ifndef HTTP_CLIENT_EXTENSION_COMPAT
	system_cas = OS::get_singleton()->get_system_ca_certificates().utf8();
#endif
}

size_t HTTPClientCurl::_read_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	size_t size = p_size * p_nitems;
	if (size == 0) {
		return 0;
	}

	CURL *eh = (CURL *)p_userdata;
	CURLRequest *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	// From new to requesting.
	if (req->status == CURLRequest::STATUS_NEW) {
		req->status = CURLRequest::STATUS_REQUESTING;
	}
	ERR_FAIL_COND_V(req->status != CURLRequest::STATUS_REQUESTING, 0);

	Array arr = req->request->get_partial_data(size);
	PackedByteArray pba = arr[1].operator PackedByteArray();
	if (pba.size()) {
		memcpy(p_buffer, pba.ptr(), pba.size());
	}

	return curl_off_t(pba.size());
}

size_t HTTPClientCurl::_header_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	size_t size = p_size * p_nitems;
	if (size == 0) {
		return 0;
	}

	CURL *eh = (CURL *)p_userdata;
	CURLRequest *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	// From requesting to headers.
	if (req->status == CURLRequest::STATUS_REQUESTING) {
		req->status = CURLRequest::STATUS_HEADERS;
	}
	ERR_FAIL_COND_V(req->status != CURLRequest::STATUS_HEADERS, 0);

	PackedByteArray pba;
	pba.resize(size);
	memcpy(pba.ptrw(), p_buffer, size);
	req->response->put_data(pba);
	return curl_off_t(pba.size());
}

size_t HTTPClientCurl::_write_callback(char *p_buffer, size_t p_size, size_t p_nitems, void *p_userdata) {
	size_t size = p_size * p_nitems;
	if (size == 0) {
		return 0;
	}

	CURL *eh = (CURL *)p_userdata;
	CURLRequest *req = nullptr;
	curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);

	// From headers to body.
	if (req->status == CURLRequest::STATUS_HEADERS) {
		req->status = CURLRequest::STATUS_BODY;
		_parse_response_headers(req);
	}
	ERR_FAIL_COND_V(req->status != CURLRequest::STATUS_BODY, 0);

	return curl_off_t(req->rb.write((const uint8_t *)p_buffer, size));
}

void HTTPClientCurl::_parse_response_headers(CURLRequest *p_request) {
	p_request->headers = p_request->response->get_data_array().get_string_from_utf8().split("\r\n");
	p_request->response->clear();
	for (int i = 0; i < p_request->headers.size(); i++) {
		const String h = p_request->headers[i].to_lower();
		if (i == 0 && h.begins_with("http")) {
			String num = h.get_slicec(' ', 1);
			p_request->response_code = num.to_int();
		} else if (h.begins_with("content-length:") && !p_request->head) {
			p_request->body_size = h.substr(h.find(":") + 1, h.length()).strip_edges().to_int();
		} else if (h.begins_with("transfer-encoding:")) {
			String encoding = h.substr(h.find(":") + 1, h.length()).strip_edges();
			if (encoding == "chunked") {
				p_request->chunked = true;
			}
		}
	}
}

Error HTTPClientCurl::_init_request_headers(CURL *p_handle, const PackedStringArray &p_headers, int p_clen) {
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

bool HTTPClientCurl::_has_response() const {
	return current_request.is_valid() && current_request->headers.size();
}

bool HTTPClientCurl::_is_response_chunked() const {
	return current_request.is_valid() ? current_request->chunked : false;
}

int HTTPClientCurl::_get_response_code() const {
	return current_request.is_valid() ? current_request->response_code : 0;
}

int64_t HTTPClientCurl::_get_response_body_length() const {
	if (is_response_chunked()) {
		return -1;
	}
	return current_request.is_valid() && !current_request->head ? current_request->body_size : 0;
}

PackedStringArray HTTPClientCurl::_get_response_headers() {
	return current_request.is_valid() ? current_request->headers : PackedStringArray();
}

Error HTTPClientCurl::_connect_to_host(const String &p_host, int32_t p_port, const Ref<TLSOptions> &p_options) {
	_close();
	if (!p_host.is_valid_ip_address()) {
		resolver_id = IP::get_singleton()->resolve_hostname_queue_item(p_host);
		if (resolver_id == IP::RESOLVER_INVALID_ID) {
			return ERR_CANT_RESOLVE;
		}
	}
	connected_host = p_host;
	connected_port = p_port;
	tls_options = p_options;
	return OK;
}

void HTTPClientCurl::_close() {
	if (current_request.is_valid() && current_request->status != CURLRequest::STATUS_COMPLETE) {
		curl_multi_remove_handle(curl, current_request->handle);
		curl_easy_cleanup(current_request->handle);
	}
	if (resolve_list != nullptr) {
		curl_slist_free_all(resolve_list);
		resolve_list = nullptr;
	}
	if (resolver_id != IP::RESOLVER_INVALID_ID) {
		IP::get_singleton()->erase_resolve_item(resolver_id);
		resolver_id = IP::RESOLVER_INVALID_ID;
	}
	connected_host = "";
	connected_port = 0;
	current_request.unref();
}

Error HTTPClientCurl::_request(HTTPClient::Method p_method, const String &p_url, const PackedStringArray &p_headers, const uint8_t *p_body, int32_t p_body_size) {
	ERR_FAIL_COND_V(_get_status() != HTTPClient::STATUS_CONNECTED, FAILED);
	bool use_ssl = connected_port == 443 || tls_options.is_valid();
	String host = connected_host;
	if (host.is_valid_ip_address() && host.find(":") != -1) {
		host = "[" + host + "]";
	}
	String url = (use_ssl ? "https://" : "http://") + host + ":" + itos(connected_port) + p_url;
	int request_body_size = p_body_size;
	ERR_FAIL_COND_V_MSG(request_body_size && p_method != HTTPClient::METHOD_POST && p_method != HTTPClient::METHOD_PUT, ERR_INVALID_PARAMETER, "Selected method does not support request body");
	ERR_FAIL_COND_V_MSG(p_method < 0 || p_method > 9, ERR_INVALID_PARAMETER, "Invalid method.");
	CURL *eh = curl_easy_init();
	curl_easy_setopt(eh, CURLOPT_URL, url.utf8().get_data());
	curl_easy_setopt(eh, CURLOPT_CUSTOMREQUEST, methods[p_method]);
	bool is_head = p_method == HTTPClient::Method::METHOD_HEAD;
	if (is_head) {
		curl_easy_setopt(eh, CURLOPT_NOBODY, 1L);
	}
	curl_easy_setopt(eh, CURLOPT_BUFFERSIZE, read_chunk_size);

	if (use_ssl) {
		curl_easy_setopt(eh, CURLOPT_USE_SSL, CURLUSESSL_ALL);
		if (system_cas.size() > 0) {
			// Default static CAs
			curl_blob ca_blob;
			ca_blob.data = (uint8_t *)system_cas.get_data();
			ca_blob.len = system_cas.size();
			ca_blob.flags = CURL_BLOB_NOCOPY;
			curl_easy_setopt(eh, CURLOPT_CAINFO_BLOB, &ca_blob);
		}
	}

	if (tls_options.is_valid()) {
		bool skip_host_verify = tls_options->is_unsafe_client();
		Ref<X509Certificate> ca_cert = tls_options->get_trusted_ca_chain();
		bool insecure = skip_host_verify && ca_cert.is_null();
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
	}

	Error err = _init_request_headers(eh, p_headers, request_body_size);
	if (err != OK) {
		curl_easy_cleanup(eh);
		ERR_FAIL_V(FAILED);
	}

	if (resolve_list != nullptr) {
		curl_easy_setopt(eh, CURLOPT_RESOLVE, resolve_list);
	}

	current_request.instantiate();
	current_request->handle = eh;
	current_request->status = CURLRequest::STATUS_REQUESTING;
	current_request->head = is_head;
	current_request->rb.resize(nearest_shift(read_chunk_size));
	current_request->response.instantiate();

	// Initialize callbacks.
	curl_easy_setopt(eh, CURLOPT_PRIVATE, current_request.ptr());
	curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, _header_callback);
	curl_easy_setopt(eh, CURLOPT_HEADERDATA, eh);
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, _write_callback);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, eh);
	if (request_body_size) {
		current_request->request.instantiate();
		PackedByteArray pba;
		pba.resize(p_body_size);
		memcpy(pba.ptrw(), p_body, p_body_size);
		current_request->request->set_data_array(pba);
		// Set request read callback.
		curl_easy_setopt(eh, CURLOPT_READFUNCTION, _read_callback);
		curl_easy_setopt(eh, CURLOPT_READDATA, eh);
	}

	curl_multi_add_handle(curl, eh);

	return OK;
}

Error HTTPClientCurl::_poll() {
	if (curl == nullptr) {
		return ERR_UNAVAILABLE;
	}
	if (resolver_id != IP::RESOLVER_INVALID_ID) {
		IP::ResolverStatus status = IP::get_singleton()->get_resolve_item_status(resolver_id);
		if (status == IP::RESOLVER_STATUS_WAITING) {
			return OK;
		}
		if (status != IP::RESOLVER_STATUS_DONE) {
			_close(); // Will also erase the resolver_id.
			return OK;
		}
		// Resolved.
		PackedStringArray addresses = IP::get_singleton()->get_resolve_item_addresses(resolver_id);
		IP::get_singleton()->erase_resolve_item(resolver_id);
		resolver_id = IP::RESOLVER_INVALID_ID;
		// Convert the resolve list to cURL format.
		for (int i = 0; i < addresses.size(); i++) {
			const String addr = addresses[i];
			if (addr.find(":") == -1) {
				continue;
			}
			addresses[i] = "[" + addr + "]";
		}
		String resolve = connected_host + ":" + itos(connected_port) + ":" + String(",").join(addresses);
		resolve_list = curl_slist_append(resolve_list, resolve.utf8().get_data());
	}
	if (!current_request.is_valid() || current_request->status >= CURLRequest::STATUS_BODY) {
		return OK;
	}
	int running_handles = 0;
	_curl_transfer(running_handles);
	return OK;
}

void HTTPClientCurl::_curl_transfer(int &running_handles) {
	if (blocking_mode) {
		int ready = 0;
		CURLMcode rc = curl_multi_wait(curl, nullptr, 0, 1000, &ready);
		if (!ready) {
			return;
		}
	}

	CURLMcode rc = curl_multi_perform(curl, &running_handles);

	struct CURLMsg *m;
	do {
		int msgq = 0;
		m = curl_multi_info_read(curl, &msgq);
		if (m && (m->msg == CURLMSG_DONE)) {
			CURL *eh = m->easy_handle;
			/* m->data.result holds the error code for the transfer */
			CURLRequest *req = nullptr;
			curl_easy_getinfo(eh, CURLINFO_PRIVATE, &req);
			curl_multi_remove_handle(curl, eh);
			curl_easy_cleanup(eh);
			req->handle = nullptr;
			if (req->status == CURLRequest::STATUS_HEADERS) {
				// Headers only.
				_parse_response_headers(req);
				req->status = CURLRequest::STATUS_BODY;
			}
			if (req->status != CURLRequest::STATUS_BODY) {
				// Error occurred.
				req->error = true;
			}
		}
	} while (m);
}

HTTPClient::Status HTTPClientCurl::_get_status() const {
	if (current_request.is_null()) {
		if (resolver_id != IP::RESOLVER_INVALID_ID) {
			return HTTPClient::STATUS_RESOLVING;
		}
		if (connected_host.is_empty()) {
			return HTTPClient::STATUS_DISCONNECTED;
		}
		if (connected_host.is_valid_ip_address()) {
			return HTTPClient::STATUS_CONNECTED;
		}
		return resolve_list == nullptr ? HTTPClient::STATUS_CANT_RESOLVE : HTTPClient::STATUS_CONNECTED;
	}
	if (current_request->error) {
		return HTTPClient::STATUS_CONNECTION_ERROR;
	}
	switch (current_request->status) {
		case CURLRequest::STATUS_NEW:
			return HTTPClient::STATUS_CONNECTED;
		case CURLRequest::STATUS_REQUESTING:
		case CURLRequest::STATUS_HEADERS:
			return HTTPClient::STATUS_REQUESTING;
		case CURLRequest::STATUS_BODY:
			return HTTPClient::STATUS_BODY;
		case CURLRequest::STATUS_COMPLETE:
			return HTTPClient::STATUS_CONNECTED;
		default:
			ERR_FAIL_V(HTTPClient::STATUS_DISCONNECTED);
	}
}

PackedByteArray HTTPClientCurl::_read_response_body_chunk() {
	ERR_FAIL_COND_V(current_request.is_null() || current_request->status < CURLRequest::STATUS_BODY, PackedByteArray());
	int left = current_request->rb.data_left();
	if (left == 0) {
		int running_handles = 0;
		_curl_transfer(running_handles);
		left = current_request->rb.data_left();
		if (!running_handles) {
			// This is the last chunk.
			current_request->status = CURLRequest::STATUS_COMPLETE;
		}
	}
	PackedByteArray pba;
	if (left == 0) {
		return pba;
	}
	pba.resize(left);
	current_request->rb.read(pba.ptrw(), left);
	return pba;
}

HTTPClientCurl::HTTPClientCurl() {
	curl = curl_multi_init();
}

HTTPClientCurl::~HTTPClientCurl() {
	_close();
	curl_multi_cleanup(curl);
}

}; //namespace godot
