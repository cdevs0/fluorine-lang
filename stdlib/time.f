// Wall-clock and timing wrappers. Precision and scheduling depend on the host OS.
fn now() { "Return Unix wall-clock time in seconds."; return unix_time(); }
fn formatted_now() { "Return the runtime's formatted local wall-clock time."; return time_string(); }
fn monotonic_milliseconds() { "Return a runtime clock reading in milliseconds; use for elapsed durations, not timestamps."; return clock_ms(); }
fn sleep(milliseconds) { "Suspend the current thread for approximately milliseconds."; return sleep_ms(milliseconds); }
