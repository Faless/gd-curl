/**************************************************************************/
/*  tls_options.cpp                                                       */
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

// THIS FILE IS GENERATED. EDITS WILL BE LOST.

#include "tls_options.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/engine_ptrcall.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace godot {

Ref<TLSOptionsCompat> TLSOptionsCompat::client(const Ref<X509Certificate> &trusted_chain, const String &common_name_override) {
	Ref<TLSOptionsCompat> ref(memnew(TLSOptionsCompat));
	ref->trusted_ca_chain = trusted_chain;
	ref->common_name = common_name_override;
	return ref;
}

Ref<TLSOptionsCompat> TLSOptionsCompat::client_unsafe(const Ref<X509Certificate> &trusted_chain) {
	Ref<TLSOptionsCompat> ref(memnew(TLSOptionsCompat));
	ref->trusted_ca_chain = trusted_chain;
	ref->unsafe_client = true;
	return ref;
}

Ref<TLSOptionsCompat> TLSOptionsCompat::server(const Ref<CryptoKey> &key, const Ref<X509Certificate> &certificate) {
	Ref<TLSOptionsCompat> ref(memnew(TLSOptionsCompat));
	ref->server_mode = true;
	ref->own_certificate = certificate;
	ref->private_key = key;
	return ref;
}

bool TLSOptionsCompat::is_server() const {
	return server_mode;
}

bool TLSOptionsCompat::is_unsafe_client() const {
	return !server_mode && unsafe_client;
}

String TLSOptionsCompat::get_common_name() const {
	return common_name;
}

Ref<X509Certificate> TLSOptionsCompat::get_trusted_ca_chain() const {
	return trusted_ca_chain;
}

Ref<CryptoKey> TLSOptionsCompat::get_private_key() const {
	return private_key;
}

Ref<CryptoKey> TLSOptionsCompat::get_own_certificate() const {
	return own_certificate;
}

} // namespace godot
