/**************************************************************************/
/*  http_client2_tcp.cpp                                                  */
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

#include "http_client2_tcp.h"

#include "compat/compat.hpp"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/version.hpp>
#include <godot_cpp/templates/list.hpp>

#define VERSION_BRANCH _MKSTR(GODOT_VERSION_MAJOR) "." _MKSTR(GODOT_VERSION_MINOR)
#define VERSION_NUMBER VERSION_BRANCH "." _MKSTR(GODOT_VERSION_PATCH)
#define VERSION_FULL_BUILD VERSION_NUMBER

namespace godot {

HTTPClient2 *HTTPClient2TCP::_create() {
	return memnew(HTTPClient2TCP);
}

void HTTPClient2TCP::initialize() {
	HTTPClient2::_create = HTTPClient2TCP::_create;
}

void HTTPClient2TCP::deinitialize() {
}

Ref<HTTPRequest2> HTTPClient2TCP::fetch(const String &p_url, HTTPClient::Method p_method, const PackedStringArray &p_headers, const PackedByteArray &p_body) {
	HTTPRequest2TCP::RequestData r;
	Error err = godot_parse_url(p_url, r.scheme, r.host, r.port, r.request_string);
	ERR_FAIL_COND_V_MSG(err != OK, nullptr, vformat("Error parsing URL: '%s'.", p_url));
	r.request_string = r.request_string.is_empty() ? "/" : r.request_string;
	r.port = r.port > 0 ? r.port : (r.scheme == "https://" ? 443 : 80);
	r.body = p_body;
	r.headers = p_headers;
	r.method = p_method;

	Ref<HTTPRequest2TCP> req = Ref<HTTPRequest2TCP>(memnew(HTTPRequest2TCP));
	req->request_id = last_request_id++;
	req->response.instantiate();
	req->header_buffer.resize(1 << 16 /* TODO configure? */);
	req->request.instantiate();
	HTTPRequest2TCP::_build_request(r, req->request);
	if (p_body.size() > 0) {
		req->request->put_data(p_body);
	}
	req->request->seek(0);
	req->head = p_method == HTTPClient::METHOD_HEAD;
	req->tcp.instantiate();
	req->stream = req->tcp;
	if (r.scheme == "https://") {
		req->tls_hostname = r.host;
		req->tls_options = tls_options;
		req->tls.instantiate();
	}
	req->tcp->connect_to_host(r.host, r.port);
	requests[req->request_id] = req;

	return req;
}

void HTTPClient2TCP::set_proxy(ProxyType p_proxy_type, const String &p_proxy) {
	ERR_FAIL_COND(p_proxy_type > PROXY_TYPE_MAX);
	switch (p_proxy_type) {
		case PROXY_TYPE_HTTP:
			ERR_FAIL_COND(!p_proxy.is_empty() && !p_proxy.begins_with("http://"));
			http_proxy = p_proxy;
			break;
		case PROXY_TYPE_HTTPS:
			ERR_FAIL_COND(!p_proxy.is_empty() && !p_proxy.begins_with("https://"));
			https_proxy = p_proxy;
			break;
		default:
			ERR_FAIL_MSG("Invalid proxy type");
	}
}

void HTTPClient2TCP::cancel(uint64_t p_request_id) {
	if (!requests.has(p_request_id)) {
		return;
	}
	Ref<HTTPRequest2TCP> req = requests[p_request_id];
	req->completed(false);
	requests.erase(p_request_id);
}

void HTTPClient2TCP::poll() {
	List<int> to_remove;
	for (KeyValue<uint64_t, Ref<HTTPRequest2TCP>> &E : requests) {
		E.value->_poll();
		if (E.value->complete) {
			to_remove.push_back(E.value->request_id);
		}
	}
	for (const int &id : to_remove) {
		requests.erase(id);
	}
}

HTTPClient2TCP::HTTPClient2TCP() {
}

HTTPClient2TCP::~HTTPClient2TCP() {
	for (KeyValue<uint64_t, Ref<HTTPRequest2TCP>> &E : requests) {
		E.value->completed(false);
	}
	requests.clear();
}

///
/// HTTPRequest2TCP
///
char const *HTTPRequest2TCP::methods[HTTPClient::METHOD_MAX + 1] = {
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

PackedStringArray HTTPRequest2TCP::get_headers() const {
	return headers;
}

bool HTTPRequest2TCP::has_headers() const {
	return headers_over && headers.size();
}

bool HTTPRequest2TCP::has_response() const {
	return complete && response->get_size() > 0;
}

PackedByteArray HTTPRequest2TCP::get_response() const {
	return response->get_data_array();
}

int HTTPRequest2TCP::get_response_code() const {
	return response_code;
}

void HTTPRequest2TCP::completed(bool p_success) {
	if (complete) {
		return;
	}
	if (tls.is_valid()) {
		tls->disconnect_from_stream();
	}
	tcp->disconnect_from_host();
	complete = true;
	success = p_success;
	emit_signal("completed");
}

void HTTPRequest2TCP::_build_request(const RequestData &p_data, Ref<StreamPeerBuffer> r_output) {
	String request = String(methods[p_data.method]) + " " + p_data.request_string + " HTTP/1.1\r\n";
	bool add_host = true;
	bool add_clen = p_data.body.size() > 0;
	bool add_uagent = true;
	bool add_accept = true;
	for (int i = 0; i < p_data.headers.size(); i++) {
		const String &h = p_data.headers[i];
		request += h + String("\r\n");
		if (add_host && h.findn("Host:") == 0) {
			add_host = false;
		}
		if (add_clen && h.findn("Content-Length:") == 0) {
			add_clen = false;
		}
		if (add_uagent && h.findn("User-Agent:") == 0) {
			add_uagent = false;
		}
		if (add_accept && h.findn("Accept:") == 0) {
			add_accept = false;
		}
	}
	if (add_host) {
		if ((p_data.scheme == "https://" && p_data.port == 443) || (p_data.scheme == "http://" && p_data.port == 80)) {
			// Don't append the standard ports.
			request += "Host: " + p_data.host + "\r\n";
		} else {
			request += "Host: " + p_data.host + ":" + itos(p_data.port) + "\r\n";
		}
	}
	if (add_clen) {
		request += "Content-Length: " + itos(p_data.body.size()) + "\r\n";
		// Should it add utf8 encoding?
	}
	if (add_uagent) {
		request += "User-Agent: GodotEngine/" + String(VERSION_FULL_BUILD) + " (" + OS::get_singleton()->get_name() + ")\r\n";
	}
	if (add_accept) {
		request += "Accept: */*\r\n";
	}
	request += "\r\n";
	r_output->put_data(request.to_utf8_buffer());
}

void HTTPRequest2TCP::_parse_chunk_header(const PackedByteArray &p_chunk) {
	int chunk_size = p_chunk.size();
	if (chunk_size == 0) {
		return;
	}
	uint8_t *ptrw = header_buffer.ptrw();
	memcpy(ptrw + header_buffer_pos, p_chunk.ptr(), chunk_size);
	for (int i = 0; i < chunk_size; i++, header_buffer_pos++) {
		ERR_FAIL_COND(header_buffer_pos >= header_buffer.size());
		if (header_buffer_pos < 3) {
			continue;
		}
		if (ptrw[header_buffer_pos - 1] == '\r' && ptrw[header_buffer_pos] == '\n') {
			chunk_left = String::utf8((char *)ptrw, header_buffer_pos - 1).split(";")[0].strip_edges().hex_to_int();
			header_buffer_pos = 0;
			if (chunk_left == 0) {
				completed(true);
				return;
			}
			_parse_chunk_body(p_chunk.slice(i + 1));
			break;
		}
	}
}

void HTTPRequest2TCP::_parse_chunk_body(const PackedByteArray &p_chunk) {
	int size = p_chunk.size();
	if (size == 0) {
		return;
	}
	if (size >= chunk_left) {
		response->put_data(p_chunk.slice(0, chunk_left));
		PackedByteArray left = p_chunk.slice(chunk_left);
		chunk_left = 0;
		_parse_chunk_header(left);
	} else {
		response->put_data(p_chunk);
		chunk_left -= size;
	}
}

void HTTPRequest2TCP::_parse_response_chunk(const PackedByteArray &p_chunk) {
	if (p_chunk.size() == 0) {
		return;
	}
	if (chunked) {
		if (chunk_left) {
			_parse_chunk_body(p_chunk);
		} else {
			_parse_chunk_header(p_chunk);
		}
	} else {
		response->put_data(p_chunk);
		if (body_size != -1) {
			body_left -= p_chunk.size();
			if (body_left == 0) {
				completed(true);
			} else if (body_left < 0) {
				completed(false); // Got too much data.
				ERR_PRINT("Received too much data, body_left: " + itos(body_left));
			}
		} else if (!read_until_eof) {
			completed(false);
			ERR_PRINT("Don't know how to parse non-chunked, keep-alive request without a content-length");
		}
	}
}

void HTTPRequest2TCP::_parse_header(const String &p_header) {
	String h = p_header.to_lower().strip_edges();
	if (h.begins_with("transfer-encoding:")) {
		String encoding = h.substr(h.find(":") + 1).strip_edges();
		for (const String &value : encoding.split(",")) {
			if (value.strip_edges() == "chunked") {
				chunked = true;
				break;
			}
		}
	} else if (h.begins_with("content-length:")) {
		body_size = h.substr(h.find(":") + 1).strip_edges().to_int();
		body_left = body_size;
	} else if (h.begins_with("connection:")) {
		if (h.substr(h.find(":") + 1).strip_edges() == "close") {
			read_until_eof = true;
		}
	}
}

void HTTPRequest2TCP::_poll() {
	if (complete) {
		return;
	}
	if (tcp->get_status() == StreamPeerTCP::STATUS_CONNECTING) {
		tcp->poll();
	}
	StreamPeerTCP::Status tcp_status = tcp->get_status();
	if (tcp_status != StreamPeerTCP::STATUS_CONNECTED) {
		if (tcp_status != StreamPeerTCP::STATUS_CONNECTING) {
			completed(false);
		}
		return;
	}
	if (tls.is_valid()) {
		if (tls->get_stream().is_null()) {
			tls->connect_to_stream(tcp, tls_hostname, tls_options);
			stream = tls;
		}
		if (tls->get_status() == StreamPeerTLS::STATUS_HANDSHAKING) {
			tls->poll();
		}
		StreamPeerTLS::Status tls_status = tls->get_status();
		if (tls_status != StreamPeerTLS::STATUS_CONNECTED) {
			if (tls_status != StreamPeerTLS::STATUS_HANDSHAKING) {
				completed(false);
			}
			return;
		}
	}
	if (request.is_valid()) {
		int pos = request->get_position();
		Array r = stream->put_partial_data(request->get_data_array().slice(pos, pos + (1 << 16 /* TODO customize? */)));
		Error err = (Error)r[0].operator int();
		int wrote = r[1].operator int();
		if (err != OK && err != ERR_BUSY) {
			completed(false);
			return;
		}
		request->seek(pos + wrote);
		if (request->get_available_bytes() == 0) {
			request.unref();
		}
	}
	if (request.is_valid()) {
		return;
	}
	// Receiving
	if (headers_over) {
		Array data = stream->get_partial_data(1 << 16 /* TODO customize? */);
		ERR_FAIL_COND(data.size() != 2); // BUG!
		Error err = (Error)data[0].operator int();
		PackedByteArray pba = data[1].operator PackedByteArray();
		if (err != OK && err != ERR_BUSY) {
			completed(read_until_eof && err == ERR_FILE_EOF); // TODO seems broken.
			return;
		}
		_parse_response_chunk(pba);
		// TODO Handle response over.
		return;
	} else {
		int to_read = header_buffer.size() - header_buffer_pos;
		if (to_read < 1) {
			completed(false); // Header too big.
			return;
		}
		Array data = stream->get_partial_data(to_read);
		ERR_FAIL_COND(data.size() != 2); // BUG!
		Error err = (Error)data[0].operator int();
		PackedByteArray pba = data[1].operator PackedByteArray();
		if (err != OK && err != ERR_BUSY) {
			completed(false);
			return;
		}
		int chunk_size = pba.size();
		if (chunk_size == 0) {
			return;
		}
		uint8_t *ptrw = header_buffer.ptrw();
		memcpy(ptrw + header_buffer_pos, pba.ptr(), chunk_size);
		for (int i = 0; i < chunk_size; i++, header_buffer_pos++) {
			if (header_buffer_pos < 2) {
				continue;
			}
			if (ptrw[header_buffer_pos - 1] == '\r' && ptrw[header_buffer_pos] == '\n') {
				// Store current
				String h = String::utf8((char *)ptrw, header_buffer_pos).strip_edges();
				if (h.is_empty()) {
					headers_over = true;
					header_buffer_pos = 0;
					// Add remaining data.
					_parse_response_chunk(pba.slice(i + 1));
					break;
				}
				if (headers.size() == 0) {
					response_code = h.split(" ")[1].to_int();
				} else {
					_parse_header(h);
				}
				headers.push_back(h);
				// Reset
				memcpy(ptrw, pba.ptr() + i, chunk_size - i);
				header_buffer_pos = 0;
			}
		}
	}
}

} //namespace godot
