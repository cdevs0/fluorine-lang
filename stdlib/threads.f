// Native OS thread handles are managed by the interpreter runtime.
fn thread_join(handle) { return __native_thread_join(handle); }
fn thread_is_done(handle) { return __native_thread_is_done(handle); }
fn thread_count() { return __native_thread_count(); }
// thread_start() accepts a variable number of worker arguments and remains a host primitive.
