// Pseudorandom helpers. Do not use the runtime PRNG for cryptography or security tokens.
fn seed(value) { "Seed the runtime pseudo-random generator."; return seed_random(value); }
fn number() { "Return one pseudo-random number using the runtime's random primitive."; return random(); }
fn integer(low, high) { "Return an integer in the inclusive interval [low, high]."; if (low != floor(low) || high != floor(high) || low > high) { throw "integer: expected integer low <= high"; } return random(low, high); }
fn choice(values) { "Return a random array element; throw if values is empty."; if (len(values) == 0) { throw "choice: array is empty"; } return values[integer(0, len(values) - 1)]; }
fn shuffled(values) { "Return a Fisher-Yates shuffled shallow copy of values."; let result = []; let i = 0; while (i < len(values)) { push(result, values[i]); i++; } i = len(result) - 1; while (i > 0) { let j = integer(0, i); let tmp = result[i]; result[i] = result[j]; result[j] = tmp; i--; } return result; }
