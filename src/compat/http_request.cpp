/**************************************************************************/
/*  http_request.cpp                                                      */
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

#include "http_request.hpp"
#include "compat.hpp"

#include "godot_cpp/classes/http_client.hpp"
#include "godot_cpp/classes/http_request.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/timer.hpp"

namespace godot {

Error HTTPRequestCompat::_request() {
	return client->connect_to_host(url, port, use_tls ? tls_options : nullptr);
}

Error HTTPRequestCompat::_parse_url(const String &p_url) {
	use_tls = false;
	request_string = "";
	port = 80;
	request_sent = false;
	got_response = false;
	body_len = -1;
	body.clear();
	downloaded.set(0);
	final_body_size.set(0);
	redirections = 0;

	String scheme;
	Error err = godot_parse_url(p_url, scheme, url, port, request_string);
	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Error parsing URL: '%s'.", p_url));

	if (scheme == "https://") {
		use_tls = true;
	} else if (scheme != "http://") {
		ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER, vformat("Invalid URL scheme: '%s'.", scheme));
	}

	if (port == 0) {
		port = use_tls ? 443 : 80;
	}
	if (request_string.is_empty()) {
		request_string = "/";
	}
	return OK;
}

bool HTTPRequestCompat::has_header(const PackedStringArray &p_headers, const String &p_header_name) {
	bool exists = false;

	String lower_case_header_name = p_header_name.to_lower();
	for (int i = 0; i < p_headers.size() && !exists; i++) {
		String sanitized = p_headers[i].strip_edges().to_lower();
		if (sanitized.begins_with(lower_case_header_name)) {
			exists = true;
		}
	}

	return exists;
}

String HTTPRequestCompat::get_header_value(const PackedStringArray &p_headers, const String &p_header_name) {
	String value = "";

	String lowwer_case_header_name = p_header_name.to_lower();
	for (int i = 0; i < p_headers.size(); i++) {
		if (p_headers[i].find(":") > 0) {
			PackedStringArray parts = p_headers[i].split(":", false, 1);
			if (parts.size() > 1 && parts[0].strip_edges().to_lower() == lowwer_case_header_name) {
				value = parts[1].strip_edges();
				break;
			}
		}
	}

	return value;
}

Error HTTPRequestCompat::request(const String &p_url, PackedStringArray p_custom_headers, HTTPClient::Method p_method, const String &p_request_data) {
	// Copy the string into a raw buffer.
	PackedByteArray raw_data;

	CharString charstr = p_request_data.utf8();
	size_t len = charstr.length();
	if (len > 0) {
		raw_data.resize(len);
		uint8_t *w = raw_data.ptrw();
		memcpy(w, charstr.ptr(), len);
	}

	return request_raw(p_url, p_custom_headers, p_method, raw_data);
}

