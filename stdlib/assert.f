// Assertion helpers. Failed assertions throw values that can be caught with try/catch.
fn that(condition, message) { "Throw message when condition is false; otherwise return true."; if (!condition) { throw message; } return true; }
fn equal(actual, expected, message) { "Assert equality using Unknown's == semantics."; if (actual != expected) { throw message; } return true; }
fn not_equal(actual, expected, message) { "Assert that two values are not equal."; if (actual == expected) { throw message; } return true; }
fn near(actual, expected, epsilon, message) { "Assert numeric values differ by no more than epsilon."; if (abs(actual - expected) > epsilon) { throw message; } return true; }
