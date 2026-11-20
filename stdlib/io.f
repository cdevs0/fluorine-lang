// Text-file convenience wrappers. Paths are subject to process permissions.
fn exists(path) { "Return true if a file exists at path."; return file_exists(path); }
fn read(path) { "Read and return the entire text file."; return read_text(path); }
fn write(path, text) { "Replace/create a text file with text."; return write_text(path, text); }
fn append(path, text) { "Append text to a file."; return append_text(path, text); }
fn remove(path) { "Remove a file; errors depend on host permissions and filesystem state."; return remove_file(path); }