Error HTTPRequestCompat::request_raw(const String &p_url, PackedStringArray p_custom_headers, HTTPClient::Method p_method, PackedByteArray p_request_data_raw) {
	ERR_FAIL_COND_V(!is_inside_tree(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(requesting, ERR_BUSY, "HTTPRequestCompat is processing a request. Wait for completion or cancel it before attempting a new one.");

	if (timeout > 0) {
		timer->stop();
		timer->start(timeout);
	}

	method = p_method;

	Error err = _parse_url(p_url);
	if (err) {
		return err;
	}

	headers = p_custom_headers;

	if (accept_gzip) {
		// If the user has specified an Accept-Encoding header, don't overwrite it.
		if (!has_header(headers, "Accept-Encoding")) {
			headers.push_back("Accept-Encoding: gzip, deflate");
		}
	}

	request_data = p_request_data_raw;

	requesting = true;

	if (use_threads.is_set()) {
		thread_done.clear();
		thread_request_quit.clear();
		client->set_blocking_mode(true);
		thread->start(callable_mp_static(_thread_func).bind(this));
	} else {
		client->set_blocking_mode(false);
		err = _request();
		if (err != OK) {
			_defer_done(HTTPRequest::RESULT_CANT_CONNECT, 0, PackedStringArray(), PackedByteArray());
			return ERR_CANT_CONNECT;
		}

		set_process_internal(true);
	}

	return OK;
}

void HTTPRequestCompat::_thread_func(HTTPRequestCompat *p_userdata) {
	HTTPRequestCompat *hr = p_userdata;

	Error err = hr->_request();

	if (err != OK) {
		hr->_defer_done(HTTPRequest::RESULT_CANT_CONNECT, 0, PackedStringArray(), PackedByteArray());
	} else {
		while (!hr->thread_request_quit.is_set()) {
			bool exit = hr->_update_connection();
			if (exit) {
				break;
			}
			OS::get_singleton()->delay_usec(1);
		}
	}

	hr->thread_done.set();
}

void HTTPRequestCompat::cancel_request() {
	if (!requesting) {
		return;
	}

	timer->stop();

	if (!use_threads.is_set()) {
		set_process_internal(false);
	} else {
		thread_request_quit.set();
		if (thread->is_started()) {
			thread->wait_to_finish();
		}
	}

	file.unref();
	decompressor.unref();
	client->close();
	body.clear();
	got_response = false;
	response_code = -1;
	request_sent = false;
	requesting = false;
}

bool HTTPRequestCompat::_handle_response(bool *ret_value) {
	if (!client->has_response()) {
		_defer_done(HTTPRequest::RESULT_NO_RESPONSE, 0, PackedStringArray(), PackedByteArray());
		*ret_value = true;
		return true;
	}

	got_response = true;
	response_code = client->get_response_code();
	response_headers.clear();
	response_headers = client->get_response_headers();
	downloaded.set(0);
	final_body_size.set(0);
	decompressor.unref();

	if (response_code == 301 || response_code == 302) {
		// Handle redirect.

		if (max_redirects >= 0 && redirections >= max_redirects) {
			_defer_done(HTTPRequest::RESULT_REDIRECT_LIMIT_REACHED, response_code, response_headers, PackedByteArray());
			*ret_value = true;
			return true;
		}

		String new_request;

		for (const String &E : response_headers) {
			if (E.containsn("Location: ")) {
				new_request = E.substr(9, E.length()).strip_edges();
			}
		}

		if (!new_request.is_empty()) {
			// Process redirect.
			client->close();
			int new_redirs = redirections + 1; // Because _request() will clear it.
			Error err;
			if (new_request.begins_with("http")) {
				// New url, new request.
				_parse_url(new_request);
			} else {
				request_string = new_request;
			}

			err = _request();
			if (err == OK) {
				request_sent = false;
				got_response = false;
				body_len = -1;
				body.clear();
				downloaded.set(0);
				final_body_size.set(0);
				redirections = new_redirs;
				*ret_value = false;
				return true;
			}
		}
	}

	// Check if we need to start streaming decompression.
	String content_encoding;
	if (accept_gzip) {
		content_encoding = get_header_value(response_headers, "Content-Encoding").to_lower();
	}
	if (content_encoding == "gzip") {
		decompressor.instantiate();
		decompressor->start_decompression(false, get_download_chunk_size());
	} else if (content_encoding == "deflate") {
		decompressor.instantiate();
		decompressor->start_decompression(true, get_download_chunk_size());
	}

	return false;
}

bool HTTPRequestCompat::_update_connection() {
	switch (client->get_status()) {
		case HTTPClient::STATUS_DISCONNECTED: {
			_defer_done(HTTPRequest::RESULT_CANT_CONNECT, 0, PackedStringArray(), PackedByteArray());
			return true; // End it, since it's disconnected.
		} break;
		case HTTPClient::STATUS_RESOLVING: {
			client->poll();
			// Must wait.
			return false;
		} break;
		case HTTPClient::STATUS_CANT_RESOLVE: {
			_defer_done(HTTPRequest::RESULT_CANT_RESOLVE, 0, PackedStringArray(), PackedByteArray());
			return true;

		} break;
		case HTTPClient::STATUS_CONNECTING: {
			client->poll();
			// Must wait.
			return false;
		} break; // Connecting to IP.
		case HTTPClient::STATUS_CANT_CONNECT: {
			_defer_done(HTTPRequest::RESULT_CANT_CONNECT, 0, PackedStringArray(), PackedByteArray());
			return true;

		} break;
		case HTTPClient::STATUS_CONNECTED: {
			if (request_sent) {
				if (!got_response) {
					// No body.

					bool ret_value;

					if (_handle_response(&ret_value)) {
						return ret_value;
					}

					_defer_done(HTTPRequest::RESULT_SUCCESS, response_code, response_headers, PackedByteArray());
					return true;
				}
				if (body_len < 0) {
					// Chunked transfer is done.
					_defer_done(HTTPRequest::RESULT_SUCCESS, response_code, response_headers, body);
					return true;
				}

				_defer_done(HTTPRequest::RESULT_CHUNKED_BODY_SIZE_MISMATCH, response_code, response_headers, PackedByteArray());
				return true;
				// Request might have been done.
			} else {
				// Did not request yet, do request.

				int size = request_data.size();
				Error err = client->request(method, request_string, headers, size > 0 ? request_data.ptr() : nullptr, size);
				if (err != OK) {
					_defer_done(HTTPRequest::RESULT_CONNECTION_ERROR, 0, PackedStringArray(), PackedByteArray());
					return true;
				}

				request_sent = true;
				return false;
			}
		} break; // Connected: break requests only accepted here.
		case HTTPClient::STATUS_REQUESTING: {
			// Must wait, still requesting.
			client->poll();
			return false;

		} break; // Request in progress.
		case HTTPClient::STATUS_BODY: {
			if (!got_response) {
				bool ret_value;

				if (_handle_response(&ret_value)) {
					return ret_value;
				}

				if (!client->is_response_chunked() && client->get_response_body_length() == 0) {
					_defer_done(HTTPRequest::RESULT_SUCCESS, response_code, response_headers, PackedByteArray());
					return true;
				}

				// No body len (-1) if chunked or no content-length header was provided.
				// Change your webserver configuration if you want body len.
				body_len = client->get_response_body_length();

				if (body_size_limit >= 0 && body_len > body_size_limit) {
					_defer_done(HTTPRequest::RESULT_BODY_SIZE_LIMIT_EXCEEDED, response_code, response_headers, PackedByteArray());
					return true;
				}

				if (!download_to_file.is_empty()) {
					file = FileAccess::open(download_to_file, FileAccess::WRITE);
					if (file.is_null()) {
						_defer_done(HTTPRequest::RESULT_DOWNLOAD_FILE_CANT_OPEN, response_code, response_headers, PackedByteArray());
						return true;
					}
				}
			}

			client->poll();
			if (client->get_status() != HTTPClient::STATUS_BODY) {
				return false;
			}

			PackedByteArray chunk;
			if (decompressor.is_null()) {
				// Chunk can be read directly.
				chunk = client->read_response_body_chunk();
				downloaded.add(chunk.size());
			} else {
				// Chunk is the result of decompression.
				PackedByteArray compressed = client->read_response_body_chunk();
				downloaded.add(compressed.size());

				while (compressed.size()) {
					Array res = decompressor->put_partial_data(compressed);
					Error err = (Error)res[0].operator int();
					int wrote = res[1].operator int();
					if (err == OK) {
						res = decompressor->get_data(decompressor->get_available_bytes());
						err = (Error)res[0].operator int();
						if (err == OK) {
							chunk.append_array(res[1].operator PackedByteArray());
						}
					}
					if (err != OK) {
						_defer_done(HTTPRequest::RESULT_BODY_DECOMPRESS_FAILED, response_code, response_headers, PackedByteArray());
						return true;
					}
					// We need this check here because a "zip bomb" could result in a chunk of few kilos decompressing into gigabytes of data.
					if (body_size_limit >= 0 && final_body_size.get() + chunk.size() > body_size_limit) {
						_defer_done(HTTPRequest::RESULT_BODY_SIZE_LIMIT_EXCEEDED, response_code, response_headers, PackedByteArray());
						return true;
					}
					compressed = compressed.slice(wrote);
				}
			}
			final_body_size.add(chunk.size());

			if (body_size_limit >= 0 && final_body_size.get() > body_size_limit) {
				_defer_done(HTTPRequest::RESULT_BODY_SIZE_LIMIT_EXCEEDED, response_code, response_headers, PackedByteArray());
				return true;
			}

			if (chunk.size()) {
				if (file.is_valid()) {
					const uint8_t *r = chunk.ptr();
					file->store_buffer(r, chunk.size());
					if (file->get_error() != OK) {
						_defer_done(HTTPRequest::RESULT_DOWNLOAD_FILE_WRITE_ERROR, response_code, response_headers, PackedByteArray());
						return true;
					}
				} else {
					body.append_array(chunk);
				}
			}

			if (body_len >= 0) {
				if (downloaded.get() == body_len) {
					_defer_done(HTTPRequest::RESULT_SUCCESS, response_code, response_headers, body);
					return true;
				}
			} else if (client->get_status() == HTTPClient::STATUS_DISCONNECTED) {
				// We read till EOF, with no errors. Request is done.
				_defer_done(HTTPRequest::RESULT_SUCCESS, response_code, response_headers, body);
				return true;
			}

			return false;

		} break; // Request resulted in body: break which must be read.
		case HTTPClient::STATUS_CONNECTION_ERROR: {
			_defer_done(HTTPRequest::RESULT_CONNECTION_ERROR, 0, PackedStringArray(), PackedByteArray());
			return true;
		} break;
		case HTTPClient::STATUS_TLS_HANDSHAKE_ERROR: {
			_defer_done(HTTPRequest::RESULT_TLS_HANDSHAKE_ERROR, 0, PackedStringArray(), PackedByteArray());
			return true;
		} break;
	}

	ERR_FAIL_V(false);
}

void HTTPRequestCompat::_defer_done(int p_status, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_data) {
	callable_mp(this, &HTTPRequestCompat::_request_done).call_deferred(p_status, p_code, p_headers, p_data);
}

void HTTPRequestCompat::_request_done(int p_status, int p_code, const PackedStringArray &p_headers, const PackedByteArray &p_data) {
	cancel_request();

	emit_signal("request_completed", p_status, p_code, p_headers, p_data);
}

void HTTPRequestCompat::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (use_threads.is_set()) {
				return;
			}
			bool done = _update_connection();
			if (done) {
				set_process_internal(false);
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (requesting) {
				cancel_request();
			}
		} break;
	}
}

