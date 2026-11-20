// Functional programming helpers.
fn identity(value) { "Return value unchanged."; return value; }
fn constant(value) { "Return a function that always returns value."; fn constant_inner(unused) { return value; } return constant_inner; }
fn compose(first, second) { "Return a function equivalent to first(second(value))."; fn composed(value) { return first(second(value)); } return composed; }
fn negate(predicate) { "Return a predicate whose result is the logical negation of predicate(value)."; fn negated(value) { return !predicate(value); } return negated; }
fn once(function_value) { "Return a wrapper that invokes the function at most once and caches its first result."; let called = false; let result = null; fn invoke_once(value) { if (!called) { result = function_value(value); called = true; } return result; } return invoke_once; }
