// Small validation helpers. These are convenience checks, not security validators.
fn is_integer(value) { "Return true when value is numeric and integral."; return type_of(value) == "number" && value == floor(value); }
fn in_range(value, low, high) { "Return true when low <= value <= high."; return value >= low && value <= high; }
fn is_non_empty_string(value) { "Return true for non-empty strings."; return type_of(value) == "string" && len(value) > 0; }
fn require(condition, message) { "Throw message unless condition is truthy."; if (!condition) { throw message; } return true; }