void HTTPRequestCompat::set_use_threads(bool p_use) {
	ERR_FAIL_COND(get_http_client_status() != HTTPClient::STATUS_DISCONNECTED);
#ifdef THREADS_ENABLED
	use_threads.set_to(p_use);
#endif
}

bool HTTPRequestCompat::is_using_threads() const {
	return use_threads.is_set();
}

void HTTPRequestCompat::set_accept_gzip(bool p_gzip) {
	accept_gzip = p_gzip;
}

bool HTTPRequestCompat::is_accepting_gzip() const {
	return accept_gzip;
}

void HTTPRequestCompat::set_body_size_limit(int p_bytes) {
	ERR_FAIL_COND(get_http_client_status() != HTTPClient::STATUS_DISCONNECTED);

	body_size_limit = p_bytes;
}

int HTTPRequestCompat::get_body_size_limit() const {
	return body_size_limit;
}

void HTTPRequestCompat::set_download_file(const String &p_file) {
	ERR_FAIL_COND(get_http_client_status() != HTTPClient::STATUS_DISCONNECTED);

	download_to_file = p_file;
}

String HTTPRequestCompat::get_download_file() const {
	return download_to_file;
}

void HTTPRequestCompat::set_download_chunk_size(int p_chunk_size) {
	ERR_FAIL_COND(get_http_client_status() != HTTPClient::STATUS_DISCONNECTED);

	client->set_read_chunk_size(p_chunk_size);
}

