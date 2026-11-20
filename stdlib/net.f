// HTTP is implemented by the C socket layer. These helpers make the common modes explicit.
fn http_serve_forever(port, handler) {
    return __native_http_serve(port, handler);
}
fn http_serve_limited(port, handler, max_requests) {
    return __native_http_serve(port, handler, max_requests);
}
// The original http_serve(port, handler, [max_requests]) spelling stays available unchanged.
