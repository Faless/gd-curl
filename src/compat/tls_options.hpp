/**************************************************************************/
/*  tls_options.hpp                                                       */
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

#ifndef GODOT_CPP_TLS_OPTIONS_COMPAT_HPP
#define GODOT_CPP_TLS_OPTIONS_COMPAT_HPP

#include <godot_cpp/classes/crypto_key.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/x509_certificate.hpp>
#include <godot_cpp/variant/string.hpp>

#include <godot_cpp/core/class_db.hpp>

namespace godot {

class TLSOptionsCompat : public RefCounted {
	GDCLASS(TLSOptionsCompat, RefCounted);

private:
	Ref<X509Certificate> trusted_ca_chain;
	Ref<X509Certificate> own_certificate;
	Ref<CryptoKey> private_key;
	String common_name;
	bool unsafe_client = false;
	bool server_mode = false;

protected:
	static void _bind_methods() {
		ClassDB::bind_static_method("TLSOptionsCompat", D_METHOD("client", "trusted_chain", "common_name_override"), &TLSOptionsCompat::client, DEFVAL(Ref<X509Certificate>()), DEFVAL(String()));
		ClassDB::bind_static_method("TLSOptionsCompat", D_METHOD("client_unsafe", "trusted_chain"), &TLSOptionsCompat::client_unsafe, DEFVAL(Ref<X509Certificate>()));
		ClassDB::bind_static_method("TLSOptionsCompat", D_METHOD("server", "key", "certificate"), &TLSOptionsCompat::server);

		ClassDB::bind_method(D_METHOD("is_server"), &TLSOptionsCompat::is_server);
		ClassDB::bind_method(D_METHOD("is_unsafe_client"), &TLSOptionsCompat::is_unsafe_client);
		ClassDB::bind_method(D_METHOD("get_common_name"), &TLSOptionsCompat::get_common_name);
		ClassDB::bind_method(D_METHOD("get_trusted_ca_chain"), &TLSOptionsCompat::get_trusted_ca_chain);
		ClassDB::bind_method(D_METHOD("get_private_key"), &TLSOptionsCompat::get_private_key);
		ClassDB::bind_method(D_METHOD("get_own_certificate"), &TLSOptionsCompat::get_private_key);
	}

public:
	static Ref<TLSOptionsCompat> client(const Ref<X509Certificate> &trusted_chain = nullptr, const String &common_name_override = String());
	static Ref<TLSOptionsCompat> client_unsafe(const Ref<X509Certificate> &trusted_chain = nullptr);
	static Ref<TLSOptionsCompat> server(const Ref<CryptoKey> &key, const Ref<X509Certificate> &certificate);
	bool is_server() const;
	bool is_unsafe_client() const;
	String get_common_name() const;
	Ref<X509Certificate> get_trusted_ca_chain() const;
	Ref<CryptoKey> get_private_key() const;
	Ref<CryptoKey> get_own_certificate() const;
};

} // namespace godot

#endif // ! GODOT_CPP_TLS_OPTIONS_HPP