int HTTPRequestCompat::get_download_chunk_size() const {
	return client->get_read_chunk_size();
}

HTTPClient::Status HTTPRequestCompat::get_http_client_status() const {
	return client->get_status();
}

void HTTPRequestCompat::set_max_redirects(int p_max) {
	max_redirects = p_max;
}

int HTTPRequestCompat::get_max_redirects() const {
	return max_redirects;
}

int HTTPRequestCompat::get_downloaded_bytes() const {
	return downloaded.get();
}

int HTTPRequestCompat::get_body_size() const {
	return body_len;
}

void HTTPRequestCompat::set_http_proxy(const String &p_host, int p_port) {
	client->set_http_proxy(p_host, p_port);
}

void HTTPRequestCompat::set_https_proxy(const String &p_host, int p_port) {
	client->set_https_proxy(p_host, p_port);
}

void HTTPRequestCompat::set_timeout(double p_timeout) {
	ERR_FAIL_COND(p_timeout < 0);
	timeout = p_timeout;
}

double HTTPRequestCompat::get_timeout() {
	return timeout;
}

void HTTPRequestCompat::_timeout() {
	cancel_request();
	_defer_done(HTTPRequest::RESULT_TIMEOUT, 0, PackedStringArray(), PackedByteArray());
}

void HTTPRequestCompat::set_tls_options(const Ref<TLSOptionsCompat> &p_options) {
	ERR_FAIL_COND(p_options.is_null() || p_options->is_server());
	tls_options = p_options;
}

