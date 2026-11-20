// Array helpers. Inputs are expected to be arrays unless a function says otherwise.
fn contains(values, needle) { "Return true if an array contains a value equal to needle."; let i = 0; while (i < len(values)) { if (values[i] == needle) { return true; } i++; } return false; }
fn sum(values) { "Return the sum of a numeric array; empty arrays sum to zero."; let total = 0; let i = 0; while (i < len(values)) { total += values[i]; i++; } return total; }
fn map_values(values, transform) { "Return a new array with transform applied to every value."; let result = []; let i = 0; while (i < len(values)) { push(result, transform(values[i])); i++; } return result; }
fn filter_values(values, predicate) { "Return a new array containing values for which predicate returns true."; let result = []; let i = 0; while (i < len(values)) { if (predicate(values[i])) { push(result, values[i]); } i++; } return result; }
fn reverse(values) { "Return a reversed shallow copy of an array."; let result = []; let i = len(values) - 1; while (i >= 0) { push(result, values[i]); i--; } return result; }
fn first(values) { "Return the first element; throw if the array is empty."; if (len(values) == 0) { throw "first: array is empty"; } return values[0]; }
fn last(values) { "Return the last element; throw if the array is empty."; if (len(values) == 0) { throw "last: array is empty"; } return values[len(values) - 1]; }
fn count(values, needle) { "Count elements equal to needle."; let total = 0; let i = 0; while (i < len(values)) { if (values[i] == needle) { total++; } i++; } return total; }
fn unique(values) { "Return a stable shallow copy with duplicate values removed."; let result = []; let i = 0; while (i < len(values)) { if (!contains(result, values[i])) { push(result, values[i]); } i++; } return result; }
fn flatten_one(values) { "Flatten one level of nested arrays; non-array elements are preserved."; let result = []; let i = 0; while (i < len(values)) { if (type_of(values[i]) == "array") { let j = 0; while (j < len(values[i])) { push(result, values[i][j]); j++; } } else { push(result, values[i]); } i++; } return result; }
fn zip(left, right) { "Return an array of two-element arrays, stopping at the shorter input."; let result = []; let n = len(left); if (len(right) < n) { n = len(right); } let i = 0; while (i < n) { push(result, [left[i], right[i]]); i++; } return result; }
fn enumerate(values) { "Return an array of [index, value] pairs."; let result = []; let i = 0; while (i < len(values)) { push(result, [i, values[i]]); i++; } return result; }
fn reduce(values, reducer, initial) { "Fold values from left to right, calling reducer(accumulator, value)."; let acc = initial; let i = 0; while (i < len(values)) { acc = reducer(acc, values[i]); i++; } return acc; }
