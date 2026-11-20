// String utilities. The runtime currently indexes strings by bytes, not Unicode code points.
fn starts_with(value, prefix) {
    "Return true when value begins with prefix.";
    if (len(prefix) > len(value)) { return false; }
    let i = 0;
    while (i < len(prefix)) { if (value[i] != prefix[i]) { return false; } i++; }
    return true;
}
fn ends_with(value, suffix) {
    "Return true when value ends with suffix.";
    if (len(suffix) > len(value)) { return false; }
    let offset = len(value) - len(suffix); let i = 0;
    while (i < len(suffix)) { if (value[offset + i] != suffix[i]) { return false; } i++; }
    return true;
}
fn reverse(value) {
    "Return a byte-reversed copy of value; multibyte UTF-8 characters may be split.";
    let result = ""; let i = len(value) - 1;
    while (i >= 0) { result += value[i]; i--; }
    return result;
}
fn repeat(value, count) {
    "Repeat value count times; count must be a non-negative integer.";
    if (count < 0 || count != floor(count)) { throw "repeat: count must be a non-negative integer"; }
    let result = ""; let i = 0;
    while (i < count) { result += value; i++; }
    return result;
}
fn capitalize(value) { "Uppercase the first byte and preserve the remainder."; if (len(value) == 0) { return value; } return upper(value[0]) + substring(value, 1); }
fn is_empty(value) { "Return true when value is the empty string."; return len(value) == 0; }
fn pad_left(value, width, fill) {
    "Pad value on the left to width bytes using a one-byte fill string.";
    if (len(fill) != 1) { throw "pad_left: fill must be exactly one byte"; }
    if (width < 0 || width != floor(width)) { throw "pad_left: width must be a non-negative integer"; }
    while (len(value) < width) { value = fill + value; }
    return value;
}
fn pad_right(value, width, fill) {
    "Pad value on the right to width bytes using a one-byte fill string.";
    if (len(fill) != 1) { throw "pad_right: fill must be exactly one byte"; }
    if (width < 0 || width != floor(width)) { throw "pad_right: width must be a non-negative integer"; }
    while (len(value) < width) { value += fill; }
    return value;
}
fn count(value, needle) {
    "Count non-overlapping occurrences of a non-empty needle.";
    if (len(needle) == 0) { throw "count: needle must not be empty"; }
    let found = 0; let i = 0;
    while (i + len(needle) <= len(value)) {
        if (substring(value, i, len(needle)) == needle) { found++; i += len(needle); }
        else { i++; }
    }
    return found;
}
