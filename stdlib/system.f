// Process environment and runtime timing wrappers.
fn get_env(name, default_value) { "Read an environment variable and return default_value when unset."; let value = env(name); if (value == null) { return default_value; } return value; }
fn set_environment(name, value) { "Set an environment variable for this process where supported."; return set_env(name, value); }
fn sleep(milliseconds) { "Sleep for approximately milliseconds."; return sleep_ms(milliseconds); }
fn clock() { "Return the runtime clock reading in milliseconds."; return clock_ms(); }