void HTTPRequestCompat::_bind_methods() {
	ClassDB::bind_method(D_METHOD("request", "url", "custom_headers", "method", "request_data"), &HTTPRequestCompat::request, DEFVAL(PackedStringArray()), DEFVAL(HTTPClient::METHOD_GET), DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("request_raw", "url", "custom_headers", "method", "request_data_raw"), &HTTPRequestCompat::request_raw, DEFVAL(PackedStringArray()), DEFVAL(HTTPClient::METHOD_GET), DEFVAL(PackedByteArray()));
	ClassDB::bind_method(D_METHOD("cancel_request"), &HTTPRequestCompat::cancel_request);
	ClassDB::bind_method(D_METHOD("set_tls_options", "client_options"), &HTTPRequestCompat::set_tls_options);

	ClassDB::bind_method(D_METHOD("get_http_client_status"), &HTTPRequestCompat::get_http_client_status);

	ClassDB::bind_method(D_METHOD("set_use_threads", "enable"), &HTTPRequestCompat::set_use_threads);
	ClassDB::bind_method(D_METHOD("is_using_threads"), &HTTPRequestCompat::is_using_threads);

	ClassDB::bind_method(D_METHOD("set_accept_gzip", "enable"), &HTTPRequestCompat::set_accept_gzip);
	ClassDB::bind_method(D_METHOD("is_accepting_gzip"), &HTTPRequestCompat::is_accepting_gzip);

	ClassDB::bind_method(D_METHOD("set_body_size_limit", "bytes"), &HTTPRequestCompat::set_body_size_limit);
	ClassDB::bind_method(D_METHOD("get_body_size_limit"), &HTTPRequestCompat::get_body_size_limit);

	ClassDB::bind_method(D_METHOD("set_max_redirects", "amount"), &HTTPRequestCompat::set_max_redirects);
	ClassDB::bind_method(D_METHOD("get_max_redirects"), &HTTPRequestCompat::get_max_redirects);

	ClassDB::bind_method(D_METHOD("set_download_file", "path"), &HTTPRequestCompat::set_download_file);
	ClassDB::bind_method(D_METHOD("get_download_file"), &HTTPRequestCompat::get_download_file);

	ClassDB::bind_method(D_METHOD("get_downloaded_bytes"), &HTTPRequestCompat::get_downloaded_bytes);
	ClassDB::bind_method(D_METHOD("get_body_size"), &HTTPRequestCompat::get_body_size);

	ClassDB::bind_method(D_METHOD("set_timeout", "timeout"), &HTTPRequestCompat::set_timeout);
	ClassDB::bind_method(D_METHOD("get_timeout"), &HTTPRequestCompat::get_timeout);

	ClassDB::bind_method(D_METHOD("set_download_chunk_size", "chunk_size"), &HTTPRequestCompat::set_download_chunk_size);
	ClassDB::bind_method(D_METHOD("get_download_chunk_size"), &HTTPRequestCompat::get_download_chunk_size);

	ClassDB::bind_method(D_METHOD("set_http_proxy", "host", "port"), &HTTPRequestCompat::set_http_proxy);
	ClassDB::bind_method(D_METHOD("set_https_proxy", "host", "port"), &HTTPRequestCompat::set_https_proxy);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "download_file", PROPERTY_HINT_FILE), "set_download_file", "get_download_file");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "download_chunk_size", PROPERTY_HINT_RANGE, "256,16777216,suffix:B"), "set_download_chunk_size", "get_download_chunk_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_threads"), "set_use_threads", "is_using_threads");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "accept_gzip"), "set_accept_gzip", "is_accepting_gzip");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "body_size_limit", PROPERTY_HINT_RANGE, "-1,2000000000,suffix:B"), "set_body_size_limit", "get_body_size_limit");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_redirects", PROPERTY_HINT_RANGE, "-1,64"), "set_max_redirects", "get_max_redirects");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "timeout", PROPERTY_HINT_RANGE, "0,3600,0.1,or_greater,suffix:s"), "set_timeout", "get_timeout");

	ADD_SIGNAL(MethodInfo("request_completed", PropertyInfo(Variant::INT, "result"), PropertyInfo(Variant::INT, "response_code"), PropertyInfo(Variant::PACKED_STRING_ARRAY, "headers"), PropertyInfo(Variant::PACKED_BYTE_ARRAY, "body")));
}

HTTPRequestCompat::HTTPRequestCompat() {
	client = Ref<HTTPClientCurl>(memnew(HTTPClientCurl));
	tls_options = TLSOptionsCompat::client();
	timer = memnew(Timer);
	timer->set_one_shot(true);
	timer->connect("timeout", callable_mp(this, &HTTPRequestCompat::_timeout));
	thread.instantiate();
	add_child(timer);
}

HTTPRequestCompat::~HTTPRequestCompat() {
	cancel_request();
}

} // namespace godot
