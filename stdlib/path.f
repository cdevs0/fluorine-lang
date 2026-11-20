// Filesystem path helpers. Paths are interpreted by the host operating system.
fn join(first, second) { "Join two path components."; return path_join(first, second); }
fn file_name(path) { "Return the final component of path."; return basename(path); }
fn directory_name(path) { "Return the directory portion of path."; return dirname(path); }
