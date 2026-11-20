// Text utilities layered on runtime primitives.
fn strip(value) { "Trim leading and trailing whitespace."; return trim(value); }
fn uppercase(value) { "Return uppercase text."; return upper(value); }
fn lowercase(value) { "Return lowercase text."; return lower(value); }
fn replace_all(value, old_value, new_value) { "Replace occurrences of old_value with new_value."; return replace(value, old_value, new_value); }
fn chars(value) { "Return an array of one-byte strings; this is not Unicode-code-point aware."; return split(value, ""); }
fn words(value) { "Split whitespace-trimmed text on a literal single space."; return split(trim(value), " "); }
