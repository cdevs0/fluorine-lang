// Path and URL helpers.
fn basename(path) { return __native_basename(path); }
fn dirname(path) { return __native_dirname(path); }
fn url_encode(value) { return __native_url_encode(value); }
fn url_decode(value) { return __native_url_decode(value); }
// path_join() supports a variable number of path components and remains native.
