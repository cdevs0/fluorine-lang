// Minimal test assertion helpers.
fn assert_true(value, message) { "Throw message unless value is truthy."; if (!value) { throw message; } return true; }
fn assert_equal(actual, expected, message) { "Throw message unless actual equals expected."; if (actual != expected) { throw message; } return true; }
fn assert_approx(actual, expected, epsilon, message) { "Throw message unless numeric values differ by at most epsilon."; if (abs(actual - expected) > epsilon) { throw message; } return true; }
