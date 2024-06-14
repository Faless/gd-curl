# gd-curl

A Godot HTTPClient-compatible implementation using libcurl.

This could be used as a replacement for the default Godot HTTPClient.

It requires an upstream PR to be able to seemlessly replace the default
implementation.

When building with `compat=yes` (default), the extension will instead
bind separate classes (HTTPClientExtensionCompat, HTTPClientCurl,
HTTPRequestCompat) which can be used as a drop-in replcement for
upstream' classes HTTPclient and HTTPRequest.

The extension also provides additional `HTTPClient2` and `HTTPRequest2`
classes as a work-in-progress proof-of-concept for a new, more modern
API, targeting modern HTTP standards (HTTP/2, HTTP/3), to be proposed
upstream.

# Building

Build editor/debug:

```
scons platform=desired_platform target=template_debug
```

Build release:

```
scons platform=desired_platform target=template_release
```

Build without the compatiblity mode (requires upstream patch, in
`misc/patches/godot.diff`, and custom API extension.json).

```
scons custom_api_file=/path/to/extension.json platform=desired_platform target=desired_target compat=no
```

# Usage

## HTTPRequest(Compat)

```gdscript
func _ready() -> void:
    var hreq = HTTPRequestCompat.new()  # Or a regular HTTPRequest is not using compat mode.
    add_child(hreq)
    hreq.request("https://www.google.com", [])#, HTTPClient.METHOD_HEAD)
    var res = await hreq.request_completed
    print((res[3] as PackedByteArray).get_string_from_utf8())
    printt(res[0], res[1], res[2])
```

## HTTPClient2

```gdscript
var client : HTTPClient2 = HTTPClient2Curl.new()

func _ready() -> void:
    var req = client.fetch("https://www.google.com/", HTTPClient.METHOD_GET, ["apikey: my_api_key"], [])
    req.completed.connect(func():
        print("completed")
        print(req.has_headers())
        print(req.get_headers())
        print(req.has_response())
        print(req.get_response().get_string_from_utf8())
    , CONNECT_ONE_SHOT)

func _process(_delta: float) -> void:
    if client == null:
        return
    client.poll()  # We should probably automate this by default.
```
