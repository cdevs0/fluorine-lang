// Mathematical helpers. Numeric primitives are implemented in the C runtime.
fn square(x) { "Return x multiplied by itself."; return x * x; }
fn cube(x) { "Return x raised to the third power."; return x * x * x; }
fn clamp(x, low, high) {
    "Clamp x to the inclusive interval [low, high].";
    if (low > high) { throw "clamp: low must not exceed high"; }
    if (x < low) { return low; }
    if (x > high) { return high; }
    return x;
}
fn sign(x) { "Return -1, 0, or 1 according to the sign of x."; if (x < 0) { return -1; } if (x > 0) { return 1; } return 0; }
fn is_even(x) { "Return true if x is an even integer."; return x == floor(x) && x % 2 == 0; }
fn is_odd(x) { "Return true if x is an odd integer."; return x == floor(x) && abs(x % 2) == 1; }
fn factorial(n) {
    "Return n factorial; n must be an integer from 0 through 170.";
    if (n < 0 || n != floor(n) || n > 170) { throw "factorial: expected integer n in [0, 170]"; }
    let result = 1;
    let i = 2;
    while (i <= n) { result *= i; i++; }
    return result;
}
fn gcd(a, b) {
    "Return the greatest common divisor of integer inputs.";
    if (a != floor(a) || b != floor(b)) { throw "gcd: arguments must be integers"; }
    a = abs(a); b = abs(b);
    while (b != 0) { let r = a % b; a = b; b = r; }
    return a;
}
fn lcm(a, b) { "Return the least common multiple of integer inputs; zero if either input is zero."; if (a == 0 || b == 0) { return 0; } return abs((a / gcd(a, b)) * b); }
fn lerp(a, b, t) { "Linearly interpolate from a to b using t (normally between 0 and 1)."; return a + (b - a) * t; }
fn map_range(x, in_low, in_high, out_low, out_high) {
    "Map x from one numeric interval to another; input interval must be non-empty.";
    if (in_low == in_high) { throw "map_range: input interval must have nonzero width"; }
    return out_low + (x - in_low) * (out_high - out_low) / (in_high - in_low);
}
fn average(values) {
    "Return the arithmetic mean of a non-empty array of numbers.";
    if (len(values) == 0) { throw "average: values must not be empty"; }
    let total = 0; let i = 0;
    while (i < len(values)) { total += values[i]; i++; }
    return total / len(values);
}
