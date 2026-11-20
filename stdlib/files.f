// Filesystem primitives. Paths are interpreted relative to the process working directory.
fn read_text(path) { return __native_read_text(path); }
fn write_text(path, contents) { return __native_write_text(path, contents); }
fn append_text(path, contents) { return __native_append_text(path, contents); }
fn file_exists(path) { return __native_file_exists(path); }
fn remove_file(path) { return __native_remove_file(path); }
