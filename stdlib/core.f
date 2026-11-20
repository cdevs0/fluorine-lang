// Core public API. C-backed operations are explicit __native_* primitives.
fn is_number(value) { return type_of(value) == "number"; }
fn len(value) { return __native_len(value); }
fn push(array, value) { return __native_push(array, value); }
fn pop(array) { return __native_pop(array); }
fn keys(value) { return __native_keys(value); }
fn has_key(value, key) { return __native_has_key(value, key); }
fn gc() { return __native_gc(); }
