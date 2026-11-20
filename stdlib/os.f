
 // os.un — Standard OS module for Unknown
 // Native implementations are provided by Unknown.c.

namespace os {
    // Operating system information
    fn name() { return os_name(); }
    fn version() { return os_version(); }
    fn architecture() { return os_architecture(); }
    fn hostname() { return os_hostname(); }
    fn username() { return os_username(); }
    fn is_admin() { return os_is_admin(); }

    // Current directories and paths
    fn cwd() { return os_cwd(); }
    fn cd(path) { return os_cd(path); }
    fn chdir(path) { return os_chdir(path); }
    fn home_dir() { return os_home_dir(); }
    fn temp_dir() { return os_temp_dir(); }
    fn executable_path() { return os_executable_path(); }

    fn path_join(a, b) { return os_path_join(a, b); }
    fn basename(path) { return os_basename(path); }
    fn dirname(path) { return os_dirname(path); }

    // Filesystem queries
    fn exists(path) { return os_exists(path); }
    fn is_file(path) { return os_is_file(path); }
    fn is_dir(path) { return os_is_dir(path); }
    fn listdir(path) { return os_listdir(path); }

    // Directory creation and removal
    fn mkdir(path) { return os_mkdir(path); }
    fn mkdir_recursive(path) { return os_mkdir(path, true); }
    fn makedirs(path) { return os_mkdir(path, true); }
    fn rmdir(path) { return os_rmdir(path); }

    // File operations
    fn remove(path) { return os_remove(path); }
    fn unlink(path) { return os_unlink(path); }
    fn rename(old_path, new_path) {
        return os_rename(old_path, new_path);
    }
    fn copy_file(source, destination) {
        return os_copy_file(source, destination);
    }

    // Environment variables
    fn getenv(name) { return os_getenv(name); }
    fn setenv(name, value) { return os_setenv(name, value); }
    fn environ() { return os_environ(); }

    // Processes
    fn process_id() { return os_process_id(); }
    fn spawn(program) { return os_spawn(program); }
    fn wait(pid) { return os_wait(pid); }
    fn terminate(pid) { return os_terminate(pid); }

    // Commands and shell integration
    fn execute(command) { return os_execute(command); }
    fn system(command) { return os_system(command); }
    fn shell_execute(operation, target) {
        return os_shell_execute(operation, target);
    }
    fn shell_execute_args(operation, target, arguments) {
        return os_shell_execute(operation, target, arguments);
    }
    fn shellexec(target) { return os_shellexec(target); }

    // Open resources with the system's default application
    fn open_url(url) { return os_open_url(url); }
    fn open_file(path) { return os_open_file(path); }
    fn open_folder(path) { return os_open_folder(path); }

    // User notifications and system actions
    fn message_box(title, message) {
        return os_message_box(title, message);
    }
    fn message_box_kind(title, message, kind) {
        return os_message_box_kind(title, message, kind);
    }

    // Session and power operations
    fn lock_screen() { return os_lock_screen(); }
    fn logoff(confirm) { return os_logoff(confirm); }
    fn shutdown(confirm) { return os_shutdown(confirm); }
    fn restart(confirm) { return os_restart(confirm); }

    // Time and diagnostics
    fn sleep(milliseconds) { return os_sleep(milliseconds); }
    fn timestamp() { return os_timestamp(); }
    fn uptime() { return os_uptime(); }
    fn last_error() { return os_last_error(); }
}
