#ifndef OCR_NORMAL
#define OCR_NORMAL 32512
#endif
#ifndef OCR_IBEAM
#define OCR_IBEAM 32513
#endif
#ifndef OCR_WAIT
#define OCR_WAIT 32514
#endif
#ifndef OCR_CROSS
#define OCR_CROSS 32515
#endif
#ifndef OCR_UP
#define OCR_UP 32516
#endif
#ifndef OCR_SIZE
#define OCR_SIZE 32540
#endif
#ifndef OCR_ICON
#define OCR_ICON 32580
#endif
#ifndef OCR_SIZENWSE
#define OCR_SIZENWSE 32642
#endif
#ifndef OCR_SIZENESW
#define OCR_SIZENESW 32643
#endif
#ifndef OCR_SIZEWE
#define OCR_SIZEWE 32644
#endif
#ifndef OCR_SIZENS
#define OCR_SIZENS 32645
#endif
#ifndef OCR_SIZEALL
#define OCR_SIZEALL 32646
#endif
#ifndef OCR_NO
#define OCR_NO 32648
#endif
#ifndef OCR_HAND
#define OCR_HAND 32649
#endif
#ifndef OCR_APPSTARTING
#define OCR_APPSTARTING 32650
#endif
#ifndef OCR_HELP
#define OCR_HELP 32651
#endif
#define OEMRSOURCE
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#endif
/*
 * Fluorine 0.1: C99 tree-walking language runtime.
 * Build (POSIX): cc -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror Fluorine.c
 * -lm -ldl -pthread -o Fluorine Build (Windows/MinGW): gcc -std=c99 -O2 -Wall
 * -Wextra -Wpedantic -Werror Fluorine.c -lws2_32 -lshell32 -o Fluorine.exe Run:
 * Fluorine program.f
 *
 * Features include a broad standard library, HTTP/1.1-style local server,
 * arrays, maps, lexical functions/closures, classes, namespaces, structs, safe
 * pointers, OS-thread tasks, exceptions, imports, and FFI. See docs for
 * platform support and limitations. See README.md for the language guide,
 * limitations, and security notes.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#include <process.h>
#include <shellapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <iconv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif
#ifdef FLUORINE_WITH_CURL
#include <curl/curl.h>
#endif

#define INITIAL_CAP 16
#define MAX_ARGS 128

typedef enum {
  T_EOF,
  T_NUMBER,
  T_STRING,
  T_IDENT,
  T_LPAREN,
  T_RPAREN,
  T_LBRACE,
  T_RBRACE,
  T_LBRACKET,
  T_RBRACKET,
  T_COMMA,
  T_SEMI,
  T_COLON,
  T_DOT,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_PERCENT,
  T_CARET,
  T_PLUS_EQUAL,
  T_MINUS_EQUAL,
  T_STAR_EQUAL,
  T_SLASH_EQUAL,
  T_PERCENT_EQUAL,
  T_CARET_EQUAL,
  T_BANG,
  T_BANG_EQUAL,
  T_EQUAL,
  T_EQUAL_EQUAL,
  T_LESS,
  T_LESS_EQUAL,
  T_GREATER,
  T_GREATER_EQUAL,
  T_AND_AND,
  T_OR_OR,
  T_AMP
} FluorineToken;

typedef struct {
  FluorineToken type;
  char *lexeme;
  double number;
  int line;
  int col;
} Token;

typedef struct {
  const char *src;
  size_t pos;
  int line;
  Token *tokens;
  size_t count, cap;
} Lexer;

typedef enum {
  N_NUMBER,
  N_STRING,
  N_BOOL,
  N_NULL,
  N_VAR,
  N_ASSIGN,
  N_ARRAY,
  N_MAP,
  N_INDEX,
  N_INDEX_ASSIGN,
  N_COMPOUND_INDEX_ASSIGN,
  N_UNARY,
  N_BINARY,
  N_UPDATE,
  N_CALL,
  N_IMPORT,
  N_EXPR_STMT,
  N_LET,
  N_BLOCK,
  N_IF,
  N_WHILE,
  N_DO_WHILE,
  N_FOR,
  N_FOR_IN,
  N_FUNCTION,
  N_CLASS,
  N_NAMESPACE,
  N_STRUCT,
  N_TYPEDEF,
  N_ADDRESS,
  N_DEREF,
  N_DEREF_ASSIGN,
  N_DEREF_COMPOUND_ASSIGN,
  N_FLUORINE,
  N_THREAD,
  N_SWITCH,
  N_RETURN,
  N_BREAK,
  N_CONTINUE,
  N_PRINT,
  N_THROW,
  N_TRY
} NodeType;

typedef struct Node Node;
struct Node {
  NodeType type;
  int line;
  char *number_text;
  union {
    double number;
    char *string;
    bool boolean;
    struct {
      char *name;
    } var;
    struct {
      char *name;
      Node *value;
    } assign;
    struct {
      Node **items;
      int count;
    } array;
    struct {
      Node **keys, **values;
      int count;
    } map;
    struct {
      Node *object, *index;
    } index;
    struct {
      Node *object, *index, *value;
    } index_assign;
    struct {
      Node *object, *index, *value;
      FluorineToken op;
    } compound_index_assign;
    struct {
      char *path;
      char *alias;
      bool quoted;
    } import_stmt;
    struct {
      FluorineToken op;
      Node *right;
    } unary;
    struct {
      Node *target;
      int delta;
      bool prefix;
    } update;
    struct {
      Node *left;
      FluorineToken op;
      Node *right;
    } binary;
    struct {
      Node *callee;
      Node **args;
      int count;
    } call;
    struct {
      Node *expr;
    } expr_stmt;
    struct {
      char *name;
      Node *init;
    } let_stmt;
    struct {
      Node **items;
      int count;
    } block;
    struct {
      Node *condition, *then_branch, *else_branch;
    } if_stmt;
    struct {
      Node *expr;
      Node **values;
      Node ***bodies;
      int *counts;
      bool *defaults;
      int count;
    } switch_stmt;
    struct {
      Node *condition, *body;
    } while_stmt;
    struct {
      Node *init, *condition, *increment, *body;
    } for_stmt;
    struct {
      char *name;
      Node *iterable, *body;
    } for_in;
    struct {
      char *name;
      char **params;
      int param_count;
      char *doc;
      Node *body;
    } function;
    struct {
      char *name;
      char *parent;
      Node *body;
    } class_stmt;
    struct {
      char *name;
      Node *body;
    } namespace_stmt;
    struct {
      char *name;
      Node *body;
    } struct_stmt;
    struct {
      char *target;
      char *name;
    } typedef_stmt;
    struct {
      Node *target;
    } pointer_stmt;
    struct {
      Node *pointer, *value;
    } deref_assign;
    struct {
      Node *pointer, *value;
      FluorineToken op;
    } deref_compound_assign;
    struct {
      Node *call;
    } fluorine_stmt;
    struct {
      Node *value;
    } return_stmt;
    struct {
      Node **args;
      int count;
    } print_stmt;
    struct {
      Node *value;
    } throw_stmt;
    struct {
      Node *body;
      char *name;
      Node *handler;
      Node *finally_body;
    } try_stmt;
  } as;
};

typedef struct Value Value;
typedef struct Env Env;
typedef struct FunctionValue FunctionValue;

typedef struct Array Array;
typedef struct Map Map;
typedef struct SafePointer SafePointer;
typedef struct FluorineThread FluorineThread;
struct Array {
  Value *items;
  int count, capacity;
  bool marked;
  Array *next;
};
typedef struct MapEntry MapEntry;
struct Map {
  MapEntry *items;
  int count, capacity;
  bool marked;
  Map *next;
};
typedef enum {
  V_NULL,
  V_NUMBER,
  V_BIGINT,
  V_RATIONAL,
  V_STRING,
  V_BOOL,
  V_FUNCTION,
  V_NATIVE,
  V_ARRAY,
  V_MAP,
  V_CLASS,
  V_STRUCT_DEF,
  V_POINTER,
  V_THREAD
} ValueType;
typedef Value (*NativeFn)(int argc, Value *argv, int line);

struct Value {
  ValueType type;
  union {
    double number;
    char *bigint;
    struct {
      char *num;
      char *den;
    } rational;
    char *string;
    bool boolean;
    FunctionValue *function;
    NativeFn native;
    Array *array;
    Map *map;
    SafePointer *pointer;
    FluorineThread *thread;
  } as;
};
struct MapEntry {
  Value key, value;
};
/* Pointers are opaque, GC-managed references to interpreter storage, never C
 * addresses. */
typedef enum { PTR_BINDING, PTR_MAP_ENTRY, PTR_ARRAY_ITEM } SafePointerKind;
struct SafePointer {
  SafePointerKind kind;
  bool marked;
  Env *env;
  char *name;
  Map *map;
  Value key;
  Array *array;
  int index;
  SafePointer *next;
};
static SafePointer *all_pointers = NULL;

typedef struct {
  char *name;
  Value value;
} Binding;

struct Env {
  bool marked;
  Env *parent;
  Binding *bindings;
  int count, capacity;
};

struct FunctionValue {
  bool marked;
  Node *declaration;
  Env *closure;
};

/* Native OS threads execute real system threads. Interpreter callbacks are
 * serialized by a recursive runtime lock because environments, active frames,
 * exception state, and the tracing collector are currently process-global. */
struct FluorineThread {
  bool started, joined, done;
#ifdef _WIN32
  HANDLE handle;
  DWORD thread_id;
#else
  pthread_t handle;
#endif
  Value callee;
  char *native_name;
  Value *args;
  int argc, line;
  Value result;
  FluorineThread *next;
};
static FluorineThread *all_threads = NULL;
/* Portable thread-local storage spelling for the supported Windows toolchains:
 * MSVC/clang-cl use __declspec(thread), while GCC and MinGW-targeting Clang
 * use __thread. C11 is a fallback for other conforming compilers. */
#if defined(_MSC_VER)
#define FLUORINE_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define FLUORINE_THREAD_LOCAL __thread
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define FLUORINE_THREAD_LOCAL _Thread_local
#else
#error "Fluorine requires compiler-supported thread-local storage"
#endif

#ifdef _WIN32
static CRITICAL_SECTION fluorine_runtime_mutex;
static bool fluorine_runtime_mutex_ready = false;
static FLUORINE_THREAD_LOCAL int fluorine_runtime_lock_depth = 0;
static void runtime_mutex_init(void) {
  if (!fluorine_runtime_mutex_ready) {
    InitializeCriticalSection(&fluorine_runtime_mutex);
    fluorine_runtime_mutex_ready = true;
  }
}
static void runtime_lock(void) {
  runtime_mutex_init();
  EnterCriticalSection(&fluorine_runtime_mutex);
  fluorine_runtime_lock_depth++;
}
static void runtime_unlock(void) {
  if (fluorine_runtime_lock_depth > 0) {
    fluorine_runtime_lock_depth--;
    LeaveCriticalSection(&fluorine_runtime_mutex);
  }
}
#else
static pthread_mutex_t fluorine_runtime_mutex;
static bool fluorine_runtime_mutex_ready = false;
static FLUORINE_THREAD_LOCAL int fluorine_runtime_lock_depth = 0;
static void runtime_mutex_init(void) {
  if (!fluorine_runtime_mutex_ready) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&fluorine_runtime_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    fluorine_runtime_mutex_ready = true;
  }
}
static void runtime_lock(void) {
  runtime_mutex_init();
  pthread_mutex_lock(&fluorine_runtime_mutex);
  fluorine_runtime_lock_depth++;
}
static void runtime_unlock(void) {
  if (fluorine_runtime_lock_depth > 0) {
    fluorine_runtime_lock_depth--;
    pthread_mutex_unlock(&fluorine_runtime_mutex);
  }
}
#endif
static int runtime_unlock_all(void) {
  int depth = fluorine_runtime_lock_depth;
  while (fluorine_runtime_lock_depth > 0)
    runtime_unlock();
  return depth;
}
static void runtime_relock(int depth) {
  while (depth-- > 0)
    runtime_lock();
}
static void join_all_threads(void);

/* Cooperative green threads use a saved stackful context per fluorine task. */
typedef struct FluorineTask {
  Value callee;
  Value *args;
  int argc;
  int line;
  char *native_name; /* non-NULL when the task entry is a built-in */
  bool initialized, finished;
#ifdef _WIN32
  void *fiber;
#else
  ucontext_t context;
  void *stack;
#endif
  struct ActiveEnv *saved_active_envs;
  int saved_exec_depth, saved_exception_handler_depth;
  bool saved_has_exception;
  Value saved_pending_exception;
  struct FluorineTask *next;
  struct FluorineTask *all_next;
} FluorineTask;
static FluorineTask *fluorine_tasks_head = NULL, *fluorine_tasks_tail = NULL,
                    *fluorine_current_task = NULL;
static FluorineTask *fluorine_all_tasks = NULL;
static bool fluorine_scheduler_running = false;
#ifdef _WIN32
static void *fluorine_scheduler_fiber = NULL;
#else
static ucontext_t fluorine_scheduler_context;
#endif
static struct ActiveEnv *fluorine_scheduler_active_envs = NULL;
static int fluorine_scheduler_exec_depth = 0,
           fluorine_scheduler_exception_handler_depth = 0;
static bool fluorine_scheduler_has_exception = false;
static Value fluorine_scheduler_pending_exception;
static void run_fluorine_tasks(void);
static void run_fluorine_tasks_step(void);
static void fluorine_task_yield(void);

static bool is_native_builtin_name(const char *name) {
  static const char *names[] = {"sqrt",
                                "abs",
                                "floor",
                                "ceil",
                                "round",
                                "sin",
                                "cos",
                                "tan",
                                "asin",
                                "acos",
                                "atan",
                                "log",
                                "log10",
                                "exp",
                                "min",
                                "max",
                                "pow",
                                "input",
                                "printf",
                                "print",
                                "is_number",
                                "len",
                                "push",
                                "pop",
                                "range",
                                "py",
                                "native",
                                "keys",
                                "has_key",
                                "py_inline",
                                "c_inline",
                                "asm_inline",
                                "gc",
                                "to_number",
                                "to_string",
                                "type_of",
                                "ascii_array",
                                "substring",
                                "split",
                                "join",
                                "trim",
                                "upper",
                                "lower",
                                "contains",
                                "starts_with",
                                "ends_with",
                                "replace",
                                "ord",
                                "chr",
                                "random",
                                "read_text",
                                "write_text",
                                "append_text",
                                "file_exists",
                                "remove_file",
                                "file_size",
                                "copy_file",
                                "rename_file",
                                "make_dir",
                                "is_dir",
                                "env",
                                "set_env",
                                "clock_ms",
                                "sleep_ms",
                                "assert",
                                "http_serve",
                                "http_request",
                                "http_get",
                                "thread_start",
                                "thread_join",
                                "thread_is_done",
                                "thread_count",
                                "url_encode",
                                "url_decode",
                                "path_join",
                                "basename",
                                "dirname",
                                "unix_time",
                                "time_string",
                                "seed_random",
                                "os_os",
                                "os_system",
                                "os_shellexec",
                                "os_name",
                                "os_version",
                                "os_architecture",
                                "os_hostname",
                                "os_username",
                                "os_cwd",
                                "os_path_join",
                                "os_basename",
                                "os_dirname",
                                "os_cd",
                                "os_chdir",
                                "os_mkdir",
                                "os_makedirs",
                                "os_rmdir",
                                "os_exists",
                                "os_is_file",
                                "os_is_dir",
                                "os_remove",
                                "os_unlink",
                                "os_rename",
                                "os_copy_file",
                                "os_getenv",
                                "os_setenv",
                                "os_environ",
                                "os_listdir",
                                "os_home_dir",
                                "os_temp_dir",
                                "os_executable_path",
                                "os_process_id",
                                "os_sleep",
                                "os_timestamp",
                                "os_execute",
                                "os_uptime",
                                "os_last_error",
                                "os_shell_execute",
                                "os_open_url",
                                "os_open_file",
                                "os_open_folder",
                                "os_spawn",
                                "os_wait",
                                "os_terminate",
                                "os_message_box",
                                "os_lock_screen",
                                "os_logoff",
                                "os_shutdown",
                                "os_restart",
                                "os_is_admin",
                                "shellexec",
                                "bit_and",
                                "bit_or",
                                "bit_xor",
                                "bit_not",
                                "bit_shl",
                                "bit_shr",
                                "byte_get",
                                "byte_set",
                                "read_u16_le",
                                "read_u16_be",
                                "read_u32_le",
                                "read_u32_be",
                                "read_u64_le",
                                "read_u64_be",
                                "write_u16_le",
                                "write_u16_be",
                                "write_u32_le",
                                "write_u32_be",
                                "write_u64_le",
                                "write_u64_be",
                                "mouse_move",
                                "mouse_left_click",
                                "mouse_right_click",
                                "mouse_middle_click",
                                "mouse_left_click_at",
                                "mouse_right_click_at",
                                "mouse_position",
                                "mouse_scroll",
                                "mouse_smooth_move",
                                "mouse_set_position",
                                "mouse_double_click",
                                "mouse_press",
                                "mouse_release",
                                "mouse_is_button_down",
                                "mouse_screen_size",
                                "mouse_show_cursor",
                                "mouse_set_system_cursor",
                                "mouse_restore_system_cursor",
                                "mouse_is_available",
                                "mouse_wait_button",
                                "keyboard_wait_key",
                                "keyboard_is_key_down",
                                "keyboard_type_text",
                                "keyboard_press",
                                "keyboard_key_down",
                                "keyboard_key_up",
                                "std_format_convert",
                                "std_uuid_v4",
                                "std_uuid_v7",
                                "std_sha256",
                                "std_json_stringify",
                                "std_json_parse",
                                "std_datetime_iso",
                                "std_datetime_parts",
                                "std_cli_args",
                                "std_csv_parse",
                                "std_toml_parse",
                                "std_yaml_parse"};
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    if (strcmp(name, names[i]) == 0)
      return true;
  return false;
}

typedef struct {
  Token *tokens;
  size_t pos, count;
} Parser;

typedef struct {
  bool is_return;
  bool is_break;
  bool is_continue;
  bool is_throw;
  Value value;
} Flow;

static int had_error = 0;
static int fluorine_argc = 0;
static char **fluorine_argv = NULL;
/* Automatic semicolon insertion is enabled by default. */
static bool asi_enabled = true;
static bool debug_enabled = false;
static const char *debug_source_text = NULL;
static bool has_exception = false;
static int exception_handler_depth = 0;
static Value pending_exception;
static Env *all_envs = NULL;
typedef struct EnvLink {
  Env *env;
  struct EnvLink *next;
} EnvLink;
static EnvLink *env_list = NULL;
static FunctionValue **all_functions = NULL;
static size_t function_count = 0, function_capacity = 0;
static Array *all_arrays = NULL;
static Map *all_maps = NULL;
static Env *global_env = NULL;
typedef struct ActiveEnv {
  Env *env;
  struct ActiveEnv *next;
} ActiveEnv;
static ActiveEnv *active_envs = NULL;
static size_t gc_allocations = 0;
static bool gc_running = false;
static bool gc_pending = false;
static int exec_depth = 0;
static size_t gc_collect(void);
static char imported_paths[256][4096];
static size_t imported_count = 0;
static char isolated_module_paths[256][4096];
static Map *isolated_module_maps[256];
static size_t isolated_module_count = 0;
static int import_depth = 0;
static Node **imported_programs = NULL;
static size_t imported_program_count = 0, imported_program_capacity = 0;

static const char *beginner_tip_for_error(const char *message) {
  if (strstr(message, "Undefined variable") ||
      strstr(message, "Undefined function"))
    return "Check the spelling of the name and make sure you created it before "
           "using it. If it belongs to a module, check that you imported the "
           "module.";
  if (strstr(message, "requires numbers") ||
      strstr(message, "require numbers") ||
      strstr(message, "requires a number") ||
      strstr(message, "numeric values") ||
      strstr(message, "needs two numbers") || strstr(message, "needs a number"))
    return "This operation needs a number. Check the values on both sides and "
           "convert text to a number when appropriate.";
  if (strstr(message, "Comparison requires") || strstr(message, "comparison"))
    return "Compare values of compatible kinds, such as two numbers or two "
           "pieces of text.";
  if (strstr(message, "Division by zero") ||
      strstr(message, "Remainder by zero"))
    return "The value after '/' or '%' must not be zero. Check the calculation "
           "before this line.";
  if (strstr(message, "expects") && strstr(message, "argument"))
    return "Check the function's required inputs: the number of values and "
           "their order must match the function call.";
  if (strstr(message, "Index") || strstr(message, "index") ||
      strstr(message, "out of range"))
    return "Check that the value can be indexed and that the index is valid. "
           "Array and string positions start at 0; negative indexes count from "
           "the end.";
  if (strstr(message, "Member does not exist") ||
      strstr(message, "Map key not found"))
    return "Check the property or key name. It may be misspelled, or that "
           "property may not have been created yet.";
  if (strstr(message, "cannot be called") || strstr(message, "not a function"))
    return "Only functions and constructors can be called with parentheses. "
           "Check the value before the parentheses.";
  if (strstr(message, "semicolon") || strstr(message, "';'"))
    return "Check the end of this statement. Add ';' where needed, or check "
           "that the previous line is complete.";
  if (strstr(message, "Expected") || strstr(message, "Syntax error"))
    return "Check the nearby punctuation and spelling. The message describes "
           "what Fluorine expected at this point in the code.";
  return "Read the explanation above, then check the code on this line and the "
         "values it uses.";
}

static void runtime_error(int line, const char *fmt, ...) {
  va_list ap;
  char message[2048];
  va_start(ap, fmt);
  (void)vsnprintf(message, sizeof(message), fmt, ap);
  va_end(ap);
  if (exception_handler_depth > 0 && !has_exception) {
    char formatted[2304];
    (void)snprintf(formatted, sizeof(formatted),
                   "Runtime error on line %d: %s. Tip: %s", line, message,
                   beginner_tip_for_error(message));
    pending_exception.type = V_NULL;
    pending_exception.type = V_STRING;
    pending_exception.as.string = (char *)malloc(strlen(formatted) + 1);
    if (!pending_exception.as.string) {
      fprintf(stderr, "Out of memory\n");
      exit(2);
    }
    strcpy(pending_exception.as.string, formatted);
    has_exception = true;
    return;
  }
  fprintf(stderr, "\nError on line %d: %s\nHow to fix it: %s\n\n", line,
          message, beginner_tip_for_error(message));
  had_error = 1;
}

static void syntax_error(int line, const char *fmt, ...) {
  va_list ap;
  char message[2048];
  va_start(ap, fmt);
  (void)vsnprintf(message, sizeof(message), fmt, ap);
  va_end(ap);
  fprintf(stderr, "Syntax error on line %d: %s\nWhat to try: %s\n\n", line,
          message, beginner_tip_for_error(message));
  had_error = 1;
}

static char *copy_n(const char *s, size_t n) {
  char *out = (char *)malloc(n + 1);
  if (!out) {
    fprintf(stderr, "Out of memory\n");
    exit(2);
  }
  memcpy(out, s, n);
  out[n] = '\0';
  return out;
}

static char *copy_s(const char *s) { return copy_n(s, strlen(s)); }

static void *xrealloc(void *p, size_t n) {
  void *q = realloc(p, n);
  if (!q) {
    fprintf(stderr, "Out of memory\n");
    exit(2);
  }
  return q;
}

static void add_token(Lexer *l, FluorineToken type, size_t start, size_t end,
                      double number, int line) {
  if (l->count == l->cap) {
    l->cap = l->cap ? l->cap * 2 : INITIAL_CAP;
    l->tokens = (Token *)xrealloc(l->tokens, l->cap * sizeof(Token));
  }
  Token *t = &l->tokens[l->count++];
  t->type = type;
  t->lexeme = copy_n(l->src + start, end - start);
  t->number = number;
  t->line = line;
  t->col = 1;
  for (size_t i = start; i > 0 && l->src[i - 1] != '\n'; --i)
    t->col++;
}

static bool is_at_end(Lexer *l) { return l->src[l->pos] == '\0'; }
static char peek_char(Lexer *l) { return l->src[l->pos]; }
static char peek_next(Lexer *l) {
  return l->src[l->pos] ? l->src[l->pos + 1] : '\0';
}
static char advance_char(Lexer *l) { return l->src[l->pos++]; }

static void lex_source(Lexer *l) {
  while (!is_at_end(l)) {
    char c = advance_char(l);
    size_t start = l->pos - 1;
    int line = l->line;
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      break;
    case '\n':
      l->line++;
      break;
    case '#':
      while (!is_at_end(l) && peek_char(l) != '\n')
        advance_char(l);
      break;
    case '/':
      if (peek_char(l) == '/') {
        while (!is_at_end(l) && peek_char(l) != '\n')
          advance_char(l);
      } else if (peek_char(l) == '*') {
        /* C-style block comments. Track newlines so later diagnostics
         * retain correct source line numbers. */
        advance_char(l); /* consume '*' */
        bool closed = false;
        while (!is_at_end(l)) {
          char comment_char = advance_char(l);
          if (comment_char == '\n')
            l->line++;
          if (comment_char == '*' && peek_char(l) == '/') {
            advance_char(l);
            closed = true;
            break;
          }
        }
        if (!closed)
          syntax_error(line, "Unterminated block comment; expected '*/'.");
      } else {
        FluorineToken t =
            peek_char(l) == '=' ? (advance_char(l), T_SLASH_EQUAL) : T_SLASH;
        add_token(l, t, start, l->pos, 0, line);
      }
      break;
    case '(':
      add_token(l, T_LPAREN, start, l->pos, 0, line);
      break;
    case ')':
      add_token(l, T_RPAREN, start, l->pos, 0, line);
      break;
    case '{':
      add_token(l, T_LBRACE, start, l->pos, 0, line);
      break;
    case '}':
      add_token(l, T_RBRACE, start, l->pos, 0, line);
      break;
    case '[':
      add_token(l, T_LBRACKET, start, l->pos, 0, line);
      break;
    case ']':
      add_token(l, T_RBRACKET, start, l->pos, 0, line);
      break;
    case ',':
      add_token(l, T_COMMA, start, l->pos, 0, line);
      break;
    case ';':
      add_token(l, T_SEMI, start, l->pos, 0, line);
      break;
    case ':':
      add_token(l, T_COLON, start, l->pos, 0, line);
      break;
    case '+': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_PLUS_EQUAL) : T_PLUS;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '-': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_MINUS_EQUAL) : T_MINUS;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '*': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_STAR_EQUAL) : T_STAR;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '%': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_PERCENT_EQUAL) : T_PERCENT;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '^': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_CARET_EQUAL) : T_CARET;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '!': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_BANG_EQUAL) : T_BANG;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '=': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_EQUAL_EQUAL) : T_EQUAL;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '<': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_LESS_EQUAL) : T_LESS;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '>': {
      FluorineToken t =
          peek_char(l) == '=' ? (advance_char(l), T_GREATER_EQUAL) : T_GREATER;
      add_token(l, t, start, l->pos, 0, line);
      break;
    }
    case '&':
      if (peek_char(l) == '&') {
        advance_char(l);
        add_token(l, T_AND_AND, start, l->pos, 0, line);
      } else
        add_token(l, T_AMP, start, l->pos, 0, line);
      break;
    case '|':
      if (peek_char(l) == '|') {
        advance_char(l);
        add_token(l, T_OR_OR, start, l->pos, 0, line);
      } else
        syntax_error(line, "Expected '|' after '|'.");
      break;
    case '"':
    case '\'': {
      char quote = c;
      size_t content_start = l->pos;
      bool escaped = false;
      while (!is_at_end(l)) {
        char q = advance_char(l);
        if (q == '\n')
          l->line++;
        if (!escaped && q == quote)
          break;
        if (!escaped && q == '\\')
          escaped = true;
        else
          escaped = false;
      }
      if (l->src[l->pos - 1] != quote) {
        syntax_error(line, "Unterminated string literal.");
      } else {
        add_token(l, T_STRING, content_start, l->pos - 1, 0, line);
      }
      break;
    }
    default:
      if (c == '.' && !isdigit((unsigned char)peek_char(l))) {
        add_token(l, T_DOT, start, l->pos, 0, line);
      } else if (isdigit((unsigned char)c) ||
                 (c == '.' && isdigit((unsigned char)peek_char(l)))) {
        while (isdigit((unsigned char)peek_char(l)))
          advance_char(l);
        if (peek_char(l) == '.' && isdigit((unsigned char)peek_next(l))) {
          advance_char(l);
          while (isdigit((unsigned char)peek_char(l)))
            advance_char(l);
        }
        if (peek_char(l) == 'e' || peek_char(l) == 'E') {
          size_t save = l->pos;
          advance_char(l);
          if (peek_char(l) == '+' || peek_char(l) == '-')
            advance_char(l);
          if (isdigit((unsigned char)peek_char(l))) {
            while (isdigit((unsigned char)peek_char(l)))
              advance_char(l);
          } else
            l->pos = save;
        }
        char *num = copy_n(l->src + start, l->pos - start);
        char *endptr;
        double d = strtod(num, &endptr);
        if (*endptr)
          syntax_error(line, "Invalid number '%s'.", num);
        add_token(l, T_NUMBER, start, l->pos, d, line);
        free(num);
      } else if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)peek_char(l)) || peek_char(l) == '_')
          advance_char(l);
        add_token(l, T_IDENT, start, l->pos, 0, line);
      } else {
        syntax_error(line, "Unexpected character '%c'.", c);
      }
    }
  }
  add_token(l, T_EOF, l->pos, l->pos, 0, l->line);
}

static Node *new_node(NodeType type, int line) {
  Node *n = (Node *)calloc(1, sizeof(Node));
  if (!n) {
    fprintf(stderr, "Out of memory\n");
    exit(2);
  }
  n->type = type;
  n->line = line;
  return n;
}

static Token *current(Parser *p) { return &p->tokens[p->pos]; }
static Token *previous(Parser *p) { return &p->tokens[p->pos - 1]; }
static bool check(Parser *p, FluorineToken t) { return current(p)->type == t; }
static bool match(Parser *p, FluorineToken t) {
  if (check(p, t)) {
    p->pos++;
    return true;
  }
  return false;
}
static Token *consume(Parser *p, FluorineToken t, const char *message) {
  if (check(p, t))
    return &p->tokens[p->pos++];
  fprintf(stderr, "Syntax error on line %d, column %d: %s\nWhat to try: %s\n\n",
          current(p)->line, current(p)->col, message,
          beginner_tip_for_error(message));
  had_error = 1;
  return current(p);
}
static bool keyword(Parser *p, const char *s) {
  return check(p, T_IDENT) && strcmp(current(p)->lexeme, s) == 0;
}
static bool match_kw(Parser *p, const char *s) {
  if (keyword(p, s)) {
    p->pos++;
    return true;
  }
  return false;
}
static Node *parse_expression(Parser *p);
static Node *parse_statement(Parser *p);
static Node *parse_declaration(Parser *p);
static void free_node(Node *n);

static Node *parse_primary(Parser *p) {
  Token *t = current(p);
  if (match(p, T_NUMBER)) {
    Node *n = new_node(N_NUMBER, t->line);
    n->as.number = t->number;
    n->number_text = copy_s(t->lexeme);
    return n;
  }
  if (match(p, T_STRING)) {
    Node *n = new_node(N_STRING, t->line);
    /* Decode basic escapes while copying. */
    const char *s = t->lexeme;
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out)
      exit(2);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
      if (s[i] == '\\' && i + 1 < len) {
        i++;
        switch (s[i]) {
        case 'n':
          out[j++] = '\n';
          break;
        case 'r':
          out[j++] = '\r';
          break;
        case 't':
          out[j++] = '\t';
          break;
        case '\\':
          out[j++] = '\\';
          break;
        case '"':
          out[j++] = '"';
          break;
        case '\'':
          out[j++] = '\'';
          break;
        default:
          out[j++] = s[i];
          break;
        }
      } else
        out[j++] = s[i];
    }
    out[j] = '\0';
    n->as.string = out;
    return n;
  }
  if (match(p, T_LBRACKET)) {
    Node *n = new_node(N_ARRAY, t->line);
    n->as.array.items = NULL;
    n->as.array.count = 0;
    if (!check(p, T_RBRACKET)) {
      do {
        if (n->as.array.count >= MAX_ARGS) {
          syntax_error(current(p)->line, "Array literal exceeds %d elements.",
                       MAX_ARGS);
          break;
        }
        n->as.array.items =
            (Node **)xrealloc(n->as.array.items,
                              (size_t)(n->as.array.count + 1) * sizeof(Node *));
        n->as.array.items[n->as.array.count++] = parse_expression(p);
      } while (match(p, T_COMMA));
    }
    consume(p, T_RBRACKET, "Expected ']' after array literal.");
    return n;
  }
  if (match(p, T_LBRACE)) {
    Node *n = new_node(N_MAP, t->line);
    n->as.map.keys = NULL;
    n->as.map.values = NULL;
    n->as.map.count = 0;
    if (!check(p, T_RBRACE)) {
      do {
        if (n->as.map.count >= MAX_ARGS) {
          syntax_error(current(p)->line, "Map literal exceeds %d entries.",
                       MAX_ARGS);
          break;
        }
        Node *key = parse_expression(p);
        consume(p, T_COLON, "Expected ':' between map key and value.");
        Node *value = parse_expression(p);
        int i = n->as.map.count++;
        n->as.map.keys = (Node **)xrealloc(
            n->as.map.keys, (size_t)n->as.map.count * sizeof(Node *));
        n->as.map.values = (Node **)xrealloc(
            n->as.map.values, (size_t)n->as.map.count * sizeof(Node *));
        n->as.map.keys[i] = key;
        n->as.map.values[i] = value;
      } while (match(p, T_COMMA));
    }
    consume(p, T_RBRACE, "Expected '}' after map literal.");
    return n;
  }
  if (match_kw(p, "true")) {
    Node *n = new_node(N_BOOL, t->line);
    n->as.boolean = true;
    return n;
  }
  if (match_kw(p, "false")) {
    Node *n = new_node(N_BOOL, t->line);
    n->as.boolean = false;
    return n;
  }
  if (match_kw(p, "null"))
    return new_node(N_NULL, t->line);
  if (check(p, T_IDENT) && (strcmp(current(p)->lexeme, "let") == 0 ||
                            strcmp(current(p)->lexeme, "typedef") == 0)) {
    syntax_error(t->line,
                 "Reserved keyword cannot be used as a variable name.");
    p->pos++;
    return new_node(N_NULL, t->line);
  }
  if (match(p, T_IDENT)) {
    Node *n = new_node(N_VAR, t->line);
    n->as.var.name = copy_s(t->lexeme);
    return n;
  }
  if (match(p, T_LPAREN)) {
    Node *e = parse_expression(p);
    consume(p, T_RPAREN, "Expected ')' after expression.");
    return e;
  }
  syntax_error(t->line, "Expected an expression.");
  if (!check(p, T_EOF))
    p->pos++;
  return new_node(N_NULL, t->line);
}

static Node *parse_call(Parser *p) {
  Node *expr = parse_primary(p);
  for (;;) {
    /* A newline before a call/index/property can silently merge two
       statements. Require explicit punctuation rather than guessing. */
    if (asi_enabled && p->pos > 0 && current(p)->line > previous(p)->line &&
        (check(p, T_LPAREN) || check(p, T_LBRACKET) || check(p, T_DOT))) {
      fprintf(stderr,
              "Syntax error, line %d, col %d: ASI would create an ambiguous "
              "statement; add an explicit ';'.\n",
              current(p)->line, current(p)->col);
      had_error = 1;
      return expr;
    }
    if (match(p, T_LPAREN)) {
      Node *n = new_node(N_CALL, previous(p)->line);
      n->as.call.callee = expr;
      n->as.call.args = NULL;
      n->as.call.count = 0;
      if (!check(p, T_RPAREN)) {
        do {
          if (n->as.call.count >= MAX_ARGS) {
            syntax_error(current(p)->line, "Too many call arguments.");
            break;
          }
          n->as.call.args = (Node **)xrealloc(
              n->as.call.args, (size_t)(n->as.call.count + 1) * sizeof(Node *));
          n->as.call.args[n->as.call.count++] = parse_expression(p);
        } while (match(p, T_COMMA));
      }
      consume(p, T_RPAREN, "Expected ')' after arguments.");
      expr = n;
    } else if (match(p, T_LBRACKET)) {
      Node *n = new_node(N_INDEX, previous(p)->line);
      n->as.index.object = expr;
      n->as.index.index = parse_expression(p);
      consume(p, T_RBRACKET, "Expected ']' after index.");
      expr = n;
    } else if (match(p, T_DOT)) {
      Token *name = consume(p, T_IDENT, "Expected property name after '.'.");
      Node *key = new_node(N_STRING, name->line);
      key->as.string = copy_s(name->lexeme);
      Node *n = new_node(N_INDEX, name->line);
      n->as.index.object = expr;
      n->as.index.index = key;
      expr = n;
    } else
      break;
  }
  return expr;
}

static bool is_lvalue_node(Node *n) {
  return n && (n->type == N_VAR || n->type == N_INDEX || n->type == N_DEREF);
}
static Node *parse_unary(Parser *p) {
  if ((check(p, T_PLUS) || check(p, T_MINUS)) && p->pos + 1 < p->count &&
      p->tokens[p->pos + 1].type == current(p)->type) {
    Token *op = current(p);
    p->pos += 2;
    Node *target = parse_unary(p);
    if (!is_lvalue_node(target)) {
      syntax_error(op->line,
                   "Prefix increment/decrement requires an assignable "
                   "variable, property, index, or dereferenced pointer.");
      return target;
    }
    Node *n = new_node(N_UPDATE, op->line);
    n->as.update.target = target;
    n->as.update.delta = (op->type == T_PLUS ? 1 : -1);
    n->as.update.prefix = true;
    return n;
  }
  if (match(p, T_AMP)) {
    Token *op = previous(p);
    Node *n = new_node(N_ADDRESS, op->line);
    n->as.pointer_stmt.target = parse_unary(p);
    return n;
  }
  if (match(p, T_STAR)) {
    Token *op = previous(p);
    Node *n = new_node(N_DEREF, op->line);
    n->as.pointer_stmt.target = parse_unary(p);
    return n;
  }
  if (match(p, T_BANG) || match(p, T_MINUS) || match(p, T_PLUS)) {
    Token *op = previous(p);
    Node *n = new_node(N_UNARY, op->line);
    n->as.unary.op = op->type;
    n->as.unary.right = parse_unary(p);
    return n;
  }
  Node *expr = parse_call(p);
  while ((check(p, T_PLUS) || check(p, T_MINUS)) && p->pos + 1 < p->count &&
         p->tokens[p->pos + 1].type == current(p)->type) {
    Token *op = current(p);
    p->pos += 2;
    if (!is_lvalue_node(expr)) {
      syntax_error(op->line,
                   "Postfix increment/decrement requires an assignable "
                   "variable, property, index, or dereferenced pointer.");
      return expr;
    }
    Node *n = new_node(N_UPDATE, op->line);
    n->as.update.target = expr;
    n->as.update.delta = (op->type == T_PLUS ? 1 : -1);
    n->as.update.prefix = false;
    expr = n;
  }
  return expr;
}
static Node *parse_power(Parser *p) {
  Node *e = parse_unary(p);
  if (match(p, T_CARET)) {
    Node *n = new_node(N_BINARY, previous(p)->line);
    n->as.binary.left = e;
    n->as.binary.op = T_CARET;
    n->as.binary.right = parse_power(p);
    return n;
  }
  return e;
}
static Node *binary_chain(Parser *p, Node *(*next)(Parser *),
                          const FluorineToken *ops, size_t opcount) {
  Node *e = next(p);
  for (;;) {
    FluorineToken found = T_EOF;
    for (size_t i = 0; i < opcount; i++)
      if (check(p, ops[i])) {
        found = ops[i];
        break;
      }
    if (found == T_EOF)
      break;
    Token *op = current(p);
    p->pos++;
    Node *n = new_node(N_BINARY, op->line);
    n->as.binary.left = e;
    n->as.binary.op = found;
    n->as.binary.right = next(p);
    e = n;
  }
  return e;
}
static Node *parse_factor(Parser *p) {
  const FluorineToken o[] = {T_STAR, T_SLASH, T_PERCENT};
  return binary_chain(p, parse_power, o, 3);
}
static Node *parse_term(Parser *p) {
  const FluorineToken o[] = {T_PLUS, T_MINUS};
  return binary_chain(p, parse_factor, o, 2);
}
static Node *parse_comparison(Parser *p) {
  const FluorineToken o[] = {T_LESS, T_LESS_EQUAL, T_GREATER, T_GREATER_EQUAL};
  return binary_chain(p, parse_term, o, 4);
}
static Node *parse_equality(Parser *p) {
  const FluorineToken o[] = {T_EQUAL_EQUAL, T_BANG_EQUAL};
  return binary_chain(p, parse_comparison, o, 2);
}
static Node *parse_and(Parser *p) {
  const FluorineToken o[] = {T_AND_AND};
  return binary_chain(p, parse_equality, o, 1);
}
static Node *parse_or(Parser *p) {
  const FluorineToken o[] = {T_OR_OR};
  return binary_chain(p, parse_and, o, 1);
}

static FluorineToken compound_binary_op(FluorineToken op) {
  switch (op) {
  case T_PLUS_EQUAL:
    return T_PLUS;
  case T_MINUS_EQUAL:
    return T_MINUS;
  case T_STAR_EQUAL:
    return T_STAR;
  case T_SLASH_EQUAL:
    return T_SLASH;
  case T_PERCENT_EQUAL:
    return T_PERCENT;
  case T_CARET_EQUAL:
    return T_CARET;
  default:
    return T_EOF;
  }
}
static Node *parse_assignment(Parser *p) {
  Node *left = parse_or(p);
  FluorineToken raw = current(p)->type;
  FluorineToken binary_op = compound_binary_op(raw);
  if (raw != T_EQUAL && binary_op == T_EOF)
    return left;
  Token *eq = current(p);
  p->pos++;
  Node *right = parse_assignment(p);
  if (raw != T_EQUAL && left->type == N_DEREF) {
    Node *n = new_node(N_DEREF_COMPOUND_ASSIGN, eq->line);
    n->as.deref_compound_assign.pointer = left->as.pointer_stmt.target;
    n->as.deref_compound_assign.value = right;
    n->as.deref_compound_assign.op = binary_op;
    free(left);
    return n;
  }
  if (raw != T_EQUAL && left->type == N_INDEX) {
    Node *n = new_node(N_COMPOUND_INDEX_ASSIGN, eq->line);
    n->as.compound_index_assign.object = left->as.index.object;
    n->as.compound_index_assign.index = left->as.index.index;
    n->as.compound_index_assign.value = right;
    n->as.compound_index_assign.op = binary_op;
    free(left);
    return n;
  }
  if (raw != T_EQUAL) {
    if (left->type != N_VAR) {
      syntax_error(eq->line, "Compound assignment requires a variable, indexed "
                             "element, or property target.");
      free_node(left);
      free_node(right);
      return new_node(N_NULL, eq->line);
    }
    Node *read = new_node(N_VAR, left->line);
    read->as.var.name = copy_s(left->as.var.name);
    Node *binary = new_node(N_BINARY, eq->line);
    binary->as.binary.left = read;
    binary->as.binary.op = binary_op;
    binary->as.binary.right = right;
    right = binary;
  }
  if (left->type == N_VAR) {
    Node *n = new_node(N_ASSIGN, eq->line);
    n->as.assign.name = copy_s(left->as.var.name);
    n->as.assign.value = right;
    free_node(left);
    return n;
  }
  if (raw == T_EQUAL && left->type == N_INDEX) {
    Node *n = new_node(N_INDEX_ASSIGN, eq->line);
    n->as.index_assign.object = left->as.index.object;
    n->as.index_assign.index = left->as.index.index;
    n->as.index_assign.value = right;
    free(left);
    return n;
  }
  if (raw == T_EQUAL && left->type == N_DEREF) {
    Node *n = new_node(N_DEREF_ASSIGN, eq->line);
    n->as.deref_assign.pointer = left->as.pointer_stmt.target;
    n->as.deref_assign.value = right;
    free(left);
    return n;
  }
  syntax_error(eq->line, "Invalid assignment target; expected variable, "
                         "property, index, or dereferenced pointer.");
  free_node(left);
  free_node(right);
  return new_node(N_NULL, eq->line);
}
static bool asi_can_end_statement(FluorineToken t) {
  return t == T_IDENT || t == T_NUMBER || t == T_STRING || t == T_RPAREN ||
         t == T_RBRACKET || t == T_RBRACE || t == T_BANG;
}
static bool asi_continuation_token(FluorineToken t) {
  switch (t) {
  case T_LPAREN:
  case T_LBRACKET:
  case T_DOT:
  case T_PLUS:
  case T_MINUS:
  case T_STAR:
  case T_SLASH:
  case T_PERCENT:
  case T_CARET:
  case T_EQUAL:
  case T_EQUAL_EQUAL:
  case T_BANG_EQUAL:
  case T_LESS:
  case T_LESS_EQUAL:
  case T_GREATER:
  case T_GREATER_EQUAL:
  case T_AND_AND:
  case T_OR_OR:
  case T_COMMA:
  case T_AMP:
  case T_PLUS_EQUAL:
  case T_MINUS_EQUAL:
  case T_STAR_EQUAL:
  case T_SLASH_EQUAL:
  case T_PERCENT_EQUAL:
  case T_CARET_EQUAL:
    return true;
  default:
    return false;
  }
}
static void consume_statement_semicolon(Parser *p, const char *message) {
  if (had_error)
    return;
  if (match(p, T_SEMI))
    return;
  Token *prev = p->pos ? &p->tokens[p->pos - 1] : current(p);
  Token *next = current(p);
  if (asi_enabled && asi_can_end_statement(prev->type)) {
    /* ECMAScript-like restricted insertion points: EOF and '}' are safe. */
    if (next->type == T_EOF || next->type == T_RBRACE)
      return;
    if (prev->line < next->line) {
      if (asi_continuation_token(next->type)) {
        fprintf(
            stderr,
            "Syntax error on line %d, column %d: this line looks like it "
            "continues the previous line. If you meant to write two separate "
            "statements, put a semicolon (;) at the end of the first one.\n\n",
            next->line, next->col);
        had_error = 1;
        return;
      }
      return;
    }
    /* ASI mode also accepts an adjacent, clearly-starting statement on the
     * same line. This avoids a misleading 'Expected semicolon' diagnostic
     * for code that intentionally relies on automatic insertion. */
    if (asi_enabled && (next->type == T_EOF || next->type == T_RBRACE ||
                        next->type == T_IDENT))
      return;
  }
  fprintf(stderr, "Syntax error on line %d, column %d: %s\nWhat to try: %s\n\n",
          next->line, next->col, message, beginner_tip_for_error(message));
  had_error = 1;
}

static Node *parse_expression(Parser *p) { return parse_assignment(p); }

static Node *parse_block_after_brace(Parser *p, int line) {
  Node *n = new_node(N_BLOCK, line);
  n->as.block.items = NULL;
  n->as.block.count = 0;
  while (!check(p, T_RBRACE) && !check(p, T_EOF) && !had_error) {
    Node *item = parse_declaration(p);
    n->as.block.items = (Node **)xrealloc(
        n->as.block.items, (size_t)(n->as.block.count + 1) * sizeof(Node *));
    n->as.block.items[n->as.block.count++] = item;
  }
  consume(p, T_RBRACE, "Expected '}' after block.");
  return n;
}
static Node *parse_block(Parser *p) {
  Token *brace = consume(p, T_LBRACE, "Expected '{'.");
  return parse_block_after_brace(p, brace->line);
}

static Node *parse_declaration(Parser *p) {
  if (check(p, T_IDENT) && p->pos + 1 < p->count &&
      p->tokens[p->pos + 1].type == T_IDENT) {
    Token *type = current(p);
    bool reserved =
        strcmp(type->lexeme, "typedef") == 0 ||
        strcmp(type->lexeme, "struct") == 0 ||
        strcmp(type->lexeme, "class") == 0 || strcmp(type->lexeme, "fn") == 0 ||
        strcmp(type->lexeme, "namespace") == 0 ||
        strcmp(type->lexeme, "let") == 0 || strcmp(type->lexeme, "if") == 0 ||
        strcmp(type->lexeme, "else") == 0 ||
        strcmp(type->lexeme, "while") == 0 ||
        strcmp(type->lexeme, "for") == 0 ||
        strcmp(type->lexeme, "return") == 0 ||
        strcmp(type->lexeme, "throw") == 0 ||
        strcmp(type->lexeme, "import") == 0 ||
        strcmp(type->lexeme, "thread") == 0 ||
        strcmp(type->lexeme, "fluorine") == 0;
    if (!reserved) {
      p->pos++;
      Token *name =
          consume(p, T_IDENT, "Expected a variable name after the type name.");
      if (strcmp(name->lexeme, "let") == 0 ||
          strcmp(name->lexeme, "typedef") == 0)
        syntax_error(name->line,
                     "Reserved keyword cannot be used as a variable name.");
      Node *n = new_node(N_LET, type->line);
      n->as.let_stmt.name = copy_s(name->lexeme);
      n->as.let_stmt.init = NULL;
      if (match(p, T_EQUAL))
        n->as.let_stmt.init = parse_expression(p);
      consume_statement_semicolon(
          p, "Expected a semicolon after typed variable declaration.");
      return n;
    }
  }
  if (match_kw(p, "typedef")) {
    Token *kw = previous(p);
    /* Support both `typedef Existing Alias;` and C-like
     * `typedef struct Existing Alias;` / `typedef class Existing Alias;`. */
    if (match_kw(p, "struct") ||
        match_kw(p, "class")) { /* optional type-kind marker */
    }
    Token *target =
        consume(p, T_IDENT,
                "Expected an existing class or struct name after 'typedef'.");
    Token *name = consume(p, T_IDENT, "Expected the new type alias name.");
    consume_statement_semicolon(p, "Expected ';' after typedef declaration.");
    Node *n = new_node(N_TYPEDEF, kw->line);
    n->as.typedef_stmt.target = copy_s(target->lexeme);
    n->as.typedef_stmt.name = copy_s(name->lexeme);
    return n;
  }
  if (match_kw(p, "let")) {
    Token *kw = previous(p);
    Token *name = consume(p, T_IDENT, "Expected variable name after 'let'.");
    if (strcmp(name->lexeme, "let") == 0 ||
        strcmp(name->lexeme, "typedef") == 0)
      syntax_error(name->line,
                   "Reserved keyword cannot be used as a variable name.");
    Node *n = new_node(N_LET, kw->line);
    n->as.let_stmt.name = copy_s(name->lexeme);
    n->as.let_stmt.init = NULL;
    if (match(p, T_EQUAL))
      n->as.let_stmt.init = parse_expression(p);
    consume_statement_semicolon(
        p, "Expected a semicolon after variable declaration.");
    return n;
  }
  if (match_kw(p, "class")) {
    Token *kw = previous(p);
    Token *name = consume(p, T_IDENT, "Expected class name.");
    Node *n = new_node(N_CLASS, kw->line);
    n->as.class_stmt.name = copy_s(name->lexeme);
    n->as.class_stmt.parent = NULL;
    if (match_kw(p, "extends")) {
      Token *parent =
          consume(p, T_IDENT, "Expected parent class name after 'extends'.");
      n->as.class_stmt.parent = copy_s(parent->lexeme);
    }
    n->as.class_stmt.body = parse_block(p);
    return n;
  }
  if (match_kw(p, "namespace")) {
    Token *kw = previous(p);
    Token *name = consume(p, T_IDENT, "Expected namespace name.");
    Node *n = new_node(N_NAMESPACE, kw->line);
    n->as.namespace_stmt.name = copy_s(name->lexeme);
    n->as.namespace_stmt.body = parse_block(p);
    return n;
  }
  if (match_kw(p, "struct")) {
    Token *kw = previous(p);
    Token *name = consume(p, T_IDENT, "Expected struct name.");
    Node *n = new_node(N_STRUCT, kw->line);
    n->as.struct_stmt.name = copy_s(name->lexeme);
    n->as.struct_stmt.body = parse_block(p);
    return n;
  }
  if (match_kw(p, "fn")) {
    Token *kw = previous(p);
    Token *name = consume(p, T_IDENT, "Expected function name.");
    Node *n = new_node(N_FUNCTION, kw->line);
    n->as.function.name = copy_s(name->lexeme);
    n->as.function.params = NULL;
    n->as.function.param_count = 0;
    consume(p, T_LPAREN, "Expected '(' after function name.");
    if (!check(p, T_RPAREN)) {
      do {
        Token *param = consume(p, T_IDENT, "Expected parameter name.");
        if (strcmp(param->lexeme, "let") == 0 ||
            strcmp(param->lexeme, "typedef") == 0)
          syntax_error(param->line,
                       "Reserved keyword cannot be used as a variable name.");
        for (int i = 0; i < n->as.function.param_count; i++)
          if (strcmp(n->as.function.params[i], param->lexeme) == 0)
            syntax_error(param->line, "Duplicate parameter '%s'.",
                         param->lexeme);
        n->as.function.params = (char **)xrealloc(
            n->as.function.params,
            (size_t)(n->as.function.param_count + 1) * sizeof(char *));
        n->as.function.params[n->as.function.param_count++] =
            copy_s(param->lexeme);
        if (n->as.function.param_count > MAX_ARGS)
          syntax_error(param->line, "Too many function parameters.");
      } while (match(p, T_COMMA));
    }
    consume(p, T_RPAREN, "Expected ')' after parameters.");
    n->as.function.doc = NULL;
    n->as.function.body = parse_block(p);
    /* Python-style docstring: the first standalone string in a function body.
     */
    if (n->as.function.body && n->as.function.body->type == N_BLOCK &&
        n->as.function.body->as.block.count > 0) {
      Node *first = n->as.function.body->as.block.items[0];
      if (first && first->type == N_EXPR_STMT && first->as.expr_stmt.expr &&
          first->as.expr_stmt.expr->type == N_STRING) {
        n->as.function.doc = copy_s(first->as.expr_stmt.expr->as.string);
        free_node(first);
        for (int di = 1; di < n->as.function.body->as.block.count; di++)
          n->as.function.body->as.block.items[di - 1] =
              n->as.function.body->as.block.items[di];
        n->as.function.body->as.block.count--;
      }
    }
    return n;
  }
  return parse_statement(p);
}
static Node *parse_statement(Parser *p) {
  if (match_kw(p, "switch")) {
    Token *kw = previous(p);
    Node *n = new_node(N_SWITCH, kw->line);
    n->as.switch_stmt.expr = NULL;
    n->as.switch_stmt.values = NULL;
    n->as.switch_stmt.bodies = NULL;
    n->as.switch_stmt.counts = NULL;
    n->as.switch_stmt.defaults = NULL;
    n->as.switch_stmt.count = 0;
    consume(p, T_LPAREN, "Expected '(' after switch.");
    n->as.switch_stmt.expr = parse_expression(p);
    consume(p, T_RPAREN, "Expected ')' after switch expression.");
    consume(p, T_LBRACE, "Expected '{' before switch cases.");
    bool saw_default = false;
    while (!check(p, T_RBRACE) && !check(p, T_EOF) && !had_error) {
      bool is_default = false;
      Node *case_value = NULL;
      if (match_kw(p, "case")) {
        case_value = parse_expression(p);
        consume(p, T_COLON, "Expected ':' after case value.");
      } else if (match_kw(p, "default")) {
        is_default = true;
        if (saw_default)
          syntax_error(previous(p)->line,
                       "A switch statement may contain only one default case.");
        saw_default = true;
        consume(p, T_COLON, "Expected ':' after default.");
      } else {
        syntax_error(current(p)->line,
                     "Expected 'case', 'default', or '}' in switch body.");
        break;
      }
      int i = n->as.switch_stmt.count++;
      n->as.switch_stmt.values =
          (Node **)xrealloc(n->as.switch_stmt.values,
                            (size_t)n->as.switch_stmt.count * sizeof(Node *));
      n->as.switch_stmt.bodies =
          (Node ***)xrealloc(n->as.switch_stmt.bodies,
                             (size_t)n->as.switch_stmt.count * sizeof(Node **));
      n->as.switch_stmt.counts =
          (int *)xrealloc(n->as.switch_stmt.counts,
                          (size_t)n->as.switch_stmt.count * sizeof(int));
      n->as.switch_stmt.defaults =
          (bool *)xrealloc(n->as.switch_stmt.defaults,
                           (size_t)n->as.switch_stmt.count * sizeof(bool));
      n->as.switch_stmt.values[i] = case_value;
      n->as.switch_stmt.defaults[i] = is_default;
      n->as.switch_stmt.bodies[i] = NULL;
      n->as.switch_stmt.counts[i] = 0;
      while (!check(p, T_RBRACE) && !check(p, T_EOF) && !keyword(p, "case") &&
             !keyword(p, "default") && !had_error) {
        Node *stmt = parse_declaration(p);
        n->as.switch_stmt.bodies[i] = (Node **)xrealloc(
            n->as.switch_stmt.bodies[i],
            (size_t)(n->as.switch_stmt.counts[i] + 1) * sizeof(Node *));
        n->as.switch_stmt.bodies[i][n->as.switch_stmt.counts[i]++] = stmt;
      }
    }
    consume(p, T_RBRACE, "Expected '}' after switch body.");
    return n;
  }
  if (match_kw(p, "fluorine")) {
    Token *kw = previous(p);
    Node *call = parse_expression(p);
    if (call->type != N_CALL) {
      syntax_error(kw->line, "Expected a function call after 'fluorine'.");
      free_node(call);
      return new_node(N_FLUORINE, kw->line);
    }
    consume_statement_semicolon(
        p, "Expected a semicolon after green-thread launch.");
    Node *n = new_node(N_FLUORINE, kw->line);
    n->as.fluorine_stmt.call = call;
    return n;
  }
  if (match_kw(p, "thread")) {
    Token *kw = previous(p);
    Node *call = parse_expression(p);
    if (call->type != N_CALL) {
      syntax_error(kw->line, "Expected a function call after 'thread'.");
      free_node(call);
      return new_node(N_THREAD, kw->line);
    }
    consume_statement_semicolon(p,
                                "Expected a semicolon after OS-thread launch.");
    Node *n = new_node(N_THREAD, kw->line);
    n->as.fluorine_stmt.call = call;
    return n;
  }
  if (match_kw(p, "import")) {
    Token *kw = previous(p);
    Token *path = NULL;
    if (check(p, T_STRING) || check(p, T_IDENT)) {
      path = current(p);
      p->pos++;
    } else {
      syntax_error(
          current(p)->line,
          "Expected a module name or quoted module path after import.");
      return new_node(N_IMPORT, kw->line);
    }
    /* Quoted paths search the working directory first, then stdlib.
     * Unquoted module names are deliberately reserved for stdlib lookup. */
    Node *n = new_node(N_IMPORT, kw->line);
    n->as.import_stmt.path = copy_s(path->lexeme);
    n->as.import_stmt.alias = NULL;
    n->as.import_stmt.quoted = (path->type == T_STRING);
    if (match_kw(p, "as")) {
      Token *alias = consume(p, T_IDENT, "Expected module alias after 'as'.");
      n->as.import_stmt.alias = copy_s(alias->lexeme);
    }
    consume_statement_semicolon(p, "Expected a semicolon after import.");
    return n;
  }
  if (match_kw(p, "throw")) {
    Token *kw = previous(p);
    Node *n = new_node(N_THROW, kw->line);
    if (asi_enabled && current(p)->line > kw->line) {
      fprintf(stderr,
              "Syntax error, line %d, col %d: a line break is not allowed "
              "immediately after 'throw'; write the thrown expression on the "
              "same line.\n",
              current(p)->line, current(p)->col);
      had_error = 1;
      n->as.throw_stmt.value = new_node(N_NULL, kw->line);
      return n;
    }
    n->as.throw_stmt.value = parse_expression(p);
    consume_statement_semicolon(p, "Expected a semicolon after throw.");
    return n;
  }
  if (match_kw(p, "try")) {
    Token *kw = previous(p);
    Node *n = new_node(N_TRY, kw->line);
    n->as.try_stmt.body = parse_block(p);
    n->as.try_stmt.name = NULL;
    n->as.try_stmt.handler = NULL;
    n->as.try_stmt.finally_body = NULL;
    if (match_kw(p, "catch")) {
      consume(p, T_LPAREN, "Expected '(' after catch.");
      Token *name = consume(p, T_IDENT, "Expected exception variable name.");
      n->as.try_stmt.name = copy_s(name->lexeme);
      consume(p, T_RPAREN, "Expected ')' after catch variable.");
      n->as.try_stmt.handler = parse_block(p);
    }
    if (match_kw(p, "finally"))
      n->as.try_stmt.finally_body = parse_block(p);
    if (!n->as.try_stmt.handler && !n->as.try_stmt.finally_body)
      syntax_error(kw->line, "Expected catch or finally after try block.");
    return n;
  }
  if (match_kw(p, "if")) {
    Token *kw = previous(p);
    Node *n = new_node(N_IF, kw->line);
    consume(p, T_LPAREN, "Expected '(' after 'if'.");
    n->as.if_stmt.condition = parse_expression(p);
    consume(p, T_RPAREN, "Expected ')' after condition.");
    n->as.if_stmt.then_branch = parse_statement(p);
    n->as.if_stmt.else_branch = NULL;
    if (match_kw(p, "else")) {
      if (keyword(p, "if"))
        n->as.if_stmt.else_branch = parse_statement(p);
      else
        n->as.if_stmt.else_branch = parse_statement(p);
    }
    return n;
  }
  if (match_kw(p, "do")) {
    Token *kw = previous(p);
    Node *n = new_node(N_DO_WHILE, kw->line);
    n->as.while_stmt.body = parse_statement(p);
    consume(p, T_IDENT, "Expected 'while' after do body.");
    if (strcmp(previous(p)->lexeme, "while") != 0)
      syntax_error(kw->line, "Expected 'while' after do body.");
    consume(p, T_LPAREN, "Expected '(' after do-while.");
    n->as.while_stmt.condition = parse_expression(p);
    consume(p, T_RPAREN, "Expected ')' after do-while condition.");
    consume_statement_semicolon(p, "Expected a semicolon after do-while.");
    return n;
  }
  if (match_kw(p, "while")) {
    Token *kw = previous(p);
    Node *n = new_node(N_WHILE, kw->line);
    consume(p, T_LPAREN, "Expected '(' after 'while'.");
    n->as.while_stmt.condition = parse_expression(p);
    consume(p, T_RPAREN, "Expected ')' after condition.");
    n->as.while_stmt.body = parse_statement(p);
    return n;
  }
  if (match_kw(p, "for")) {
    Token *kw = previous(p);
    Node *n = new_node(N_FOR, kw->line);
    consume(p, T_LPAREN, "Expected '(' after 'for'.");
    if (match(p, T_SEMI))
      n->as.for_stmt.init = NULL;
    else if (match_kw(p, "let") ||
             (check(p, T_IDENT) && strcmp(current(p)->lexeme, "int") == 0 &&
              p->pos + 1 < p->count && p->tokens[p->pos + 1].type == T_IDENT) ||
             (check(p, T_IDENT) && p->pos + 1 < p->count &&
              p->tokens[p->pos + 1].type == T_IDENT &&
              strcmp(p->tokens[p->pos + 1].lexeme, "in") == 0)) {
      /* C-style for headers commonly use `int i = 0`; accept this narrow
         form without treating `int` as a general Fluorine data type. */
      if (strcmp(previous(p)->lexeme, "let") != 0 && check(p, T_IDENT) &&
          strcmp(current(p)->lexeme, "int") == 0)
        p->pos++;
      Token *name =
          consume(p, T_IDENT, "Expected variable name in for initializer.");
      if (strcmp(name->lexeme, "let") == 0 ||
          strcmp(name->lexeme, "typedef") == 0)
        syntax_error(name->line,
                     "Reserved keyword cannot be used as a variable name.");
      if (match_kw(p, "in")) {
        Node *each = new_node(N_FOR_IN, kw->line);
        each->as.for_in.name = copy_s(name->lexeme);
        each->as.for_in.iterable = parse_expression(p);
        consume(p, T_RPAREN, "Expected ')' after for-in iterable.");
        each->as.for_in.body = parse_statement(p);
        free(n);
        return each;
      }
      Node *decl = new_node(N_LET, name->line);
      decl->as.let_stmt.name = copy_s(name->lexeme);
      decl->as.let_stmt.init = NULL;
      if (match(p, T_EQUAL))
        decl->as.let_stmt.init = parse_expression(p);
      consume(p, T_SEMI, "Expected ';' after for initializer.");
      n->as.for_stmt.init = decl;
    } else {
      Node *e = parse_expression(p);
      consume(p, T_SEMI, "Expected ';' after for initializer.");
      Node *s = new_node(N_EXPR_STMT, e->line);
      s->as.expr_stmt.expr = e;
      n->as.for_stmt.init = s;
    }
    n->as.for_stmt.condition = check(p, T_SEMI) ? NULL : parse_expression(p);
    consume(p, T_SEMI, "Expected ';' after for condition.");
    n->as.for_stmt.increment = check(p, T_RPAREN) ? NULL : parse_expression(p);
    consume(p, T_RPAREN, "Expected ')' after for clauses.");
    n->as.for_stmt.body = parse_statement(p);
    return n;
  }
  if (match_kw(p, "return")) {
    Token *kw = previous(p);
    Node *n = new_node(N_RETURN, kw->line);
    n->as.return_stmt.value =
        (check(p, T_SEMI) || check(p, T_RBRACE) || check(p, T_EOF) ||
         (asi_enabled && current(p)->line > kw->line))
            ? NULL
            : parse_expression(p);
    consume_statement_semicolon(p, "Expected a semicolon after return.");
    return n;
  }
  if (match_kw(p, "break")) {
    Token *kw = previous(p);
    Node *n = new_node(N_BREAK, kw->line);
    consume_statement_semicolon(p, "Expected a semicolon after break.");
    return n;
  }
  if (match_kw(p, "continue")) {
    Token *kw = previous(p);
    Node *n = new_node(N_CONTINUE, kw->line);
    consume_statement_semicolon(p, "Expected a semicolon after continue.");
    return n;
  }
  if (match_kw(p, "print")) {
    Token *kw = previous(p);
    Node *n = new_node(N_PRINT, kw->line);
    n->as.print_stmt.args = NULL;
    n->as.print_stmt.count = 0;
    consume(p, T_LPAREN, "Expected '(' after print.");
    if (!check(p, T_RPAREN)) {
      do {
        n->as.print_stmt.args = (Node **)xrealloc(
            n->as.print_stmt.args,
            (size_t)(n->as.print_stmt.count + 1) * sizeof(Node *));
        n->as.print_stmt.args[n->as.print_stmt.count++] = parse_expression(p);
        if (n->as.print_stmt.count > MAX_ARGS)
          syntax_error(kw->line, "Too many print arguments.");
      } while (match(p, T_COMMA));
    }
    consume(p, T_RPAREN, "Expected ')' after print arguments.");
    consume_statement_semicolon(p, "Expected a semicolon after print.");
    return n;
  }
  if (match(p, T_LBRACE))
    return parse_block_after_brace(p, previous(p)->line);
  Node *e = parse_expression(p);
  consume_statement_semicolon(p, "Expected a semicolon after expression.");
  Node *n = new_node(N_EXPR_STMT, e->line);
  n->as.expr_stmt.expr = e;
  return n;
}

/* Arbitrary-precision signed integers use canonical decimal strings. Operations
   allocate their results dynamically and check size arithmetic before
   allocation. */
static char *bi_norm(const char *s) {
  if (!s || !*s)
    return copy_s("0");
  bool neg = *s == '-';
  if (*s == '-' || *s == '+')
    s++;
  while (*s == '0')
    s++;
  if (!*s)
    return copy_s("0");
  size_t n = strlen(s);
  if (n > SIZE_MAX - (neg ? 2u : 1u)) {
    fprintf(stderr, "Integer is too large to represent in memory.\n");
    exit(2);
  }
  char *r = (char *)malloc(n + (neg ? 2 : 1));
  if (!r) {
    fprintf(stderr, "Out of memory while storing integer.\n");
    exit(2);
  }
  size_t off = 0;
  if (neg)
    r[off++] = '-';
  memcpy(r + off, s, n + 1);
  return r;
}
static int bi_sign(const char *s) {
  return s[0] == '-' ? -1 : (strcmp(s, "0") == 0 ? 0 : 1);
}
static const char *bi_mag(const char *s) { return s[0] == '-' ? s + 1 : s; }
static int bi_cmpmag(const char *a, const char *b) {
  size_t x = strlen(a), y = strlen(b);
  if (x != y)
    return x < y ? -1 : 1;
  int c = strcmp(a, b);
  return (c > 0) - (c < 0);
}
static int bi_cmp(const char *a, const char *b) {
  int sa = bi_sign(a), sb = bi_sign(b);
  if (sa != sb)
    return sa < sb ? -1 : 1;
  if (!sa)
    return 0;
  int c = bi_cmpmag(bi_mag(a), bi_mag(b));
  return sa < 0 ? -c : c;
}
static char *bi_addmag(const char *a, const char *b) {
  size_t x = strlen(a), y = strlen(b), n = (x > y ? x : y);
  if (n > SIZE_MAX - 2) {
    fprintf(stderr, "Integer too large.\n");
    exit(2);
  }
  char *r = (char *)malloc(n + 2);
  if (!r)
    exit(2);
  r[n + 1] = 0;
  int carry = 0;
  for (size_t k = 0; k < n; k++) {
    int da = k < x ? a[x - 1 - k] - '0' : 0,
        db = k < y ? b[y - 1 - k] - '0' : 0, z = da + db + carry;
    r[n - k] = (char)('0' + z % 10);
    carry = z / 10;
  }
  r[0] = (char)('0' + carry);
  return bi_norm(r);
}
/* |a| >= |b| */
static char *bi_submag(const char *a, const char *b) {
  size_t x = strlen(a), y = strlen(b);
  char *r = (char *)malloc(x + 1);
  if (!r)
    exit(2);
  r[x] = 0;
  int borrow = 0;
  for (size_t k = 0; k < x; k++) {
    int da = a[x - 1 - k] - '0', db = k < y ? b[y - 1 - k] - '0' : 0,
        z = da - db - borrow;
    if (z < 0) {
      z += 10;
      borrow = 1;
    } else
      borrow = 0;
    r[x - 1 - k] = (char)('0' + z);
  }
  char *q = bi_norm(r);
  free(r);
  return q;
}
static char *bi_add(const char *a, const char *b) {
  int sa = bi_sign(a), sb = bi_sign(b);
  if (!sa)
    return copy_s(b);
  if (!sb)
    return copy_s(a);
  if (sa == sb) {
    char *m = bi_addmag(bi_mag(a), bi_mag(b));
    char *r;
    if (sa < 0) {
      size_t n = strlen(m);
      r = (char *)malloc(n + 2);
      if (!r)
        exit(2);
      r[0] = '-';
      memcpy(r + 1, m, n + 1);
      free(m);
    } else
      r = m;
    return r;
  }
  int c = bi_cmpmag(bi_mag(a), bi_mag(b));
  if (!c)
    return copy_s("0");
  const char *big = c > 0 ? bi_mag(a) : bi_mag(b);
  const char *small = c > 0 ? bi_mag(b) : bi_mag(a);
  char *m = bi_submag(big, small);
  int sign = c > 0 ? sa : sb;
  if (sign > 0)
    return m;
  size_t n = strlen(m);
  char *r = (char *)malloc(n + 2);
  if (!r)
    exit(2);
  r[0] = '-';
  memcpy(r + 1, m, n + 1);
  free(m);
  return r;
}
static char *bi_neg(const char *a) {
  if (!strcmp(a, "0"))
    return copy_s("0");
  if (a[0] == '-')
    return copy_s(a + 1);
  size_t n = strlen(a);
  char *r = (char *)malloc(n + 2);
  if (!r)
    exit(2);
  r[0] = '-';
  memcpy(r + 1, a, n + 1);
  return r;
}
static char *bi_mul(const char *a, const char *b) {
  const char *x = bi_mag(a), *y = bi_mag(b);
  size_t nx = strlen(x), ny = strlen(y);
  if (nx > SIZE_MAX - ny - 2) {
    fprintf(stderr, "Integer multiplication size overflow.\n");
    exit(2);
  }
  size_t n = nx + ny;
  unsigned int *d = (unsigned int *)calloc(n, sizeof(unsigned int));
  if (!d)
    exit(2);
  for (size_t i = nx; i-- > 0;) {
    unsigned int carry = 0;
    for (size_t j = ny; j-- > 0;) {
      size_t pos = i + j + 1;
      unsigned int z =
          d[pos] + (unsigned int)((x[i] - '0') * (y[j] - '0')) + carry;
      d[pos] = z % 10;
      carry = z / 10;
    }
    d[i] += carry;
  }
  char *r = (char *)malloc(n + 2);
  if (!r) {
    free(d);
    exit(2);
  }
  size_t off = 0;
  if (bi_sign(a) * bi_sign(b) < 0)
    r[off++] = '-';
  size_t i = 0;
  while (i + 1 < n && d[i] == 0)
    i++;
  for (; i < n; i++)
    r[off++] = (char)('0' + d[i]);
  r[off] = 0;
  free(d);
  char *q = bi_norm(r);
  free(r);
  return q;
}
static char *bi_divmod_mag(const char *a, const char *b, char **remout) {
  if (!strcmp(b, "0")) {
    *remout = NULL;
    return NULL;
  }
  size_t n = strlen(a);
  char *q = (char *)malloc(n + 1);
  if (!q)
    exit(2);
  q[n] = 0;
  char *rem = copy_s("0");
  for (size_t i = 0; i < n; i++) {
    size_t rn = strlen(rem);
    char *t = (char *)malloc(rn + 2);
    if (!t)
      exit(2);
    memcpy(t, rem, rn);
    t[rn] = a[i];
    t[rn + 1] = 0;
    free(rem);
    rem = bi_norm(t);
    free(t);
    int digit = 0;
    while (bi_cmpmag(rem, b) >= 0) {
      char *z = bi_submag(rem, b);
      free(rem);
      rem = z;
      if (++digit > 9)
        break;
    }
    q[i] = (char)('0' + digit);
  }
  char *z = bi_norm(q);
  free(q);
  *remout = rem;
  return z;
}
static char *bi_div(const char *a, const char *b, bool remainder) {
  if (!strcmp(b, "0"))
    return NULL;
  if (!remainder && !strcmp(b, "1"))
    return copy_s(a);
  if (!remainder && !strcmp(b, "-1"))
    return bi_neg(a);
  if (!remainder && !strcmp(a, b))
    return copy_s("1");
  if (remainder && (!strcmp(b, "1") || !strcmp(b, "-1")))
    return copy_s("0");
  char *r = NULL;
  char *q = bi_divmod_mag(bi_mag(a), bi_mag(b), &r);
  if (remainder) {
    free(q);
    q = r;
    r = NULL;
    if (bi_sign(a) < 0 && strcmp(q, "0")) {
      size_t n = strlen(q);
      char *z = (char *)malloc(n + 2);
      if (!z)
        exit(2);
      z[0] = '-';
      memcpy(z + 1, q, n + 1);
      free(q);
      q = z;
    }
  } else
    free(r);
  if (!remainder && bi_sign(a) * bi_sign(b) < 0 && strcmp(q, "0")) {
    size_t n = strlen(q);
    char *z = (char *)malloc(n + 2);
    if (!z)
      exit(2);
    z[0] = '-';
    memcpy(z + 1, q, n + 1);
    free(q);
    q = z;
  }
  return q;
}
static double bi_double(const char *s) { return strtod(s, NULL); }

static char *bi_gcd(const char *left, const char *right) {
  if (!strcmp(bi_mag(left), "1") || !strcmp(bi_mag(right), "1"))
    return copy_s("1");
  char *a = bi_norm(bi_mag(left)), *b = bi_norm(bi_mag(right));
  while (strcmp(b, "0") != 0) {
    char *r = bi_div(a, b, true);
    free(a);
    a = b;
    b = bi_norm(r);
    free(r);
  }
  free(b);
  return a;
}
static char *bi_div_small(const char *number, unsigned int divisor,
                          unsigned int *remainder) {
  size_t n = strlen(number);
  char *q = (char *)malloc(n + 1);
  if (!q) {
    fprintf(stderr, "Out of memory while dividing integer.\n");
    exit(2);
  }
  unsigned long carry = 0;
  for (size_t i = 0; i < n; i++) {
    unsigned long part = carry * 10UL + (unsigned long)(number[i] - '0');
    q[i] = (char)('0' + part / divisor);
    carry = part % divisor;
  }
  q[n] = '\0';
  char *normalized = bi_norm(q);
  free(q);
  if (remainder)
    *remainder = (unsigned int)carry;
  return normalized;
}
static Value v_null(void) {
  Value v;
  v.type = V_NULL;
  return v;
}
static Value v_num(double x) {
  Value v;
  v.type = V_NUMBER;
  v.as.number = x;
  return v;
}
static Value v_bigint(const char *s) {
  Value v;
  v.type = V_BIGINT;
  v.as.bigint = bi_norm(s);
  return v;
}
static Value v_ratio(const char *numerator, const char *denominator) {
  char *n = bi_norm(numerator), *d = bi_norm(denominator);
  if (!strcmp(d, "0")) {
    free(n);
    free(d);
    Value bad = v_null();
    return bad;
  }
  if (d[0] == '-') {
    char *nn = bi_neg(n), *dd = bi_neg(d);
    free(n);
    free(d);
    n = nn;
    d = dd;
  }
  if (!strcmp(n, "0")) {
    free(d);
    d = copy_s("1");
  }
  if (!strcmp(d, "1")) {
    Value v;
    v.type = V_RATIONAL;
    v.as.rational.num = n;
    v.as.rational.den = d;
    return v;
  } else {
    char *g = bi_gcd(n, d);
    char *nn = bi_div(n, g, false), *dd = bi_div(d, g, false);
    free(n);
    free(d);
    free(g);
    n = nn;
    d = dd;
  }
  Value v;
  v.type = V_RATIONAL;
  v.as.rational.num = n;
  v.as.rational.den = d;
  return v;
}
static bool is_exact_numeric(Value v) {
  return v.type == V_BIGINT || v.type == V_RATIONAL;
}
static bool is_numeric(Value v) {
  return v.type == V_NUMBER || is_exact_numeric(v);
}

static const char *value_type_name(Value v) {
  switch (v.type) {
  case V_NULL:
    return "null";
  case V_NUMBER:
  case V_BIGINT:
  case V_RATIONAL:
    return "a number";
  case V_STRING:
    return "text (string)";
  case V_BOOL:
    return "true/false (boolean)";
  case V_FUNCTION:
    return "a function";
  case V_NATIVE:
    return "a built-in function";
  case V_ARRAY:
    return "an array (list)";
  case V_MAP:
    return "an object/map";
  case V_CLASS:
    return "a class";
  case V_STRUCT_DEF:
    return "a struct type";
  case V_POINTER:
    return "a pointer/reference";
  case V_THREAD:
    return "a thread";
  }
  return "a fluorine value";
}

static void exact_components(Value v, char **numerator, char **denominator) {
  if (v.type == V_BIGINT) {
    *numerator = copy_s(v.as.bigint);
    *denominator = copy_s("1");
  } else if (v.type == V_RATIONAL) {
    *numerator = copy_s(v.as.rational.num);
    *denominator = copy_s(v.as.rational.den);
  } else {
    *numerator = copy_s("0");
    *denominator = copy_s("1");
  }
}
static double numeric_double(Value v) {
  if (v.type == V_BIGINT)
    return bi_double(v.as.bigint);
  if (v.type == V_RATIONAL)
    return bi_double(v.as.rational.num) / bi_double(v.as.rational.den);
  return v.as.number;
}
static int numeric_cmp(Value a, Value b) {
  if (is_exact_numeric(a) && is_exact_numeric(b)) {
    char *an, *ad, *bn, *bd;
    exact_components(a, &an, &ad);
    exact_components(b, &bn, &bd);
    char *left = bi_mul(an, bd), *right = bi_mul(bn, ad);
    int c = bi_cmp(left, right);
    free(an);
    free(ad);
    free(bn);
    free(bd);
    free(left);
    free(right);
    return c;
  }
  double x = numeric_double(a), y = numeric_double(b);
  return (x > y) - (x < y);
}
static Value exact_power(Value base, Value exponent, int line);
static Value numeric_binary(FluorineToken op, Value a, Value b, int line) {
  if (!is_numeric(a) || !is_numeric(b)) {
    runtime_error(
        line, "This math operation needs two numbers, but received %s and %s.",
        value_type_name(a), value_type_name(b));
    return v_null();
  }
  if (is_exact_numeric(a) && is_exact_numeric(b) && op == T_CARET) {
    char *en, *ed;
    exact_components(b, &en, &ed);
    bool integer_exponent = strcmp(ed, "1") == 0;
    free(en);
    free(ed);
    if (integer_exponent)
      return exact_power(a, b, line);
  }
  if (is_exact_numeric(a) && is_exact_numeric(b) &&
      (op == T_PLUS || op == T_MINUS || op == T_STAR || op == T_SLASH ||
       op == T_PERCENT)) {
    char *an, *ad, *bn, *bd;
    exact_components(a, &an, &ad);
    exact_components(b, &bn, &bd);
    if ((op == T_SLASH || op == T_PERCENT) && !strcmp(bn, "0")) {
      runtime_error(line,
                    op == T_SLASH ? "Division by zero." : "Remainder by zero.");
      free(an);
      free(ad);
      free(bn);
      free(bd);
      return v_null();
    }
    if (op == T_PLUS || op == T_MINUS) {
      char *left = bi_mul(an, bd), *right = bi_mul(bn, ad);
      if (op == T_MINUS) {
        char *neg = bi_neg(right);
        free(right);
        right = neg;
      }
      char *num = bi_add(left, right), *den = bi_mul(ad, bd);
      Value result;
      if (a.type == V_BIGINT && b.type == V_BIGINT && op == T_PLUS)
        result = v_bigint(num);
      else if (a.type == V_BIGINT && b.type == V_BIGINT && op == T_MINUS)
        result = v_bigint(num);
      else
        result = v_ratio(num, den);
      free(left);
      free(right);
      free(num);
      free(den);
      free(an);
      free(ad);
      free(bn);
      free(bd);
      return result;
    }
    if (op == T_STAR) {
      char *num = bi_mul(an, bn), *den = bi_mul(ad, bd);
      Value result = (a.type == V_BIGINT && b.type == V_BIGINT)
                         ? v_bigint(num)
                         : v_ratio(num, den);
      free(num);
      free(den);
      free(an);
      free(ad);
      free(bn);
      free(bd);
      return result;
    }
    if (op == T_SLASH) {
      char *num = bi_mul(an, bd), *den = bi_mul(ad, bn);
      Value result;
      if (a.type == V_BIGINT && b.type == V_BIGINT) {
        char *rem = bi_div(a.as.bigint, b.as.bigint, true);
        if (!strcmp(rem, "0")) {
          char *q = bi_div(a.as.bigint, b.as.bigint, false);
          result = v_bigint(q);
          free(q);
        } else
          result = v_ratio(num, den);
        free(rem);
      } else
        result = v_ratio(num, den);
      free(num);
      free(den);
      free(an);
      free(ad);
      free(bn);
      free(bd);
      return result;
    }
    /* Rational remainder uses truncation toward zero, matching C fmod. */
    char *qnum = bi_mul(an, bd), *qden = bi_mul(ad, bn);
    if (qden[0] == '-') {
      char *neg = bi_neg(qnum), *pos = bi_neg(qden);
      free(qnum);
      free(qden);
      qnum = neg;
      qden = pos;
    }
    char *q = bi_div(qnum, qden, false);
    char *qb = bi_mul(q, bn), *qbd = bi_mul(qb, ad);
    char *scaled_a = bi_mul(an, bd), *neg_qbd = bi_neg(qbd),
         *num = bi_add(scaled_a, neg_qbd);
    char *den = bi_mul(ad, bd);
    Value result = v_ratio(num, den);
    free(an);
    free(ad);
    free(bn);
    free(bd);
    free(qnum);
    free(qden);
    free(q);
    free(qb);
    free(qbd);
    free(scaled_a);
    free(neg_qbd);
    free(num);
    free(den);
    return result;
  }
  double x = numeric_double(a), y = numeric_double(b);
  if (op == T_SLASH && y == 0) {
    runtime_error(line, "Division by zero.");
    return v_null();
  }
  if (op == T_PERCENT && y == 0) {
    runtime_error(line, "Remainder by zero.");
    return v_null();
  }
  if (op == T_PLUS)
    return v_num(x + y);
  if (op == T_MINUS)
    return v_num(x - y);
  if (op == T_STAR)
    return v_num(x * y);
  if (op == T_SLASH)
    return v_num(x / y);
  if (op == T_PERCENT)
    return v_num(fmod(x, y));
  return v_num(pow(x, y));
}
static Value exact_power(Value base, Value exponent, int line) {
  char *exp_num = NULL, *exp_den = NULL;
  exact_components(exponent, &exp_num, &exp_den);
  if (strcmp(exp_den, "1") != 0) {
    free(exp_num);
    free(exp_den);
    return v_num(pow(numeric_double(base), numeric_double(exponent)));
  }
  bool negative = exp_num[0] == '-';
  char *exp = bi_norm(bi_mag(exp_num));
  free(exp_num);
  free(exp_den);
  char *bn, *bd;
  exact_components(base, &bn, &bd);
  if (negative) {
    if (!strcmp(bn, "0")) {
      runtime_error(line, "Zero cannot be raised to a negative exponent.");
      free(exp);
      free(bn);
      free(bd);
      return v_null();
    }
    char *tmp = bn;
    bn = bd;
    bd = tmp;
  }
  char *rn = copy_s("1"), *rd = copy_s("1");
  while (strcmp(exp, "0") != 0) {
    unsigned int bit = 0;
    char *half = bi_div_small(exp, 2, &bit);
    free(exp);
    exp = half;
    if (bit) {
      char *nn = bi_mul(rn, bn), *dd = bi_mul(rd, bd);
      free(rn);
      free(rd);
      rn = nn;
      rd = dd;
    }
    if (strcmp(exp, "0") != 0) {
      char *nn = bi_mul(bn, bn), *dd = bi_mul(bd, bd);
      free(bn);
      free(bd);
      bn = nn;
      bd = dd;
    }
  }
  Value result = v_ratio(rn, rd);
  free(rn);
  free(rd);
  free(bn);
  free(bd);
  free(exp);
  return result;
}
static bool valid_numeric_text(const char *s) {
  size_t i = 0;
  bool digits = false;
  if (s[i] == '+' || s[i] == '-')
    i++;
  while (isdigit((unsigned char)s[i])) {
    digits = true;
    i++;
  }
  if (s[i] == '.') {
    i++;
    while (isdigit((unsigned char)s[i])) {
      digits = true;
      i++;
    }
  }
  if (!digits)
    return false;
  if (s[i] == 'e' || s[i] == 'E') {
    i++;
    if (s[i] == '+' || s[i] == '-')
      i++;
    size_t before = i;
    while (isdigit((unsigned char)s[i]))
      i++;
    if (i == before)
      return false;
  }
  return s[i] == '\0';
}
/* UTF-8 helpers: character-to-number conversion uses Unicode code points,
 * not Python int()-style decimal parsing of every string. Numeric strings are
 * still parsed as numbers; a non-numeric single-character string maps to its
 * Unicode scalar value. */
static bool utf8_decode_one(const unsigned char *s, size_t *used,
                            uint32_t *cp) {
  if (!s || !s[0])
    return false;
  uint32_t c;
  size_t n;
  if (s[0] < 0x80) {
    c = s[0];
    n = 1;
  } else if ((s[0] & 0xE0) == 0xC0) {
    c = s[0] & 0x1F;
    n = 2;
    if (c < 2)
      return false;
  } else if ((s[0] & 0xF0) == 0xE0) {
    c = s[0] & 0x0F;
    n = 3;
  } else if ((s[0] & 0xF8) == 0xF0) {
    c = s[0] & 0x07;
    n = 4;
    if (c > 4)
      return false;
  } else
    return false;
  for (size_t i = 1; i < n; i++) {
    if ((s[i] & 0xC0) != 0x80)
      return false;
    c = (c << 6) | (uint32_t)(s[i] & 0x3F);
  }
  if ((n == 2 && c < 0x80) || (n == 3 && c < 0x800) ||
      (n == 4 && c < 0x10000) || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF))
    return false;
  *used = n;
  *cp = c;
  return true;
}
static bool utf8_single_codepoint(const char *s, uint32_t *cp) {
  size_t n = 0;
  uint32_t c = 0;
  if (!s || !utf8_decode_one((const unsigned char *)s, &n, &c) || s[n] != '\0')
    return false;
  *cp = c;
  return true;
}
static size_t utf8_encode_codepoint(uint32_t c, char out[5]) {
  if (c <= 0x7F) {
    out[0] = (char)c;
    out[1] = 0;
    return 1;
  }
  if (c <= 0x7FF) {
    out[0] = (char)(0xC0 | (c >> 6));
    out[1] = (char)(0x80 | (c & 0x3F));
    out[2] = 0;
    return 2;
  }
  if (c >= 0xD800 && c <= 0xDFFF)
    return 0;
  if (c <= 0xFFFF) {
    out[0] = (char)(0xE0 | (c >> 12));
    out[1] = (char)(0x80 | ((c >> 6) & 0x3F));
    out[2] = (char)(0x80 | (c & 0x3F));
    out[3] = 0;
    return 3;
  }
  if (c <= 0x10FFFF) {
    out[0] = (char)(0xF0 | (c >> 18));
    out[1] = (char)(0x80 | ((c >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((c >> 6) & 0x3F));
    out[3] = (char)(0x80 | (c & 0x3F));
    out[4] = 0;
    return 4;
  }
  return 0;
}
static bool numeric_to_long_trunc(Value v, long *out);
static bool numeric_to_long(Value v, long *out) {
  /* Integer-taking APIs accept floating-point and rational numbers by
   * truncating toward zero, matching a beginner-friendly implicit int cast. */
  return numeric_to_long_trunc(v, out);
}
static bool numeric_to_long_trunc(Value v, long *out) {
  if (v.type == V_BOOL) {
    *out = v.as.boolean ? 1L : 0L;
    return true;
  }
  if (v.type == V_STRING) {
    uint32_t cp = 0;
    if (utf8_single_codepoint(v.as.string, &cp) && cp <= (uint32_t)LONG_MAX) {
      *out = (long)cp;
      return true;
    }
    if (valid_numeric_text(v.as.string)) {
      errno = 0;
      char *end = NULL;
      double d = strtod(v.as.string, &end);
      if (errno == ERANGE || !end || *end || !isfinite(d) ||
          d < (double)LONG_MIN || d >= -(double)LONG_MIN)
        return false;
      *out = (long)d;
      return true;
    }
    return false;
  }
  if (v.type == V_NUMBER) {
    if (!isfinite(v.as.number) || v.as.number < (double)LONG_MIN ||
        v.as.number >= -(double)LONG_MIN)
      return false;
    *out = (long)v.as.number;
    return true;
  }
  char *integer = NULL;
  if (v.type == V_BIGINT)
    integer = copy_s(v.as.bigint);
  else if (v.type == V_RATIONAL)
    integer = bi_div(v.as.rational.num, v.as.rational.den, false);
  else
    return false;
  errno = 0;
  char *end = NULL;
  long n = strtol(integer, &end, 10);
  bool ok = errno != ERANGE && end != integer && end && *end == '\0';
  free(integer);
  if (!ok)
    return false;
  *out = n;
  return true;
}
static bool numeric_to_u32(Value v, uint32_t *out) {
  if (v.type == V_NUMBER) {
    if (!isfinite(v.as.number) || floor(v.as.number) != v.as.number ||
        v.as.number < 0 || v.as.number > 4294967295.0)
      return false;
    *out = (uint32_t)v.as.number;
    return true;
  }
  if (v.type == V_STRING) {
    if (utf8_single_codepoint(v.as.string, out))
      return true;
    if (!*v.as.string || !valid_numeric_text(v.as.string))
      return false;
    errno = 0;
    char *end = NULL;
    unsigned long long n = strtoull(v.as.string, &end, 10);
    if (errno == ERANGE || end == v.as.string || !end || *end || n > UINT32_MAX)
      return false;
    *out = (uint32_t)n;
    return true;
  }
  const char *text = NULL;
  if (v.type == V_BIGINT)
    text = v.as.bigint;
  else if (v.type == V_RATIONAL && strcmp(v.as.rational.den, "1") == 0)
    text = v.as.rational.num;
  else
    return false;
  if (*text == '-')
    return false;
  errno = 0;
  char *end = NULL;
  unsigned long long n = strtoull(text, &end, 10);
  if (errno == ERANGE || end == text || !end || *end || n > UINT32_MAX)
    return false;
  *out = (uint32_t)n;
  return true;
}
static bool numeric_to_u64(Value v, uint64_t *out) {
  if (v.type == V_NUMBER) {
    /* A binary double is only guaranteed to represent every integer up to
     * 2^53-1. */
    if (!isfinite(v.as.number) || floor(v.as.number) != v.as.number ||
        v.as.number < 0.0 || v.as.number > 9007199254740991.0)
      return false;
    *out = (uint64_t)v.as.number;
    return true;
  }
  const char *text = NULL;
  if (v.type == V_BIGINT)
    text = v.as.bigint;
  else if (v.type == V_RATIONAL && strcmp(v.as.rational.den, "1") == 0)
    text = v.as.rational.num;
  else
    return false;
  if (!text || !*text || *text == '-')
    return false;
  errno = 0;
  char *end = NULL;
  unsigned long long n = strtoull(text, &end, 10);
  if (errno == ERANGE || end == text || !end || *end)
    return false;
  *out = (uint64_t)n;
  return true;
}

static Value numeric_literal_value(const char *text, int line) {
  if (!strpbrk(text, ".eE"))
    return v_bigint(text);
  const char *expmark = strchr(text, 'e');
  if (!expmark)
    expmark = strchr(text, 'E');
  size_t mantissa_len = expmark ? (size_t)(expmark - text) : strlen(text);
  long long exponent = 0;
  if (expmark) {
    char *end = NULL;
    errno = 0;
    exponent = strtoll(expmark + 1, &end, 10);
    if (errno == ERANGE || !end || *end) {
      runtime_error(line, "Numeric exponent is too large to represent safely.");
      return v_null();
    }
  }
  size_t dot = mantissa_len;
  for (size_t i = 0; i < mantissa_len; i++)
    if (text[i] == '.') {
      dot = i;
      break;
    }
  size_t frac_places = dot < mantissa_len ? mantissa_len - dot - 1 : 0;
  size_t digits_count = mantissa_len - (dot < mantissa_len ? 1u : 0u);
  char *digits = (char *)malloc(digits_count + 2);
  if (!digits) {
    runtime_error(line, "Not enough memory to parse numeric literal.");
    return v_null();
  }
  size_t w = 0;
  for (size_t i = 0; i < mantissa_len; i++)
    if (text[i] != '.')
      digits[w++] = text[i];
  digits[w] = '\0';
  char *num = bi_norm(digits);
  free(digits);
  if (exponent >= 0 && (unsigned long long)exponent >= frac_places) {
    size_t zeros = (size_t)((unsigned long long)exponent - frac_places);
    size_t n = strlen(num);
    if (zeros > SIZE_MAX - n - 1) {
      free(num);
      runtime_error(line, "Numeric literal exceeds addressable memory size.");
      return v_null();
    }
    char *expanded = (char *)malloc(n + zeros + 1);
    if (!expanded) {
      free(num);
      runtime_error(line, "Not enough memory to parse numeric exponent.");
      return v_null();
    }
    memcpy(expanded, num, n);
    memset(expanded + n, '0', zeros);
    expanded[n + zeros] = '\0';
    Value value = v_ratio(expanded, "1");
    free(expanded);
    free(num);
    return value;
  }
  size_t scale;
  if (exponent < 0) {
    unsigned long long magnitude = (unsigned long long)(-(exponent + 1)) + 1ULL;
    if (magnitude > SIZE_MAX - frac_places) {
      free(num);
      runtime_error(line, "Numeric exponent exceeds addressable memory size.");
      return v_null();
    }
    scale = frac_places + (size_t)magnitude;
  } else
    scale = frac_places - (size_t)exponent;
  if (scale > SIZE_MAX - 2) {
    free(num);
    runtime_error(line, "Numeric scale exceeds addressable memory size.");
    return v_null();
  }
  char *den = (char *)malloc(scale + 2);
  if (!den) {
    free(num);
    runtime_error(line, "Not enough memory to parse numeric fraction.");
    return v_null();
  }
  den[0] = '1';
  memset(den + 1, '0', scale);
  den[scale + 1] = '\0';
  Value value = v_ratio(num, den);
  free(num);
  free(den);
  return value;
}
static char *rational_to_string(Value v) {
  const char *num = v.as.rational.num, *den = v.as.rational.den;
  if (!strcmp(den, "1"))
    return copy_s(num);
  char *remaining = copy_s(den);
  size_t twos = 0, fives = 0;
  unsigned int rem = 0;
  for (;;) {
    char *q = bi_div_small(remaining, 2, &rem);
    if (rem) {
      free(q);
      break;
    }
    free(remaining);
    remaining = q;
    twos++;
  }
  for (;;) {
    char *q = bi_div_small(remaining, 5, &rem);
    if (rem) {
      free(q);
      break;
    }
    free(remaining);
    remaining = q;
    fives++;
  }
  bool finite = !strcmp(remaining, "1");
  free(remaining);
  if (!finite) {
    size_t a = strlen(num), b = strlen(den);
    if (a > SIZE_MAX - b - 2)
      return copy_s("<number>");
    char *out = (char *)malloc(a + b + 2);
    if (!out)
      exit(2);
    snprintf(out, a + b + 2, "%s/%s", num, den);
    return out;
  }
  size_t scale = twos > fives ? twos : fives;
  char *scaled = copy_s(num);
  for (size_t i = twos; i < scale; i++) {
    char *q = bi_mul(scaled, "2");
    free(scaled);
    scaled = q;
  }
  for (size_t i = fives; i < scale; i++) {
    char *q = bi_mul(scaled, "5");
    free(scaled);
    scaled = q;
  }
  bool negative = scaled[0] == '-';
  const char *mag = bi_mag(scaled);
  size_t len = strlen(mag);
  if (scale == 0) {
    free(scaled);
    return copy_s(num);
  }
  if (scale > SIZE_MAX - len - 4) {
    free(scaled);
    return copy_s("<number>");
  }
  size_t cap = (len > scale ? len : scale) + 4 + (negative ? 1u : 0u);
  char *out = (char *)malloc(cap);
  if (!out)
    exit(2);
  size_t pos = 0;
  if (negative && strcmp(mag, "0"))
    out[pos++] = '-';
  if (len <= scale) {
    out[pos++] = '0';
    out[pos++] = '.';
    for (size_t i = 0; i < scale - len; i++)
      out[pos++] = '0';
    memcpy(out + pos, mag, len);
    pos += len;
  } else {
    size_t whole = len - scale;
    memcpy(out + pos, mag, whole);
    pos += whole;
    out[pos++] = '.';
    memcpy(out + pos, mag + whole, scale);
    pos += scale;
  }
  while (pos > 0 && out[pos - 1] == '0')
    pos--;
  if (pos > 0 && out[pos - 1] == '.')
    pos--;
  if (pos == 0) {
    out[pos++] = '0';
  }
  out[pos] = '\0';
  free(scaled);
  return out;
}

static Value v_bool(bool x) {
  Value v;
  v.type = V_BOOL;
  v.as.boolean = x;
  return v;
}
static Value v_string(const char *s) {
  Value v;
  v.type = V_STRING;
  v.as.string = copy_s(s ? s : "");
  return v;
}
static Value v_array(void) {
  if (!gc_running && ++gc_allocations > 256)
    gc_pending = true;
  Array *a = (Array *)calloc(1, sizeof(Array));
  if (!a)
    exit(2);
  a->next = all_arrays;
  all_arrays = a;
  Value v;
  v.type = V_ARRAY;
  v.as.array = a;
  return v;
}
static Value copy_value(Value v);
static void free_value(Value v);

static void enqueue_fluorine_task(Value callee, const char *native_name,
                                  int argc, Value *args, int line) {
  FluorineTask *t = (FluorineTask *)calloc(1, sizeof(*t));
  if (!t) {
    runtime_error(line, "Could not allocate green thread.");
    return;
  }
  t->callee = copy_value(callee);
  t->native_name = native_name ? copy_s(native_name) : NULL;
  t->argc = argc;
  t->line = line;
  t->args = (Value *)calloc((size_t)(argc ? argc : 1), sizeof(Value));
  if (!t->args) {
    free_value(t->callee);
    free(t->native_name);
    free(t);
    runtime_error(line, "Could not allocate green-thread arguments.");
    return;
  }
  for (int i = 0; i < argc; i++)
    t->args[i] = copy_value(args[i]);
  t->saved_active_envs = active_envs;
  t->saved_exec_depth = exec_depth;
  t->saved_exception_handler_depth = exception_handler_depth;
  t->saved_has_exception = has_exception;
  t->saved_pending_exception = copy_value(pending_exception);
  t->all_next = fluorine_all_tasks;
  fluorine_all_tasks = t;
  if (fluorine_tasks_tail)
    fluorine_tasks_tail->next = t;
  else
    fluorine_tasks_head = t;
  fluorine_tasks_tail = t;
}

static Value v_map(void) {
  if (!gc_running && ++gc_allocations > 256)
    gc_pending = true;
  Map *m = (Map *)calloc(1, sizeof(Map));
  if (!m)
    exit(2);
  m->next = all_maps;
  all_maps = m;
  Value v;
  v.type = V_MAP;
  v.as.map = m;
  return v;
}
static int map_find(Map *m, Value key) {
  for (int i = 0; i < m->count; i++) {
    Value k = m->items[i].key;
    if ((is_numeric(k) && is_numeric(key) && numeric_cmp(k, key) == 0) ||
        (k.type == key.type &&
         ((k.type == V_STRING && strcmp(k.as.string, key.as.string) == 0) ||
          (k.type == V_BOOL && k.as.boolean == key.as.boolean) ||
          k.type == V_NULL)))
      return i;
  }
  return -1;
}
static void map_set(Map *m, Value key, Value value) {
  int i = map_find(m, key);
  if (i < 0) {
    if (m->count == m->capacity) {
      m->capacity = m->capacity ? m->capacity * 2 : 8;
      m->items = (MapEntry *)xrealloc(m->items,
                                      (size_t)m->capacity * sizeof(MapEntry));
    }
    i = m->count++;
    m->items[i].key = copy_value(key);
    m->items[i].value.type = V_NULL;
  } else
    free_value(m->items[i].value);
  m->items[i].value = copy_value(value);
}

static void gc_mark_env(Env *e);
static void gc_mark_value(Value v) {
  if (v.type == V_ARRAY && v.as.array && !v.as.array->marked) {
    Array *a = v.as.array;
    a->marked = true;
    for (int i = 0; i < a->count; i++)
      gc_mark_value(a->items[i]);
  } else if ((v.type == V_MAP || v.type == V_CLASS || v.type == V_STRUCT_DEF) &&
             v.as.map && !v.as.map->marked) {
    Map *m = v.as.map;
    m->marked = true;
    for (int i = 0; i < m->count; i++) {
      gc_mark_value(m->items[i].key);
      gc_mark_value(m->items[i].value);
    }
  } else if (v.type == V_FUNCTION && v.as.function && !v.as.function->marked) {
    v.as.function->marked = true;
    gc_mark_env(v.as.function->closure);
  } else if (v.type == V_POINTER && v.as.pointer && !v.as.pointer->marked) {
    SafePointer *q = v.as.pointer;
    q->marked = true;
    if (q->kind == PTR_BINDING)
      gc_mark_env(q->env);
    else if (q->kind == PTR_MAP_ENTRY) {
      Value m;
      m.type = V_MAP;
      m.as.map = q->map;
      gc_mark_value(m);
      gc_mark_value(q->key);
    } else if (q->kind == PTR_ARRAY_ITEM) {
      Value a;
      a.type = V_ARRAY;
      a.as.array = q->array;
      gc_mark_value(a);
    }
  } else if (v.type == V_THREAD && v.as.thread) {
    if (v.as.thread->joined)
      gc_mark_value(v.as.thread->result);
    else {
      gc_mark_value(v.as.thread->callee);
      for (int ti = 0; ti < v.as.thread->argc; ti++)
        gc_mark_value(v.as.thread->args[ti]);
    }
  }
}
static void gc_mark_env(Env *e) {
  if (!e || e->marked)
    return;
  e->marked = true;
  gc_mark_env(e->parent);
  for (int i = 0; i < e->count; i++)
    gc_mark_value(e->bindings[i].value);
}
static size_t gc_collect(void) {
  if (gc_running)
    return 0;
  runtime_lock();
  gc_running = true;
  for (EnvLink *l = env_list; l; l = l->next)
    l->env->marked = false;
  for (Array *a = all_arrays; a; a = a->next)
    a->marked = false;
  for (Map *m = all_maps; m; m = m->next)
    m->marked = false;
  for (size_t i = 0; i < function_count; i++)
    if (all_functions[i])
      all_functions[i]->marked = false;
  for (SafePointer *q = all_pointers; q; q = q->next)
    q->marked = false;
  gc_mark_env(global_env);
  for (FluorineThread *t = all_threads; t; t = t->next)
    if (!t->joined) {
      gc_mark_value(t->callee);
      for (int ti = 0; ti < t->argc; ti++)
        gc_mark_value(t->args[ti]);
    }
  for (ActiveEnv *a = active_envs; a; a = a->next)
    gc_mark_env(a->env);
  if (has_exception)
    gc_mark_value(pending_exception);
  for (FluorineTask *t = fluorine_tasks_head; t; t = t->next) {
    gc_mark_value(t->callee);
    for (int i = 0; i < t->argc; i++)
      gc_mark_value(t->args[i]);
  }
  for (FluorineTask *t = fluorine_all_tasks; t; t = t->all_next) {
    gc_mark_value(t->callee);
    for (int i = 0; i < t->argc; i++)
      gc_mark_value(t->args[i]);
    for (ActiveEnv *a = t->saved_active_envs; a; a = a->next)
      gc_mark_env(a->env);
    if (t->saved_has_exception)
      gc_mark_value(t->saved_pending_exception);
  }
  for (size_t i = 0; i < isolated_module_count; i++) {
    Value root;
    root.type = V_MAP;
    root.as.map = isolated_module_maps[i];
    gc_mark_value(root);
  }
  size_t freed = 0;
  EnvLink **ep = &env_list;
  while (*ep) {
    EnvLink *l = *ep;
    Env *e = l->env;
    if (!e->marked) {
      *ep = l->next;
      for (int i = 0; i < e->count; i++) {
        free(e->bindings[i].name);
        free_value(e->bindings[i].value);
      }
      free(e->bindings);
      free(e);
      free(l);
      freed++;
    } else
      ep = &l->next;
  }
  for (size_t i = 0; i < function_count; i++)
    if (all_functions[i] && !all_functions[i]->marked) {
      free(all_functions[i]);
      all_functions[i] = NULL;
      freed++;
    }
  SafePointer **sp = &all_pointers;
  while (*sp) {
    SafePointer *q = *sp;
    if (!q->marked) {
      *sp = q->next;
      free(q->name);
      free_value(q->key);
      free(q);
      freed++;
    } else
      sp = &q->next;
  }
  Array **ap = &all_arrays;
  while (*ap) {
    Array *a = *ap;
    if (!a->marked) {
      *ap = a->next;
      for (int i = 0; i < a->count; i++)
        free_value(a->items[i]);
      free(a->items);
      free(a);
      freed++;
    } else
      ap = &a->next;
  }
  Map **mp = &all_maps;
  while (*mp) {
    Map *m = *mp;
    if (!m->marked) {
      *mp = m->next;
      for (int i = 0; i < m->count; i++) {
        free_value(m->items[i].key);
        free_value(m->items[i].value);
      }
      free(m->items);
      free(m);
      freed++;
    } else
      mp = &m->next;
  }
  gc_allocations = 0;
  gc_running = false;
  runtime_unlock();
  return freed;
}

static bool truthy(Value v) {
  switch (v.type) {
  case V_NULL:
    return false;
  case V_BOOL:
    return v.as.boolean;
  case V_NUMBER:
    return v.as.number != 0.0 && !isnan(v.as.number);
  case V_BIGINT:
    return strcmp(v.as.bigint, "0") != 0;
  case V_RATIONAL:
    return strcmp(v.as.rational.num, "0") != 0;
  case V_STRING:
    return v.as.string[0] != '\0';
  default:
    return true;
  }
}
static void free_value(Value v) {
  if (v.type == V_STRING)
    free(v.as.string);
  else if (v.type == V_BIGINT)
    free(v.as.bigint);
  else if (v.type == V_RATIONAL) {
    free(v.as.rational.num);
    free(v.as.rational.den);
  }
}
static Value copy_value(Value v) {
  if (v.type == V_STRING)
    return v_string(v.as.string);
  if (v.type == V_BIGINT)
    return v_bigint(v.as.bigint);
  if (v.type == V_RATIONAL)
    return v_ratio(v.as.rational.num, v.as.rational.den);
  return v;
}
static char *value_to_string(Value v) {
  char buf[128];
  switch (v.type) {
  case V_NULL:
    return copy_s("null");
  case V_BOOL:
    return copy_s(v.as.boolean ? "true" : "false");
  case V_BIGINT:
    return copy_s(v.as.bigint);
  case V_RATIONAL:
    return rational_to_string(v);
  case V_NUMBER:
    if (isfinite(v.as.number) && fabs(v.as.number) < 1e15 &&
        floor(v.as.number) == v.as.number)
      snprintf(buf, sizeof(buf), "%.0f", v.as.number);
    else
      snprintf(buf, sizeof(buf), "%.15g", v.as.number);
    return copy_s(buf);
  case V_STRING:
    return copy_s(v.as.string);
  case V_CLASS:
    return copy_s("<class>");
  case V_STRUCT_DEF:
    return copy_s("<struct> ");
  case V_POINTER:
    return copy_s("<safe-pointer>");
  case V_THREAD:
    return copy_s("<native-thread>");
  case V_MAP: {
    size_t cap = 64;
    char *out = (char *)malloc(cap);
    if (!out)
      exit(2);
    size_t used = 0;
    out[used++] = '{';
    for (int i = 0; i < v.as.map->count; i++) {
      char *k = value_to_string(v.as.map->items[i].key);
      char *val = value_to_string(v.as.map->items[i].value);
      size_t need = strlen(k) + strlen(val) + 5;
      if (used + need + 1 > cap) {
        while (used + need + 1 > cap)
          cap *= 2;
        out = (char *)xrealloc(out, cap);
      }
      if (i) {
        out[used++] = ',';
        out[used++] = ' ';
      }
      memcpy(out + used, k, strlen(k));
      used += strlen(k);
      out[used++] = ':';
      out[used++] = ' ';
      memcpy(out + used, val, strlen(val));
      used += strlen(val);
      free(k);
      free(val);
    }
    out[used++] = '}';
    out[used] = '\0';
    return out;
  }
  case V_ARRAY: {
    size_t cap = 64;
    char *out = (char *)malloc(cap);
    if (!out)
      exit(2);
    size_t used = 0;
    out[used++] = '[';
    for (int i = 0; i < v.as.array->count; i++) {
      char *item = value_to_string(v.as.array->items[i]);
      size_t need = strlen(item) + 4;
      if (used + need + 1 > cap) {
        while (used + need + 1 > cap)
          cap *= 2;
        out = (char *)xrealloc(out, cap);
      }
      if (i) {
        out[used++] = ',';
        out[used++] = ' ';
      }
      memcpy(out + used, item, strlen(item));
      used += strlen(item);
      free(item);
    }
    out[used++] = ']';
    out[used] = '\0';
    return out;
  }
  case V_FUNCTION:
    return copy_s("<function>");
  case V_NATIVE:
    return copy_s("<native function>");
  }
  return copy_s("<value>");
}
static bool equal_values(Value a, Value b) {
  if (is_numeric(a) && is_numeric(b)) {
    if (a.type == V_NUMBER && b.type == V_NUMBER)
      return a.as.number == b.as.number;
    if ((a.type == V_NUMBER && !isfinite(a.as.number)) ||
        (b.type == V_NUMBER && !isfinite(b.as.number)))
      return false;
    return numeric_cmp(a, b) == 0;
  }
  if (a.type != b.type)
    return false;
  switch (a.type) {
  case V_NULL:
    return true;
  case V_NUMBER:
    return a.as.number == b.as.number;
  case V_BIGINT:
    return strcmp(a.as.bigint, b.as.bigint) == 0;
  case V_BOOL:
    return a.as.boolean == b.as.boolean;
  case V_STRING:
    return strcmp(a.as.string, b.as.string) == 0;
  case V_ARRAY:
    return a.as.array == b.as.array;
  case V_MAP:
    return a.as.map == b.as.map;
  case V_CLASS:
  case V_STRUCT_DEF:
    return a.as.map == b.as.map;
  case V_POINTER:
    return a.as.pointer == b.as.pointer;
  case V_THREAD:
    return a.as.thread == b.as.thread;
  default:
    return a.as.function == b.as.function;
  }
}

static Env *new_env(Env *parent) {
  Env *e = (Env *)calloc(1, sizeof(Env));
  if (!e)
    exit(2);
  e->parent = parent;
  EnvLink *link = (EnvLink *)malloc(sizeof(EnvLink));
  if (!link)
    exit(2);
  link->env = e;
  link->next = env_list;
  env_list = link;
  all_envs = e;
  return e;
}
static Binding *find_local(Env *e, const char *name) {
  for (int i = 0; i < e->count; i++)
    if (strcmp(e->bindings[i].name, name) == 0)
      return &e->bindings[i];
  return NULL;
}
static Binding *find_binding(Env *e, const char *name) {
  for (Env *p = e; p; p = p->parent) {
    Binding *b = find_local(p, name);
    if (b)
      return b;
  }
  return NULL;
}
static bool define_var(Env *e, const char *name, Value value, int line) {
  if (find_local(e, name)) {
    runtime_error(line, "Variable '%s' is already declared in this block.",
                  name);
    return false;
  }
  if (e->count == e->capacity) {
    e->capacity = e->capacity ? e->capacity * 2 : INITIAL_CAP;
    e->bindings =
        (Binding *)xrealloc(e->bindings, (size_t)e->capacity * sizeof(Binding));
  }
  e->bindings[e->count].name = copy_s(name);
  e->bindings[e->count].value = copy_value(value);
  e->count++;
  return true;
}
static bool assign_var(Env *e, const char *name, Value value, int line) {
  Binding *b = find_binding(e, name);
  if (!b) {
    runtime_error(line, "Undefined variable '%s'.", name);
    return false;
  }
  free_value(b->value);
  b->value = copy_value(value);
  return true;
}
static SafePointer *new_safe_pointer(SafePointerKind kind) {
  SafePointer *p = (SafePointer *)calloc(1, sizeof(*p));
  if (!p) {
    fprintf(stderr, "Out of memory\n");
    exit(2);
  }
  p->kind = kind;
  p->key = v_null();
  p->next = all_pointers;
  all_pointers = p;
  return p;
}
static Value pointer_value(SafePointer *p) {
  Value v;
  v.type = V_POINTER;
  v.as.pointer = p;
  return v;
}
static bool pointer_read(SafePointer *p, Value *out, int line) {
  if (!p) {
    runtime_error(line, "Null or invalid safe pointer.");
    return false;
  }
  if (p->kind == PTR_BINDING) {
    if (!p->env || p->index < 0 || p->index >= p->env->count) {
      runtime_error(line, "Safe pointer is out of bounds (one-past-end cannot "
                          "be dereferenced).");
      return false;
    }
    *out = copy_value(p->env->bindings[p->index].value);
    return true;
  }
  if (p->kind == PTR_MAP_ENTRY) {
    if (!p->map || p->index < 0 || p->index >= p->map->count) {
      runtime_error(line, "Safe map pointer is out of bounds (one-past-end "
                          "cannot be dereferenced).");
      return false;
    }
    *out = copy_value(p->map->items[p->index].value);
    return true;
  }
  if (p->kind == PTR_ARRAY_ITEM) {
    if (!p->array || p->index < 0 || p->index >= p->array->count) {
      runtime_error(line, "Safe pointer array target is out of bounds (array "
                          "may have been shortened).");
      return false;
    }
    *out = copy_value(p->array->items[p->index]);
    return true;
  }
  runtime_error(line, "Invalid safe pointer target.");
  return false;
}
static bool pointer_write(SafePointer *p, Value value, int line) {
  if (!p) {
    runtime_error(line, "Null or invalid safe pointer.");
    return false;
  }
  if (p->kind == PTR_BINDING) {
    if (!p->env || p->index < 0 || p->index >= p->env->count) {
      runtime_error(
          line,
          "Safe pointer is out of bounds (one-past-end cannot be written).");
      return false;
    }
    free_value(p->env->bindings[p->index].value);
    p->env->bindings[p->index].value = copy_value(value);
    return true;
  }
  if (p->kind == PTR_MAP_ENTRY) {
    if (!p->map || p->index < 0 || p->index >= p->map->count) {
      runtime_error(line, "Safe map pointer is out of bounds (one-past-end "
                          "cannot be written).");
      return false;
    }
    free_value(p->map->items[p->index].value);
    p->map->items[p->index].value = copy_value(value);
    return true;
  }
  if (p->kind == PTR_ARRAY_ITEM) {
    if (!p->array || p->index < 0 || p->index >= p->array->count) {
      runtime_error(line, "Safe pointer array target is out of bounds (array "
                          "may have been shortened).");
      return false;
    }
    free_value(p->array->items[p->index]);
    p->array->items[p->index] = copy_value(value);
    return true;
  }
  runtime_error(line, "Invalid safe pointer target.");
  return false;
}
static bool pointer_move(SafePointer *p, int delta, int line) {
  if (!p) {
    runtime_error(line, "Cannot perform arithmetic on a null pointer.");
    return false;
  }
  int limit = -1;
  if (p->kind == PTR_BINDING) {
    if (!p->env) {
      runtime_error(line, "Pointer has no binding storage.");
      return false;
    }
    limit = p->env->count;
  } else if (p->kind == PTR_MAP_ENTRY) {
    if (!p->map) {
      runtime_error(line, "Pointer has no map storage.");
      return false;
    }
    limit = p->map->count;
  } else if (p->kind == PTR_ARRAY_ITEM) {
    if (!p->array) {
      runtime_error(line, "Pointer has no array storage.");
      return false;
    }
    limit = p->array->count;
  } else {
    runtime_error(line, "Fluorine safe-pointer kind.");
    return false;
  }
  long next = (long)p->index + (long)delta;
  if (next < 0 || next > limit) {
    runtime_error(line, "Pointer arithmetic moved outside its storage "
                        "(one-past-end is allowed).");
    return false;
  }
  p->index = (int)next;
  if (p->kind == PTR_BINDING) {
    free(p->name);
    p->name =
        (p->index < limit) ? copy_s(p->env->bindings[p->index].name) : NULL;
  }
  if (p->kind == PTR_MAP_ENTRY) {
    free_value(p->key);
    p->key =
        (p->index < limit) ? copy_value(p->map->items[p->index].key) : v_null();
  }
  return true;
}
static SafePointer *clone_safe_pointer(const SafePointer *src) {
  if (!src)
    return NULL;
  SafePointer *p = new_safe_pointer(src->kind);
  p->env = src->env;
  p->name = src->name ? copy_s(src->name) : NULL;
  p->map = src->map;
  p->key = copy_value(src->key);
  p->array = src->array;
  p->index = src->index;
  return p;
}

typedef struct {
  ValueType type;
  void *source;
  Value clone;
} CloneEntry;
typedef struct {
  CloneEntry *items;
  size_t count, capacity;
} CloneMemo;
static bool clone_instance_graph(Value source, CloneMemo *memo, int depth,
                                 int line, Value *out) {
  if (depth > 256) {
    runtime_error(
        line,
        "Object field cloning exceeded the maximum nesting depth of 256.");
    return false;
  }
  if (source.type != V_ARRAY && source.type != V_MAP) {
    *out = copy_value(source);
    return true;
  }
  void *identity =
      source.type == V_ARRAY ? (void *)source.as.array : (void *)source.as.map;
  for (size_t i = 0; i < memo->count; i++)
    if (memo->items[i].type == source.type &&
        memo->items[i].source == identity) {
      *out = copy_value(memo->items[i].clone);
      return true;
    }
  Value clone = source.type == V_ARRAY ? v_array() : v_map();
  if (memo->count == memo->capacity) {
    memo->capacity = memo->capacity ? memo->capacity * 2 : 8;
    memo->items = (CloneEntry *)xrealloc(memo->items,
                                         memo->capacity * sizeof(CloneEntry));
  }
  memo->items[memo->count].type = source.type;
  memo->items[memo->count].source = identity;
  memo->items[memo->count].clone = copy_value(clone);
  memo->count++;
  if (source.type == V_ARRAY) {
    Array *src = source.as.array, *dst = clone.as.array;
    for (int i = 0; i < src->count; i++) {
      Value child = v_null();
      if (!clone_instance_graph(src->items[i], memo, depth + 1, line, &child))
        return false;
      if (dst->count == dst->capacity) {
        dst->capacity = dst->capacity ? dst->capacity * 2 : 8;
        dst->items = (Value *)xrealloc(dst->items,
                                       (size_t)dst->capacity * sizeof(Value));
      }
      dst->items[dst->count++] = copy_value(child);
      free_value(child);
    }
  } else {
    Map *src = source.as.map;
    for (int i = 0; i < src->count; i++) {
      Value key = v_null(), value = v_null();
      if (!clone_instance_graph(src->items[i].key, memo, depth + 1, line,
                                &key) ||
          !clone_instance_graph(src->items[i].value, memo, depth + 1, line,
                                &value)) {
        free_value(key);
        free_value(value);
        return false;
      }
      map_set(clone.as.map, key, value);
      free_value(key);
      free_value(value);
    }
  }
  *out = clone;
  return true;
}

static Value eval(Node *, Env *);
static Flow exec(Node *, Env *);
static Value run_import(const char *path, Env *env, int line, const char *alias,
                        bool quoted);
static Value native_call(const char *name, int argc, Value *a, int line);
static Value call_value(Value callee, int argc, Value *args, int line,
                        Value receiver, bool has_receiver) {
  if (callee.type == V_NATIVE)
    return callee.as.native(argc, args, line);
  if (callee.type == V_CLASS || callee.type == V_STRUCT_DEF) {
    Value instance = v_map();
    CloneMemo memo = {0};
    for (int i = 0; i < callee.as.map->count; i++) {
      Value key = callee.as.map->items[i].key;
      if (key.type == V_STRING &&
          (strcmp(key.as.string, "__fluorine_struct") == 0 ||
           strcmp(key.as.string, "__fluorine_fields") == 0 ||
           strcmp(key.as.string, "__fluorine_parent") == 0 ||
           strcmp(key.as.string, "__fluorine_class_name") == 0))
        continue;
      Value member = v_null();
      if (!clone_instance_graph(callee.as.map->items[i].value, &memo, 0, line,
                                &member)) {
        free(memo.items);
        return v_null();
      }
      map_set(instance.as.map, key, member);
      free_value(member);
    }
    free(memo.items);
    if (callee.type == V_STRUCT_DEF) {
      Value fields_key = v_string("__fluorine_fields");
      int fi = map_find(callee.as.map, fields_key);
      free_value(fields_key);
      Value fields = fi >= 0 ? callee.as.map->items[fi].value : v_null();
      int count = fields.type == V_ARRAY ? fields.as.array->count : 0;
      if (argc != count) {
        runtime_error(line,
                      "Struct constructor expects %d field value(s), got %d.",
                      count, argc);
        return v_null();
      }
      for (int i = 0; i < count; i++)
        map_set(instance.as.map, fields.as.array->items[i], args[i]);
    } else {
      Value init_key = v_string("init");
      int ii = map_find(instance.as.map, init_key);
      free_value(init_key);
      if (ii >= 0 && instance.as.map->items[ii].value.type == V_FUNCTION) {
        Value init = instance.as.map->items[ii].value;
        Value ignored = call_value(init, argc, args, line, instance, true);
        free_value(ignored);
      } else if (argc != 0) {
        runtime_error(line,
                      "Class without init() accepts no constructor arguments.");
        return v_null();
      }
    }
    return instance;
  }
  if (callee.type != V_FUNCTION) {
    runtime_error(line, "This value cannot be called. Call a function, native "
                        "function, class, or struct constructor instead.");
    return v_null();
  }
  FunctionValue *fv = callee.as.function;
  Node *decl = fv->declaration;
  int param_count = decl->as.function.param_count;
  /* A member call supplies its receiver implicitly. This keeps legacy
     explicit-self calls working while allowing obj.method() to bind the
     receiver to the first declared parameter when one is present. */
  bool implicit_receiver = has_receiver && argc + 1 == param_count;
  if (argc + (implicit_receiver ? 1 : 0) != param_count) {
    runtime_error(line, "Function '%s' expects %d argument(s)%s, got %d.",
                  decl->as.function.name, param_count,
                  has_receiver ? " (including its implicit receiver)" : "",
                  argc);
    return v_null();
  }
  Env *call_env = new_env(fv->closure);
  if (has_receiver)
    define_var(call_env, "this", receiver, line);
  int offset = implicit_receiver ? 1 : 0;
  if (implicit_receiver)
    define_var(call_env, decl->as.function.params[0], receiver, line);
  for (int i = 0; i < argc; i++)
    define_var(call_env, decl->as.function.params[i + offset], args[i], line);
  Flow flow = exec(decl->as.function.body, call_env);
  if (flow.is_break || flow.is_continue)
    runtime_error(line, "break/continue cannot escape a function.");
  if (flow.is_throw) {
    free_value(flow.value);
    return v_null();
  }
  return flow.is_return ? flow.value : v_null();
}

static void fluorine_thread_body(FluorineThread *t) {
  /* Interpreter state is shared, so execution is protected by the runtime
     lock. The work itself runs on a real OS thread (Win32 thread/pthread). */
  runtime_lock();
  t->result =
      t->native_name
          ? native_call(t->native_name, t->argc, t->args, t->line)
          : call_value(t->callee, t->argc, t->args, t->line, v_null(), false);
  free_value(t->callee);
  t->callee = v_null();
  free(t->native_name);
  t->native_name = NULL;
  for (int i = 0; i < t->argc; i++)
    free_value(t->args[i]);
  free(t->args);
  t->args = NULL;
  t->argc = 0;
  t->done = true;
  runtime_unlock();
}
#ifdef _WIN32
static DWORD WINAPI fluorine_thread_entry(LPVOID opaque) {
  fluorine_thread_body((FluorineThread *)opaque);
  return 0;
}
#else
static void *fluorine_thread_entry(void *opaque) {
  fluorine_thread_body((FluorineThread *)opaque);
  return NULL;
}
#endif
static Value start_runtime_thread(Value callee, const char *native_name,
                                  int argc, Value *args, int line) {
  if (argc < 0 || argc > MAX_ARGS) {
    runtime_error(line, "Thread accepts at most %d arguments.", MAX_ARGS);
    return v_null();
  }
  FluorineThread *t = (FluorineThread *)calloc(1, sizeof(*t));
  if (!t)
    exit(2);
  t->callee = copy_value(callee);
  t->native_name = native_name ? copy_s(native_name) : NULL;
  t->argc = argc;
  t->line = line;
  t->result = v_null();
  if (argc) {
    t->args = (Value *)calloc((size_t)argc, sizeof(Value));
    if (!t->args)
      exit(2);
    for (int i = 0; i < argc; i++)
      t->args[i] = copy_value(args[i]);
  }
  runtime_lock();
  t->next = all_threads;
  all_threads = t;
  runtime_unlock();
#ifdef _WIN32
  t->handle = CreateThread(NULL, 0, fluorine_thread_entry, t, 0, &t->thread_id);
  if (!t->handle) {
    t->done = true;
    runtime_error(line, "Could not create operating-system thread (error %lu).",
                  (unsigned long)GetLastError());
    return v_null();
  }
#else
  int rc = pthread_create(&t->handle, NULL, fluorine_thread_entry, t);
  if (rc != 0) {
    t->done = true;
    runtime_error(line, "Could not create operating-system thread (error %d).",
                  rc);
    return v_null();
  }
#endif
  t->started = true;
  Value v;
  v.type = V_THREAD;
  v.as.thread = t;
  return v;
}
static Value native_thread_start(int argc, Value *a, int line) {
  if (argc < 1 || a[0].type != V_FUNCTION) {
    runtime_error(line, "thread_start expects a user-defined function followed "
                        "by its arguments.");
    return v_null();
  }
  return start_runtime_thread(a[0], NULL, argc - 1, a + 1, line);
}
static Value native_thread_join(int argc, Value *a, int line) {
  if (argc != 1 || a[0].type != V_THREAD || !a[0].as.thread) {
    runtime_error(line, "thread_join expects one thread handle.");
    return v_null();
  }
  FluorineThread *t = a[0].as.thread;
  if (!t->started) {
    runtime_error(line, "Cannot join a thread that did not start.");
    return v_null();
  }
  if (!t->joined) {
    int depth = runtime_unlock_all();
#ifdef _WIN32
    WaitForSingleObject(t->handle, INFINITE);
    CloseHandle(t->handle);
    t->handle = NULL;
#else
    pthread_join(t->handle, NULL);
#endif
    runtime_relock(depth);
    t->joined = true;
  }
  return copy_value(t->result);
}
static Value native_thread_is_done(int argc, Value *a, int line) {
  if (argc != 1 || a[0].type != V_THREAD || !a[0].as.thread) {
    runtime_error(line, "thread_is_done expects one thread handle.");
    return v_null();
  }
  return v_bool(a[0].as.thread->done);
}
static Value native_thread_count(int argc, Value *a, int line) {
  (void)a;
  if (argc != 0) {
    runtime_error(line, "thread_count expects no arguments.");
    return v_null();
  }
  int n = 0;
  for (FluorineThread *t = all_threads; t; t = t->next)
    if (t->started && !t->joined)
      n++;
  return v_num(n);
}
static void join_all_threads(void) {
  for (;;) {
    runtime_lock();
    FluorineThread *target = NULL;
    for (FluorineThread *t = all_threads; t; t = t->next) {
      if (t->started && !t->joined) {
        target = t;
        break;
      }
    }
    runtime_unlock();
    if (!target)
      break;
#ifdef _WIN32
    if (target->handle) {
      WaitForSingleObject(target->handle, INFINITE);
      CloseHandle(target->handle);
      target->handle = NULL;
    }
#else
    pthread_join(target->handle, NULL);
#endif
    runtime_lock();
    target->joined = true;
    runtime_unlock();
  }
}

typedef struct {
  const char *name;
  const char *doc;
} BuiltinDoc;
static const BuiltinDoc builtin_docs[] = {
    {"sqrt",
     "sqrt(x)\nReturn the non-negative square root of x. Requires x >= 0."},
    {"abs", "abs(x)\nReturn the absolute value of a number."},
    {"floor",
     "floor(x)\nRound x down to the greatest integer not greater than x."},
    {"ceil", "ceil(x)\nRound x up to the least integer not less than x."},
    {"round", "round(x)\nRound x to the nearest integer."},
    {"sin", "sin(x)\nSine of x in radians."},
    {"cos", "cos(x)\nCosine of x in radians."},
    {"tan", "tan(x)\nTangent of x in radians."},
    {"asin", "asin(x)\nInverse sine in radians; x must be in [-1, 1]."},
    {"acos", "acos(x)\nInverse cosine in radians; x must be in [-1, 1]."},
    {"atan", "atan(x)\nInverse tangent in radians."},
    {"log", "log(x)\nNatural logarithm. x must be positive."},
    {"log10", "log10(x)\nBase-10 logarithm. x must be positive."},
    {"exp", "exp(x)\nReturn e raised to x."},
    {"min", "min(a, b, ...)\nReturn the smallest numeric argument."},
    {"max", "max(a, b, ...)\nReturn the largest numeric argument."},
    {"pow", "pow(base, exponent)\nRaise base to exponent."},
    {"input", "input([prompt])\nPrint an optional prompt and read one line "
              "from standard input as a string."},
    {"is_number", "is_number(value)\nReturn true if value is numeric."},
    {"len", "len(value)\nReturn the number of array elements, map entries, or "
            "bytes in a string."},
    {"push", "push(array, value)\nAppend value to array. Mutates the array and "
             "returns the updated length."},
    {"pop", "pop(array)\nRemove and return the last array element."},
    {"range", "range(stop) or range(start, stop[, step])\nCreate an array of "
              "numbers in a half-open arithmetic range; step defaults to 1 and "
              "cannot be zero. Results are capped at one million items."},
    {"keys", "keys(map)\nReturn an array of map keys in insertion order."},
    {"has_key", "has_key(map, key)\nReturn whether map contains key."},
    {"to_number",
     "to_number(value)\nConvert numeric values, booleans, numeric strings, or "
     "one non-numeric Unicode character to its code point."},
    {"to_string",
     "to_string(value)\nReturn the runtime's string representation of value."},
    {"type_of", "type_of(value)\nReturn the runtime type name as a string."},
    {"substring", "substring(text, start[, length])\nReturn a substring using "
                  "byte offsets; negative indexing is not implied."},
    {"split", "split(text, separator)\nSplit text into an array of strings."},
    {"join", "join(array, separator)\nJoin array values into one string."},
    {"trim", "trim(text)\nRemove leading and trailing whitespace."},
    {"upper", "upper(text)\nReturn an uppercase copy of text (locale/platform "
              "behavior applies)."},
    {"lower", "lower(text)\nReturn a lowercase copy of text (locale/platform "
              "behavior applies)."},
    {"contains", "contains(text, part)\nReturn whether text contains part."},
    {"starts_with",
     "starts_with(text, prefix)\nReturn whether text begins with prefix."},
    {"ends_with",
     "ends_with(text, suffix)\nReturn whether text ends with suffix."},
    {"replace",
     "replace(text, old, new)\nReplace occurrences of old with new."},
    {"ord", "ord(character)\nReturn the Unicode code point of the first UTF-8 "
            "character."},
    {"chr", "chr(number)\nCreate a UTF-8 string from a Unicode code point "
            "(excluding surrogates)."},
    {"random",
     "random() or random(low, high)\nReturn a pseudo-random float in [0, 1] or "
     "an inclusive bounded integer when integer bounds are supplied."},
    {"seed_random",
     "seed_random(seed)\nSeed the pseudo-random number generator."},
    {"os_os", "os_os()\nReturn the current operating system name (windows, "
              "macos, linux, freebsd, or fluorine)."},
    {"os_system", "os_system(command)\nRun a command synchronously through the "
                  "system command processor, like C system(); returns the "
                  "platform system() status."},
    {"os_shellexec", "os_shellexec(target)\nAsk the operating system to open a "
                     "file, URL, or registered target in the background; does "
                     "not wait for the opened application."},
    {"bit_and", "bit_and(a, b)\nBitwise AND on 32-bit integers."},
    {"bit_or", "bit_or(a, b)\nBitwise OR on 32-bit integers."},
    {"bit_xor", "bit_xor(a, b)\nBitwise XOR on 32-bit integers."},
    {"bit_not", "bit_not(a)\nBitwise NOT on a 32-bit integer."},
    {"bit_shl", "bit_shl(a, n)\nShift a 32-bit integer left by 0-31 bits."},
    {"bit_shr",
     "bit_shr(a, n)\nLogical right shift of a 32-bit integer by 0-31 bits."},
    {"byte_get",
     "byte_get(array, index)\nRead an integer byte (0-255) from an array."},
    {"byte_set", "byte_set(array, index, value)\nWrite an integer byte (0-255) "
                 "into an array; returns the written byte."},
    {"read_u16_le", "read_u16_le(bytes, offset)\nRead an unsigned 16-bit "
                    "little-endian integer from a numeric byte array."},
    {"read_u16_be", "read_u16_be(bytes, offset)\nRead an unsigned 16-bit "
                    "big-endian integer from a numeric byte array."},
    {"read_u32_le", "read_u32_le(bytes, offset)\nRead an unsigned 32-bit "
                    "little-endian integer from a numeric byte array."},
    {"read_u32_be", "read_u32_be(bytes, offset)\nRead an unsigned 32-bit "
                    "big-endian integer from a numeric byte array."},
    {"read_u64_le",
     "read_u64_le(bytes, offset)\nRead an unsigned 64-bit little-endian "
     "integer exactly as an arbitrary-precision integer."},
    {"read_u64_be",
     "read_u64_be(bytes, offset)\nRead an unsigned 64-bit big-endian integer "
     "exactly as an arbitrary-precision integer."},
    {"write_u16_le",
     "write_u16_le(bytes, offset, value)\nWrite an unsigned 16-bit "
     "little-endian integer to a numeric byte array; returns true on success."},
    {"write_u16_be",
     "write_u16_be(bytes, offset, value)\nWrite an unsigned 16-bit big-endian "
     "integer to a numeric byte array; returns true on success."},
    {"write_u32_le",
     "write_u32_le(bytes, offset, value)\nWrite an unsigned 32-bit "
     "little-endian integer to a numeric byte array; returns true on success."},
    {"write_u32_be",
     "write_u32_be(bytes, offset, value)\nWrite an unsigned 32-bit big-endian "
     "integer to a numeric byte array; returns true on success."},
    {"write_u64_le",
     "write_u64_le(bytes, offset, value)\nWrite an unsigned 64-bit "
     "little-endian integer to a numeric byte array; returns true on success."},
    {"write_u64_be",
     "write_u64_be(bytes, offset, value)\nWrite an unsigned 64-bit big-endian "
     "integer to a numeric byte array; returns true on success."},
    {"shellexec", "shellexec(target)\nAlias for os_shellexec(target)."},
    {"read_text", "read_text(path, [encoding])\nRead a text file and decode it "
                  "to Fluorine UTF-8 strings; defaults to UTF-8. Encoding "
                  "names are supplied by the platform conversion library."},
    {"write_text",
     "write_text(path, text, [encoding])\nWrite UTF-8 text encoded using the "
     "requested encoding (default UTF-8); returns bytes written."},
    {"append_text", "append_text(path, text, [encoding])\nAppend UTF-8 text "
                    "encoded using the requested encoding (default UTF-8)."},
    {"file_exists",
     "file_exists(path)\nReturn whether path refers to an existing file."},
    {"remove_file",
     "remove_file(path)\nRemove a file. Requires filesystem permissions."},
    {"file_size", "file_size(path)\nReturn file size in bytes, or -1 if path "
                  "is not a regular file."},
    {"copy_file", "copy_file(source, destination)\nCopy a file in binary mode; "
                  "return true on success."},
    {"rename_file", "rename_file(source, destination)\nRename or move a file; "
                    "return true on success."},
    {"make_dir", "make_dir(path)\nCreate one directory (not parent "
                 "directories); return true on success."},
    {"is_dir", "is_dir(path)\nReturn whether path is a directory."},
    {"env",
     "env(name)\nRead an environment variable; return null when it is unset."},
    {"set_env", "set_env(name, value)\nSet a process environment variable "
                "where supported."},
    {"clock_ms", "clock_ms()\nReturn a monotonic-ish runtime clock reading in "
                 "milliseconds; do not treat it as wall-clock time."},
    {"sleep_ms", "sleep_ms(milliseconds)\nSuspend the current thread for "
                 "approximately the given duration."},
    {"assert", "assert(condition[, message])\nRaise a runtime error when "
               "condition is false."},
    {"path_join", "path_join(left, right)\nJoin two path components using "
                  "platform-aware separators."},
    {"basename", "basename(path)\nReturn the final path component."},
    {"dirname", "dirname(path)\nReturn the directory portion of a path."},
    {"unix_time", "unix_time()\nReturn wall-clock Unix time in seconds."},
    {"time_string",
     "time_string()\nReturn a formatted local wall-clock time string."},
    {"url_encode",
     "url_encode(text)\nPercent-encode text for use as a URL component."},
    {"url_decode",
     "url_decode(text)\nDecode percent-encoded URL component text."},
    {"http_serve", "http_serve(port, handler[, max_requests])\nServe requests "
                   "on loopback 127.0.0.1. Handler receives method, path, "
                   "headers map, and body; it may return a response map with "
                   "body/status. Omit max_requests to run until stopped."},
    {"http_request",
     "http_request(method, url[, headers_map[, body_string]])\nSend a native "
     "HTTP request and return {status, headers, body, ok}. Supports plain "
     "http:// URLs, HTTP headers and string request bodies; HTTPS requires a "
     "TLS-enabled build. Responses are limited to 8 MiB and connections time "
     "out after about 10 seconds."},
    {"http_get", "http_get(url[, headers_map])\nConvenience GET request; "
                 "returns the same response map as http_request."},
    {"thread_start", "thread_start(function[, arguments...])\nStart a native "
                     "thread running a callable and return a thread handle."},
    {"thread_join",
     "thread_join(handle)\nWait for a thread and return its result."},
    {"thread_is_done",
     "thread_is_done(handle)\nReturn whether a thread has completed."},
    {"thread_count",
     "thread_count()\nReturn the number of started threads not yet joined."},
    {"gc", "gc()\nRequest/run a garbage-collection cycle and return its "
           "runtime-specific result."},
    {"py",
     "py(source, variables?)\nExecute Python source from a temporary script "
     "using bundled Python; optional map injects primitive variables."},
    {"native", "native(name, ...)\nInvoke a native integration entry point. "
               "Availability and safety depend on the build and environment."},
    {"py_inline", "py_inline(source)\nExecute inline Python integration code; "
                  "unsafe for untrusted input."},
    {"c_inline", "c_inline(source)\nCompile/execute inline C integration code "
                 "where supported; arbitrary code execution is possible."},
    {"asm_inline",
     "asm_inline(source)\nInvoke inline assembly integration where supported; "
     "highly platform-dependent and unsafe for untrusted input."}};
static const char *builtin_help(const char *name) {
  for (size_t i = 0; i < sizeof(builtin_docs) / sizeof(builtin_docs[0]); i++)
    if (strcmp(name, builtin_docs[i].name) == 0)
      return builtin_docs[i].doc;
  return NULL;
}

static Value eval_impl(Node *n, Env *env) {
  if (!n || had_error || has_exception)
    return v_null();
  switch (n->type) {
  case N_NUMBER:
    return n->number_text ? numeric_literal_value(n->number_text, n->line)
                          : v_num(n->as.number);
  case N_STRING:
    return v_string(n->as.string);
  case N_BOOL:
    return v_bool(n->as.boolean);
  case N_NULL:
    return v_null();
  case N_ARRAY: {
    Value v = v_array();
    for (int i = 0; i < n->as.array.count; i++) {
      Value item = eval(n->as.array.items[i], env);
      Array *a = v.as.array;
      if (a->count == a->capacity) {
        a->capacity = a->capacity ? a->capacity * 2 : 8;
        a->items =
            (Value *)xrealloc(a->items, (size_t)a->capacity * sizeof(Value));
      }
      a->items[a->count++] = copy_value(item);
      free_value(item);
    }
    return v;
  }
  case N_MAP: {
    Value v = v_map();
    for (int i = 0; i < n->as.map.count; i++) {
      Value k = eval(n->as.map.keys[i], env),
            x = eval(n->as.map.values[i], env);
      map_set(v.as.map, k, x);
      free_value(k);
      free_value(x);
    }
    return v;
  }
  case N_ADDRESS: {
    Node *target = n->as.pointer_stmt.target;
    SafePointer *p = NULL;
    if (target->type == N_VAR) {
      Binding *b = find_binding(env, target->as.var.name);
      if (!b) {
        runtime_error(n->line,
                      "Cannot take address of undefined variable '%s'.",
                      target->as.var.name);
        return v_null();
      }
      p = new_safe_pointer(PTR_BINDING);
      p->env = env;
      while (p->env && find_local(p->env, target->as.var.name) == NULL)
        p->env = p->env->parent;
      p->name = copy_s(target->as.var.name);
      if (p->env) {
        for (int pi = 0; pi < p->env->count; pi++)
          if (strcmp(p->env->bindings[pi].name, target->as.var.name) == 0) {
            p->index = pi;
            break;
          }
      }
    } else if (target->type == N_INDEX) {
      Value object = eval(target->as.index.object, env);
      if (had_error) {
        free_value(object);
        return v_null();
      }
      Value ix = eval(target->as.index.index, env);
      if (had_error) {
        free_value(object);
        free_value(ix);
        return v_null();
      }
      if (object.type == V_MAP || object.type == V_CLASS ||
          object.type == V_STRUCT_DEF) {
        if (map_find(object.as.map, ix) < 0) {
          runtime_error(n->line, "Cannot take address of a missing map key.");
          free_value(object);
          free_value(ix);
          return v_null();
        }
        p = new_safe_pointer(PTR_MAP_ENTRY);
        p->map = object.as.map;
        p->key = copy_value(ix);
        p->index = map_find(object.as.map, ix);
      } else {
        long k = 0;
        if (object.type == V_ARRAY && numeric_to_long(ix, &k)) {
          if (k < 0)
            k += object.as.array->count;
          if (k < 0 || k >= object.as.array->count) {
            runtime_error(n->line,
                          "Cannot take address of an out-of-range array item.");
            free_value(object);
            free_value(ix);
            return v_null();
          }
          p = new_safe_pointer(PTR_ARRAY_ITEM);
          p->array = object.as.array;
          p->index = (int)k;
        } else {
          runtime_error(n->line, "Address-of property/index requires a map key "
                                 "or valid array index.");
          free_value(object);
          free_value(ix);
          return v_null();
        }
      }
      free_value(object);
      free_value(ix);
    } else {
      runtime_error(
          n->line,
          "Address-of supports variables, map properties, and array elements.");
      return v_null();
    }
    return pointer_value(p);
  }
  case N_DEREF: {
    Value pv = eval(n->as.pointer_stmt.target, env);
    if (pv.type != V_POINTER) {
      runtime_error(n->line, "Cannot dereference a non-pointer value.");
      free_value(pv);
      return v_null();
    }
    Value out = v_null();
    (void)pointer_read(pv.as.pointer, &out, n->line);
    return out;
  }
  case N_DEREF_ASSIGN: {
    Value pv = eval(n->as.deref_assign.pointer, env);
    Value value = eval(n->as.deref_assign.value, env);
    if (pv.type != V_POINTER) {
      runtime_error(n->line, "Pointer assignment requires a safe pointer.");
      free_value(pv);
      free_value(value);
      return v_null();
    }
    (void)pointer_write(pv.as.pointer, value, n->line);
    return value;
  }
  case N_DEREF_COMPOUND_ASSIGN: {
    Value pv = eval(n->as.deref_compound_assign.pointer, env);
    if (pv.type != V_POINTER) {
      runtime_error(n->line,
                    "Pointer compound assignment requires a safe pointer.");
      free_value(pv);
      return v_null();
    }
    Value old = v_null();
    if (!pointer_read(pv.as.pointer, &old, n->line)) {
      free_value(pv);
      return v_null();
    }
    Value rhs = eval(n->as.deref_compound_assign.value, env);
    if (had_error) {
      free_value(pv);
      free_value(old);
      free_value(rhs);
      return v_null();
    }
    Value result = v_null();
    FluorineToken op = n->as.deref_compound_assign.op;
    if (op == T_PLUS && (old.type == V_STRING || rhs.type == V_STRING)) {
      char *a = value_to_string(old), *b = value_to_string(rhs);
      size_t z = strlen(a) + strlen(b) + 1;
      char *out = (char *)malloc(z);
      if (!out)
        exit(2);
      strcpy(out, a);
      strcat(out, b);
      result = v_string(out);
      free(out);
      free(a);
      free(b);
    } else if (!is_numeric(old) || !is_numeric(rhs)) {
      runtime_error(n->line, "Pointer compound assignment requires numeric "
                             "values, except string concatenation with '+='.");
    } else
      result = numeric_binary(op, old, rhs, n->line);
    if (!had_error)
      (void)pointer_write(pv.as.pointer, result, n->line);
    free_value(pv);
    free_value(old);
    free_value(rhs);
    return result;
  }
  case N_INDEX: {
    if (n->as.index.object && n->as.index.object->type == N_VAR &&
        n->as.index.index && n->as.index.index->type == N_STRING &&
        strcmp(n->as.index.index->as.string, "help") == 0) {
      const char *name = n->as.index.object->as.var.name;
      static const char *builtins[] = {"sqrt",
                                       "abs",
                                       "floor",
                                       "ceil",
                                       "round",
                                       "sin",
                                       "cos",
                                       "tan",
                                       "asin",
                                       "acos",
                                       "atan",
                                       "log",
                                       "log10",
                                       "exp",
                                       "min",
                                       "max",
                                       "pow",
                                       "input",
                                       "printf",
                                       "print",
                                       "is_number",
                                       "len",
                                       "push",
                                       "pop",
                                       "range",
                                       "py",
                                       "native",
                                       "keys",
                                       "has_key",
                                       "py_inline",
                                       "c_inline",
                                       "asm_inline",
                                       "gc",
                                       "to_number",
                                       "to_string",
                                       "type_of",
                                       "ascii_array",
                                       "substring",
                                       "split",
                                       "join",
                                       "trim",
                                       "upper",
                                       "lower",
                                       "contains",
                                       "starts_with",
                                       "ends_with",
                                       "replace",
                                       "ord",
                                       "chr",
                                       "random",
                                       "read_text",
                                       "write_text",
                                       "append_text",
                                       "file_exists",
                                       "remove_file",
                                       "file_size",
                                       "copy_file",
                                       "rename_file",
                                       "make_dir",
                                       "is_dir",
                                       "env",
                                       "set_env",
                                       "clock_ms",
                                       "sleep_ms",
                                       "assert",
                                       "http_serve",
                                       "http_request",
                                       "http_get",
                                       "thread_start",
                                       "thread_join",
                                       "thread_is_done",
                                       "thread_count",
                                       "url_encode",
                                       "url_decode",
                                       "path_join",
                                       "basename",
                                       "dirname",
                                       "unix_time",
                                       "time_string",
                                       "seed_random",
                                       "os_os",
                                       "os_system",
                                       "os_shellexec",
                                       "os_name",
                                       "os_version",
                                       "os_architecture",
                                       "os_hostname",
                                       "os_username",
                                       "os_cwd",
                                       "os_path_join",
                                       "os_basename",
                                       "os_dirname",
                                       "os_cd",
                                       "os_chdir",
                                       "os_mkdir",
                                       "os_makedirs",
                                       "os_rmdir",
                                       "os_exists",
                                       "os_is_file",
                                       "os_is_dir",
                                       "os_remove",
                                       "os_unlink",
                                       "os_rename",
                                       "os_copy_file",
                                       "os_getenv",
                                       "os_setenv",
                                       "os_environ",
                                       "os_listdir",
                                       "os_home_dir",
                                       "os_temp_dir",
                                       "os_executable_path",
                                       "os_process_id",
                                       "os_sleep",
                                       "os_timestamp",
                                       "os_execute",
                                       "os_uptime",
                                       "os_last_error",
                                       "os_shell_execute",
                                       "os_open_url",
                                       "os_open_file",
                                       "os_open_folder",
                                       "os_spawn",
                                       "os_wait",
                                       "os_terminate",
                                       "os_message_box",
                                       "os_lock_screen",
                                       "os_logoff",
                                       "os_shutdown",
                                       "os_restart",
                                       "os_is_admin",
                                       "shellexec",
                                       "bit_and",
                                       "bit_or",
                                       "bit_xor",
                                       "bit_not",
                                       "bit_shl",
                                       "bit_shr",
                                       "byte_get",
                                       "byte_set",
                                       "read_u16_le",
                                       "read_u16_be",
                                       "read_u32_le",
                                       "read_u32_be",
                                       "read_u64_le",
                                       "read_u64_be",
                                       "write_u16_le",
                                       "write_u16_be",
                                       "write_u32_le",
                                       "write_u32_be",
                                       "write_u64_le",
                                       "write_u64_be",
                                       "mouse_move",
                                       "mouse_left_click",
                                       "mouse_right_click",
                                       "mouse_middle_click",
                                       "mouse_left_click_at",
                                       "mouse_right_click_at",
                                       "mouse_position",
                                       "mouse_scroll",
                                       "mouse_smooth_move",
                                       "mouse_set_position",
                                       "mouse_double_click",
                                       "mouse_press",
                                       "mouse_release",
                                       "mouse_is_button_down",
                                       "mouse_screen_size",
                                       "mouse_show_cursor",
                                       "mouse_set_system_cursor",
                                       "mouse_restore_system_cursor",
                                       "mouse_is_available",
                                       "mouse_wait_button",
                                       "keyboard_wait_key",
                                       "keyboard_is_key_down",
                                       "keyboard_type_text",
                                       "keyboard_press",
                                       "keyboard_key_down",
                                       "keyboard_key_up",
                                       "std_format_convert",
                                       "std_uuid_v4",
                                       "std_uuid_v7",
                                       "std_sha256",
                                       "std_json_stringify",
                                       "std_json_parse",
                                       "std_datetime_iso",
                                       "std_datetime_parts",
                                       "std_cli_args",
                                       "std_csv_parse",
                                       "std_toml_parse",
                                       "std_yaml_parse"};
      for (size_t bi = 0; bi < sizeof(builtins) / sizeof(builtins[0]); bi++)
        if (strcmp(name, builtins[bi]) == 0) {
          const char *description = builtin_help(name);
          if (description)
            return v_string(description);
          char doc[256];
          snprintf(doc, sizeof(doc),
                   "%s(...)\nBuilt-in function. See docs/standard-library.md "
                   "for accepted arguments and return value.",
                   name);
          return v_string(doc);
        }
    }
    Value object = eval(n->as.index.object, env);
    if (had_error) {
      free_value(object);
      return v_null();
    }
    Value ix = eval(n->as.index.index, env);
    if (had_error) {
      free_value(object);
      free_value(ix);
      return v_null();
    }
    long ix_long = 0;
    bool ix_is_integer = numeric_to_long(ix, &ix_long);
    if (ix.type == V_STRING && strcmp(ix.as.string, "help") == 0 &&
        (object.type == V_FUNCTION || object.type == V_NATIVE)) {
      char doc[2048];
      doc[0] = '\0';
      if (object.type == V_FUNCTION && object.as.function &&
          object.as.function->declaration) {
        Node *d = object.as.function->declaration;
        size_t used = (size_t)snprintf(doc, sizeof(doc), "%s(",
                                       d->as.function.name ? d->as.function.name
                                                           : "function");
        for (int di = 0; di < d->as.function.param_count && used < sizeof(doc);
             di++) {
          int wrote = snprintf(doc + used, sizeof(doc) - used, "%s%s",
                               di ? ", " : "", d->as.function.params[di]);
          if (wrote < 0)
            break;
          used += (size_t)wrote;
        }
        if (used < sizeof(doc)) {
          const char *description = d->as.function.doc && *d->as.function.doc
                                        ? d->as.function.doc
                                        : "No documentation provided.";
          snprintf(doc + used, sizeof(doc) - used, ")\n%s", description);
        }
      } else {
        const char *nm =
            (n->as.index.object && n->as.index.object->type == N_VAR)
                ? n->as.index.object->as.var.name
                : "native function";
        const char *description = builtin_help(nm);
        if (description)
          snprintf(doc, sizeof(doc), "%s", description);
        else
          snprintf(doc, sizeof(doc),
                   "%s(...)\nBuilt-in function. See docs/standard-library.md "
                   "for accepted arguments and return value.",
                   nm);
      }
      free_value(object);
      free_value(ix);
      return v_string(doc);
    }
    if (object.type == V_MAP || object.type == V_CLASS ||
        object.type == V_STRUCT_DEF) {
      int k = map_find(object.as.map, ix);
      if (k < 0) {
        runtime_error(n->line, "Map key not found.");
        free_value(object);
        free_value(ix);
        return v_null();
      }
      Value result = copy_value(object.as.map->items[k].value);
      free_value(object);
      free_value(ix);
      return result;
    }
    if (object.type == V_ARRAY && ix_is_integer) {
      long k = ix_long;
      if (k < 0)
        k += object.as.array->count;
      if (k < 0 || k >= object.as.array->count) {
        runtime_error(n->line, "Array index %ld out of range (length %d).", k,
                      object.as.array->count);
        free_value(object);
        free_value(ix);
        return v_null();
      }
      Value result = copy_value(object.as.array->items[k]);
      free_value(object);
      free_value(ix);
      return result;
    }
    if (object.type == V_STRING && ix_is_integer) {
      size_t len = strlen(object.as.string);
      long k = ix_long;
      if (k < 0)
        k += (long)len;
      if (k < 0 || (size_t)k >= len) {
        runtime_error(n->line, "String index %ld out of range (length %lu).", k,
                      (unsigned long)len);
        free_value(object);
        free_value(ix);
        return v_null();
      }
      char ch[2] = {object.as.string[k], 0};
      Value result = v_string(ch);
      free_value(object);
      free_value(ix);
      return result;
    }
    runtime_error(n->line, "Indexing requires an array or string with integer "
                           "index, or map with key.");
    free_value(object);
    free_value(ix);
    return v_null();
  }
  case N_INDEX_ASSIGN: {
    Value object = eval(n->as.index_assign.object, env);
    if (had_error) {
      free_value(object);
      return v_null();
    }
    Value ix = eval(n->as.index_assign.index, env);
    if (had_error) {
      free_value(object);
      free_value(ix);
      return v_null();
    }
    long ix_long = 0;
    bool ix_is_integer = numeric_to_long(ix, &ix_long);
    Value value = eval(n->as.index_assign.value, env);
    if (had_error) {
      free_value(object);
      free_value(ix);
      free_value(value);
      return v_null();
    }
    if (object.type == V_MAP || object.type == V_CLASS ||
        object.type == V_STRUCT_DEF) {
      map_set(object.as.map, ix, value);
      Value result = copy_value(value);
      free_value(object);
      free_value(ix);
      free_value(value);
      return result;
    }
    if (object.type != V_ARRAY || !ix_is_integer) {
      runtime_error(n->line,
                    "Indexed assignment requires array/integer or map/key.");
      free_value(object);
      free_value(ix);
      free_value(value);
      return v_null();
    }
    long k = ix_long;
    if (k < 0)
      k += object.as.array->count;
    if (k < 0 || k >= object.as.array->count) {
      runtime_error(n->line, "Array index %ld out of range (length %d).", k,
                    object.as.array->count);
      free_value(object);
      free_value(ix);
      free_value(value);
      return v_null();
    }
    free_value(object.as.array->items[k]);
    object.as.array->items[k] = copy_value(value);
    Value result = copy_value(value);
    free_value(object);
    free_value(ix);
    free_value(value);
    return result;
  }
  case N_COMPOUND_INDEX_ASSIGN: {
    Value object = eval(n->as.compound_index_assign.object, env);
    if (had_error) {
      free_value(object);
      return v_null();
    }
    Value ix = eval(n->as.compound_index_assign.index, env);
    if (had_error) {
      free_value(object);
      free_value(ix);
      return v_null();
    }
    long ix_long = 0;
    bool ix_is_integer = numeric_to_long(ix, &ix_long);
    Value old = v_null();
    int array_index = -1;
    int map_index = -1;
    if (object.type == V_MAP || object.type == V_CLASS ||
        object.type == V_STRUCT_DEF) {
      map_index = map_find(object.as.map, ix);
      if (map_index < 0) {
        runtime_error(n->line, "Map key not found for compound assignment.");
        free_value(object);
        free_value(ix);
        return v_null();
      }
      old = copy_value(object.as.map->items[map_index].value);
    } else if (object.type == V_ARRAY && ix_is_integer) {
      long k = ix_long;
      if (k < 0)
        k += object.as.array->count;
      if (k < 0 || k >= object.as.array->count) {
        runtime_error(n->line, "Array index %ld out of range (length %d).", k,
                      object.as.array->count);
        free_value(object);
        free_value(ix);
        return v_null();
      }
      array_index = (int)k;
      old = copy_value(object.as.array->items[array_index]);
    } else {
      runtime_error(n->line, "Compound indexed assignment requires an "
                             "array/integer index or map/property key.");
      free_value(object);
      free_value(ix);
      return v_null();
    }
    Value rhs = eval(n->as.compound_index_assign.value, env);
    if (had_error) {
      free_value(object);
      free_value(old);
      free_value(ix);
      free_value(rhs);
      return v_null();
    }
    Value result = v_null();
    FluorineToken op = n->as.compound_index_assign.op;
    if (op == T_PLUS && (old.type == V_STRING || rhs.type == V_STRING)) {
      char *a = value_to_string(old), *b = value_to_string(rhs);
      size_t len = strlen(a) + strlen(b) + 1;
      char *joined = (char *)malloc(len);
      if (!joined)
        exit(2);
      strcpy(joined, a);
      strcat(joined, b);
      result = v_string(joined);
      free(joined);
      free(a);
      free(b);
    } else if (!is_numeric(old) || !is_numeric(rhs)) {
      runtime_error(n->line, "Compound arithmetic assignment requires numbers, "
                             "except string concatenation with '+='.");
    } else
      result = numeric_binary(op, old, rhs, n->line);
    if (!had_error) {
      if (map_index >= 0)
        map_set(object.as.map, ix, result);
      else {
        free_value(object.as.array->items[array_index]);
        object.as.array->items[array_index] = copy_value(result);
      }
    }
    free_value(object);
    free_value(old);
    free_value(ix);
    free_value(rhs);
    return result;
  }
  case N_VAR: {
    Binding *b = find_binding(env, n->as.var.name);
    if (b)
      return copy_value(b->value);
    /* Built-in constants and functions. */
    if (strcmp(n->as.var.name, "pi") == 0)
      return v_num(3.14159265358979323846);
    if (strcmp(n->as.var.name, "e") == 0)
      return v_num(2.71828182845904523536);
    static const char *names[] = {
        "sqrt",     "abs",        "floor",     "ceil", "round",   "sin",
        "cos",      "tan",        "asin",      "acos", "atan",    "log",
        "log10",    "exp",        "min",       "max",  "pow",     "input",
        "printf",   "print",      "is_number", "len",  "push",    "pop",
        "range",    "py",         "native",    "keys", "has_key", "py_inline",
        "c_inline", "asm_inline", "gc"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
      if (strcmp(n->as.var.name, names[i]) == 0) {
        /* Store a native function name in a stable tiny registry. */
        extern Value Fluorine_native_dispatch(int, Value *, int);
        (void)Fluorine_native_dispatch;
        /* Native dispatch uses a per-name wrapper table below. */
        break;
      }
    runtime_error(n->line, "Undefined variable '%s'.", n->as.var.name);
    return v_null();
  }
  case N_ASSIGN: {
    Value v = eval(n->as.assign.value, env);
    if (!had_error)
      assign_var(env, n->as.assign.name, v, n->line);
    return v;
  }
  case N_UPDATE: {
    Node *t = n->as.update.target;
    int delta = n->as.update.delta;
    Value old = v_null(), next = v_null();
    if (t->type == N_VAR) {
      Binding *b = find_binding(env, t->as.var.name);
      if (!b) {
        runtime_error(n->line, "Undefined variable '%s'.", t->as.var.name);
        return v_null();
      }
      old = copy_value(b->value);
      if (is_numeric(old)) {
        Value one = v_bigint("1");
        next = numeric_binary(delta > 0 ? T_PLUS : T_MINUS, old, one, n->line);
        free_value(one);
      } else if (old.type == V_POINTER && old.as.pointer) {
        SafePointer *moved = clone_safe_pointer(old.as.pointer);
        if (!pointer_move(moved, delta, n->line)) {
          free_value(old);
          return v_null();
        }
        next = pointer_value(moved);
      } else {
        runtime_error(
            n->line,
            "Increment/decrement requires a number or a safe pointer.");
        free_value(old);
        return v_null();
      }
      if (is_numeric(old) || old.type == V_POINTER) {
        free_value(b->value);
        b->value = copy_value(next);
      }
    } else if (t->type == N_INDEX) {
      Value obj = eval(t->as.index.object, env);
      if (had_error)
        return v_null();
      Value ix = eval(t->as.index.index, env);
      if (had_error) {
        free_value(ix);
        return v_null();
      }
      long ix_long = 0;
      bool ix_is_integer = numeric_to_long(ix, &ix_long);
      Value *slot = NULL;
      if (obj.type == V_ARRAY && ix_is_integer) {
        long k = ix_long;
        if (k < 0)
          k += obj.as.array->count;
        if (k < 0 || k >= obj.as.array->count) {
          runtime_error(n->line,
                        "Increment/decrement array index out of range.");
          free_value(obj);
          free_value(ix);
          return v_null();
        }
        slot = &obj.as.array->items[k];
      } else if (obj.type == V_MAP || obj.type == V_CLASS ||
                 obj.type == V_STRUCT_DEF) {
        int k = map_find(obj.as.map, ix);
        if (k < 0) {
          runtime_error(n->line,
                        "Increment/decrement map property does not exist.");
          free_value(obj);
          free_value(ix);
          return v_null();
        }
        slot = &obj.as.map->items[k].value;
      } else {
        runtime_error(n->line, "Increment/decrement target must be an array "
                               "element or map property.");
        free_value(obj);
        free_value(ix);
        return v_null();
      }
      old = copy_value(*slot);
      if (is_numeric(old)) {
        Value one = v_bigint("1");
        next = numeric_binary(delta > 0 ? T_PLUS : T_MINUS, old, one, n->line);
        free_value(one);
      } else if (old.type == V_POINTER && old.as.pointer) {
        SafePointer *moved = clone_safe_pointer(old.as.pointer);
        if (!pointer_move(moved, delta, n->line)) {
          free_value(obj);
          free_value(old);
          free_value(ix);
          return v_null();
        }
        next = pointer_value(moved);
      } else {
        runtime_error(
            n->line,
            "Increment/decrement requires a number or safe pointer value.");
        free_value(obj);
        free_value(old);
        free_value(ix);
        return v_null();
      }
      free_value(*slot);
      *slot = copy_value(next);
      free_value(obj);
      free_value(ix);
    } else if (t->type == N_DEREF) {
      Value pv = eval(t->as.pointer_stmt.target, env);
      if (pv.type != V_POINTER) {
        runtime_error(
            n->line,
            "Increment/decrement dereference requires a safe pointer.");
        free_value(pv);
        return v_null();
      }
      if (!pointer_read(pv.as.pointer, &old, n->line)) {
        free_value(pv);
        return v_null();
      }
      if (!is_numeric(old)) {
        runtime_error(
            n->line,
            "Increment/decrement dereference requires a numeric target.");
        free_value(old);
        free_value(pv);
        return v_null();
      }
      {
        Value one = v_bigint("1");
        next = numeric_binary(delta > 0 ? T_PLUS : T_MINUS, old, one, n->line);
        free_value(one);
      }
      (void)pointer_write(pv.as.pointer, next, n->line);
      free_value(pv);
    }
    Value result = n->as.update.prefix ? copy_value(next) : copy_value(old);
    free_value(old);
    free_value(next);
    return result;
  }
  case N_UNARY: {
    Value r = eval(n->as.unary.right, env);
    if (n->as.unary.op == T_BANG) {
      bool result = !truthy(r);
      free_value(r);
      return v_bool(result);
    }
    if (!is_numeric(r)) {
      runtime_error(n->line, "Unary '%s' requires a number.",
                    n->as.unary.op == T_MINUS ? "-" : "+");
      free_value(r);
      return v_null();
    }
    if (is_exact_numeric(r)) {
      if (n->as.unary.op == T_MINUS) {
        if (r.type == V_BIGINT) {
          char *z = bi_neg(r.as.bigint);
          Value v = v_bigint(z);
          free(z);
          free_value(r);
          return v;
        }
        char *z = bi_neg(r.as.rational.num);
        Value v = v_ratio(z, r.as.rational.den);
        free(z);
        free_value(r);
        return v;
      }
      return r;
    }
    double result = n->as.unary.op == T_MINUS ? -r.as.number : r.as.number;
    free_value(r);
    return v_num(result);
  }
  case N_BINARY: {
    Value a = eval(n->as.binary.left, env);
    if (had_error) {
      free_value(a);
      return v_null();
    }
    if (n->as.binary.op == T_AND_AND && !truthy(a)) {
      free_value(a);
      return v_bool(false);
    }
    if (n->as.binary.op == T_OR_OR && truthy(a)) {
      free_value(a);
      return v_bool(true);
    }
    Value b = eval(n->as.binary.right, env);
    if (had_error) {
      free_value(a);
      free_value(b);
      return v_null();
    }
    long pointer_offset = 0;
    bool valid_pointer_offset = numeric_to_long(b, &pointer_offset);
    if ((n->as.binary.op == T_PLUS || n->as.binary.op == T_MINUS) &&
        a.type == V_POINTER && a.as.pointer && valid_pointer_offset &&
        pointer_offset >= INT_MIN && pointer_offset <= INT_MAX) {
      int amount = (int)pointer_offset;
      if (n->as.binary.op == T_MINUS)
        amount = -amount;
      SafePointer *moved = clone_safe_pointer(a.as.pointer);
      Value result = v_null();
      if (pointer_move(moved, amount, n->line))
        result = pointer_value(moved);
      free_value(a);
      free_value(b);
      return result;
    }
    if (n->as.binary.op == T_MINUS && a.type == V_POINTER &&
        b.type == V_POINTER && a.as.pointer && b.as.pointer &&
        a.as.pointer->kind == b.as.pointer->kind) {
      bool same = (a.as.pointer->kind == PTR_BINDING &&
                   a.as.pointer->env == b.as.pointer->env) ||
                  (a.as.pointer->kind == PTR_MAP_ENTRY &&
                   a.as.pointer->map == b.as.pointer->map) ||
                  (a.as.pointer->kind == PTR_ARRAY_ITEM &&
                   a.as.pointer->array == b.as.pointer->array);
      if (same) {
        double distance =
            (double)a.as.pointer->index - (double)b.as.pointer->index;
        free_value(a);
        free_value(b);
        return v_num(distance);
      }
    }
    FluorineToken op = n->as.binary.op;
    if (op == T_AND_AND || op == T_OR_OR) {
      bool result =
          op == T_AND_AND ? (truthy(a) && truthy(b)) : (truthy(a) || truthy(b));
      free_value(a);
      free_value(b);
      return v_bool(result);
    }
    if (op == T_EQUAL_EQUAL || op == T_BANG_EQUAL) {
      bool equal = equal_values(a, b);
      free_value(a);
      free_value(b);
      return v_bool(op == T_EQUAL_EQUAL ? equal : !equal);
    }
    if (op == T_PLUS) {
      if (a.type == V_STRING || b.type == V_STRING) {
        char *sa = value_to_string(a), *sb = value_to_string(b);
        size_t z = strlen(sa) + strlen(sb) + 1;
        char *out = (char *)malloc(z);
        if (!out)
          exit(2);
        strcpy(out, sa);
        strcat(out, sb);
        Value result = v_string(out);
        free(out);
        free(sa);
        free(sb);
        free_value(a);
        free_value(b);
        return result;
      }
      if (is_numeric(a) && is_numeric(b)) {
        Value result = numeric_binary(T_PLUS, a, b, n->line);
        free_value(a);
        free_value(b);
        return result;
      }
      runtime_error(n->line,
                    "The + operator can add two numbers or join text. It "
                    "received %s and %s.",
                    value_type_name(a), value_type_name(b));
      free_value(a);
      free_value(b);
      return v_null();
    }
    if (op == T_MINUS || op == T_STAR || op == T_SLASH || op == T_PERCENT ||
        op == T_CARET) {
      if (!is_numeric(a) || !is_numeric(b)) {
        runtime_error(
            n->line,
            "This math operation needs two numbers, but received %s and %s.",
            value_type_name(a), value_type_name(b));
        free_value(a);
        free_value(b);
        return v_null();
      }
      Value result = numeric_binary(op, a, b, n->line);
      free_value(a);
      free_value(b);
      return result;
    }
    if (op == T_LESS || op == T_LESS_EQUAL || op == T_GREATER ||
        op == T_GREATER_EQUAL) {
      int cmp = 0;
      if (is_numeric(a) && is_numeric(b))
        cmp = numeric_cmp(a, b);
      else if (a.type == V_STRING && b.type == V_STRING)
        cmp = strcmp(a.as.string, b.as.string);
      else {
        runtime_error(n->line,
                      "These values cannot be compared with <, <=, >, or >=: "
                      "received %s and %s. Use two numbers or two strings.",
                      value_type_name(a), value_type_name(b));
        free_value(a);
        free_value(b);
        return v_null();
      }
      free_value(a);
      free_value(b);
      if (op == T_LESS)
        return v_bool(cmp < 0);
      if (op == T_LESS_EQUAL)
        return v_bool(cmp <= 0);
      if (op == T_GREATER)
        return v_bool(cmp > 0);
      return v_bool(cmp >= 0);
    }
    free_value(a);
    free_value(b);
    return v_null();
  }
  case N_CALL: {
    Value callee;
    Value receiver = v_null();
    bool has_receiver = false;
    if (n->as.call.callee->type == N_INDEX) {
      Node *member = n->as.call.callee;
      receiver = eval(member->as.index.object, env);
      Value key = eval(member->as.index.index, env);
      if (receiver.type == V_MAP || receiver.type == V_CLASS ||
          receiver.type == V_STRUCT_DEF) {
        int mi = map_find(receiver.as.map, key);
        if (mi >= 0) {
          callee = copy_value(receiver.as.map->items[mi].value);
          has_receiver = true;
        } else {
          runtime_error(n->line, "Member does not exist.");
          free_value(key);
          free_value(receiver);
          return v_null();
        }
      } else {
        runtime_error(n->line, "Method call requires an object or namespace.");
        free_value(key);
        free_value(receiver);
        return v_null();
      }
      free_value(key);
    } else if (n->as.call.callee->type == N_VAR) {
      const char *name = n->as.call.callee->as.var.name;
      Binding *b = find_binding(env, name);
      if (b)
        callee = copy_value(b->value);
      else {
        static const char *native_names[] = {"sqrt",
                                             "abs",
                                             "floor",
                                             "ceil",
                                             "round",
                                             "sin",
                                             "cos",
                                             "tan",
                                             "asin",
                                             "acos",
                                             "atan",
                                             "log",
                                             "log10",
                                             "exp",
                                             "min",
                                             "max",
                                             "pow",
                                             "input",
                                             "printf",
                                             "print",
                                             "is_number",
                                             "len",
                                             "push",
                                             "pop",
                                             "range",
                                             "py",
                                             "native",
                                             "keys",
                                             "has_key",
                                             "py_inline",
                                             "c_inline",
                                             "asm_inline",
                                             "gc",
                                             "to_number",
                                             "to_string",
                                             "type_of",
                                             "ascii_array",
                                             "substring",
                                             "split",
                                             "join",
                                             "trim",
                                             "upper",
                                             "lower",
                                             "contains",
                                             "starts_with",
                                             "ends_with",
                                             "replace",
                                             "ord",
                                             "chr",
                                             "random",
                                             "read_text",
                                             "write_text",
                                             "append_text",
                                             "file_exists",
                                             "remove_file",
                                             "file_size",
                                             "copy_file",
                                             "rename_file",
                                             "make_dir",
                                             "is_dir",
                                             "env",
                                             "set_env",
                                             "clock_ms",
                                             "sleep_ms",
                                             "assert",
                                             "http_serve",
                                             "http_request",
                                             "http_get",
                                             "thread_start",
                                             "thread_join",
                                             "thread_is_done",
                                             "thread_count",
                                             "url_encode",
                                             "url_decode",
                                             "path_join",
                                             "basename",
                                             "dirname",
                                             "unix_time",
                                             "time_string",
                                             "seed_random",
                                             "os_os",
                                             "os_system",
                                             "os_shellexec",
                                             "os_name",
                                             "os_version",
                                             "os_architecture",
                                             "os_hostname",
                                             "os_username",
                                             "os_cwd",
                                             "os_path_join",
                                             "os_basename",
                                             "os_dirname",
                                             "os_cd",
                                             "os_chdir",
                                             "os_mkdir",
                                             "os_makedirs",
                                             "os_rmdir",
                                             "os_exists",
                                             "os_is_file",
                                             "os_is_dir",
                                             "os_remove",
                                             "os_unlink",
                                             "os_rename",
                                             "os_copy_file",
                                             "os_getenv",
                                             "os_setenv",
                                             "os_environ",
                                             "os_listdir",
                                             "os_home_dir",
                                             "os_temp_dir",
                                             "os_executable_path",
                                             "os_process_id",
                                             "os_sleep",
                                             "os_timestamp",
                                             "os_execute",
                                             "os_uptime",
                                             "os_last_error",
                                             "os_shell_execute",
                                             "os_open_url",
                                             "os_open_file",
                                             "os_open_folder",
                                             "os_spawn",
                                             "os_wait",
                                             "os_terminate",
                                             "os_message_box",
                                             "os_lock_screen",
                                             "os_logoff",
                                             "os_shutdown",
                                             "os_restart",
                                             "os_is_admin",
                                             "shellexec",
                                             "bit_and",
                                             "bit_or",
                                             "bit_xor",
                                             "bit_not",
                                             "bit_shl",
                                             "bit_shr",
                                             "byte_get",
                                             "byte_set",
                                             "read_u16_le",
                                             "read_u16_be",
                                             "read_u32_le",
                                             "read_u32_be",
                                             "read_u64_le",
                                             "read_u64_be",
                                             "write_u16_le",
                                             "write_u16_be",
                                             "write_u32_le",
                                             "write_u32_be",
                                             "write_u64_le",
                                             "write_u64_be",
                                             "mouse_move",
                                             "mouse_left_click",
                                             "mouse_right_click",
                                             "mouse_middle_click",
                                             "mouse_left_click_at",
                                             "mouse_right_click_at",
                                             "mouse_position",
                                             "mouse_scroll",
                                             "mouse_smooth_move",
                                             "mouse_set_position",
                                             "mouse_double_click",
                                             "mouse_press",
                                             "mouse_release",
                                             "mouse_is_button_down",
                                             "mouse_screen_size",
                                             "mouse_show_cursor",
                                             "mouse_set_system_cursor",
                                             "mouse_restore_system_cursor",
                                             "mouse_is_available",
                                             "mouse_wait_button",
                                             "keyboard_wait_key",
                                             "keyboard_is_key_down",
                                             "keyboard_type_text",
                                             "keyboard_press",
                                             "keyboard_key_down",
                                             "keyboard_key_up",
                                             "std_format_convert",
                                             "std_uuid_v4",
                                             "std_uuid_v7",
                                             "std_sha256",
                                             "std_json_stringify",
                                             "std_json_parse",
                                             "std_datetime_iso",
                                             "std_datetime_parts",
                                             "std_cli_args",
                                             "std_csv_parse",
                                             "std_toml_parse",
                                             "std_yaml_parse"};
        bool found = false;
        for (size_t i = 0; i < sizeof(native_names) / sizeof(native_names[0]);
             i++)
          if (strcmp(name, native_names[i]) == 0) {
            callee.type = V_NATIVE;
            callee.as.native = NULL;
            found = true;
            break;
          }
        if (!found && strcmp(name, "pi") == 0) {
          runtime_error(n->line, "pi is a numeric constant, not a function.");
          return v_null();
        }
        if (!found && strcmp(name, "e") == 0) {
          runtime_error(n->line, "e is a numeric constant, not a function.");
          return v_null();
        }
        if (!found) {
          runtime_error(n->line, "Undefined function '%s'.", name);
          return v_null();
        }
        Value *args = (Value *)calloc(
            (size_t)(n->as.call.count ? n->as.call.count : 1), sizeof(Value));
        if (!args)
          exit(2);
        for (int i = 0; i < n->as.call.count; i++)
          args[i] = eval(n->as.call.args[i], env);
        Value result = native_call(name, n->as.call.count, args, n->line);
        for (int i = 0; i < n->as.call.count; i++) {
          free_value(args[i]);
        }
        free(args);
        return result;
      }
    } else
      callee = eval(n->as.call.callee, env);
    Value *args = (Value *)calloc(
        (size_t)(n->as.call.count ? n->as.call.count : 1), sizeof(Value));
    if (!args)
      exit(2);
    for (int i = 0; i < n->as.call.count; i++)
      args[i] = eval(n->as.call.args[i], env);
    Value result = call_value(callee, n->as.call.count, args, n->line, receiver,
                              has_receiver);
    free_value(callee);
    free_value(receiver);
    for (int i = 0; i < n->as.call.count; i++) {
      free_value(args[i]);
    }
    free(args);
    return result;
  }
  default:
    runtime_error(n->line, "Internal error: node is not an expression.");
    return v_null();
  }
}

static bool require_numbers(const char *name, int argc, Value *a, int line,
                            int min, int max) {
  if (argc < min || argc > max) {
    runtime_error(line, "%s expects %s%d argument(s).", name,
                  min == max ? "exactly " : "at least ", min);
    return false;
  }
  for (int i = 0; i < argc; i++) {
    if (a[i].type == V_BOOL) {
      a[i] = v_num(a[i].as.boolean ? 1.0 : 0.0);
      continue;
    }
    if (a[i].type == V_STRING) {
      uint32_t cp = 0;
      if (utf8_single_codepoint(a[i].as.string, &cp)) {
        free_value(a[i]);
        a[i] = v_num((double)cp);
        continue;
      }
      if (valid_numeric_text(a[i].as.string)) {
        errno = 0;
        char *end = NULL;
        double d = strtod(a[i].as.string, &end);
        if (errno != ERANGE && end && *end == '\0' && isfinite(d)) {
          free_value(a[i]);
          a[i] = v_num(d);
          continue;
        }
      }
    }
    if (!is_numeric(a[i])) {
      runtime_error(line,
                    "%s expects numbers; numeric strings, single Unicode "
                    "characters, and booleans are converted automatically.",
                    name);
      return false;
    }
  }
  return true;
}
#define FLUORINE_PATH_CAP 4096
#define FLUORINE_COMMAND_CAP 32768

/* Launch child processes without a shell. This avoids shell expansion and
 * quoting bugs when bundled paths or arguments contain spaces/metacharacters.
 */
#ifdef _WIN32
static bool append_windows_argument(char *command, size_t capacity,
                                    size_t *used, const char *arg) {
  size_t slashes = 0;
  if (!command || !used || !arg || *used >= capacity)
    return false;
  if (*used + 1 >= capacity)
    return false;
  command[(*used)++] = '"';
  for (const char *p = arg;; ++p) {
    if (*p == '\\') {
      ++slashes;
      continue;
    }
    if (*p == '"') {
      /* Every backslash before a quote is doubled, then one extra slash
       * escapes the quote itself under the Windows CRT parsing rules. */
      size_t need = slashes * 2 + 2;
      if (need > capacity - *used - 1)
        return false;
      for (size_t i = 0; i < slashes * 2 + 1; ++i)
        command[(*used)++] = '\\';
      slashes = 0;
      command[(*used)++] = '"';
      continue;
    }
    if (*p == '\0') {
      /* Backslashes immediately before the closing quote are doubled. */
      size_t need = slashes * 2 + 1;
      if (need > capacity - *used - 1)
        return false;
      for (size_t i = 0; i < slashes * 2; ++i)
        command[(*used)++] = '\\';
      command[(*used)++] = '"';
      command[*used] = '\0';
      return true;
    }
    if (slashes + 1 > capacity - *used - 1)
      return false;
    for (size_t i = 0; i < slashes; ++i)
      command[(*used)++] = '\\';
    slashes = 0;
    command[(*used)++] = *p;
  }
}
#endif

static int run_process(const char *const argv[]) {
  if (!argv || !argv[0] || !*argv[0])
    return -1;
#ifdef _WIN32
  char command[FLUORINE_COMMAND_CAP];
  size_t used = 0;
  command[0] = '\0';
  for (size_t i = 0; argv[i]; ++i) {
    if (i && used + 1 >= sizeof(command))
      return -1;
    if (i)
      command[used++] = ' ';
    if (!append_windows_argument(command, sizeof(command), &used, argv[i]))
      return -1;
  }
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(si));
  memset(&pi, 0, sizeof(pi));
  si.cb = sizeof(si);
  if (!CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    return -1;
  DWORD wait_result = WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  bool ok = wait_result == WAIT_OBJECT_0 &&
            GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return ok ? (int)exit_code : -1;
#else
  pid_t child = fork();
  if (child < 0)
    return -1;
  if (child == 0) {
    execv(argv[0], (char *const *)argv);
    _exit(errno == ENOENT ? 127 : 126);
  }
  int status;
  pid_t waited;
  do {
    waited = waitpid(child, &status, 0);
  } while (waited < 0 && errno == EINTR);
  if (waited < 0)
    return -1;
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);
  return -1;
#endif
}

static bool safe_relative_path(const char *path);

/* Resolve bundled files from the installation containing this executable,
 * never from the caller's current working directory. */
static char fluorine_executable_directory[FLUORINE_PATH_CAP] = ".";
static char fluorine_install_directory[FLUORINE_PATH_CAP] = ".";

static bool path_join(char *out, size_t capacity, const char *base,
                      const char *relative) {
  size_t n;
  int written;
  if (!out || !capacity || !base || !relative)
    return false;
  n = strlen(base);
  if (n && (base[n - 1] == '/' || base[n - 1] == '\\'))
    written = snprintf(out, capacity, "%s%s", base, relative);
  else
    written = snprintf(out, capacity, "%s/%s", base, relative);
  return written >= 0 && (size_t)written < capacity;
}

static bool set_executable_directory(const char *argv0) {
  char executable[FLUORINE_PATH_CAP];
  char directory[FLUORINE_PATH_CAP];
  char *separator;
#ifdef _WIN32
  /* Windows obtains the executable path from the OS; argv0 is only a fallback
   * on POSIX systems. Keep strict warning builds clean on Windows. */
  (void)argv0;
  DWORD length =
      GetModuleFileNameA(NULL, executable, (DWORD)sizeof(executable));
  if (length == 0 || length >= sizeof(executable))
    return false;
  executable[length] = '\0';
#else
  ssize_t length =
      readlink("/proc/self/exe", executable, sizeof(executable) - 1);
  if (length >= 0 && (size_t)length < sizeof(executable) - 1) {
    executable[length] = '\0';
  } else {
    bool found = false;
    if (argv0 && strchr(argv0, '/')) {
      if (realpath(argv0, executable))
        found = true;
    } else if (argv0 && *argv0) {
      const char *path_env = getenv("PATH");
      if (path_env) {
        const char *part = path_env;
        while (!found) {
          const char *end = strchr(part, ':');
          size_t part_len = end ? (size_t)(end - part) : strlen(part);
          char candidate[FLUORINE_PATH_CAP];
          char dirpart[FLUORINE_PATH_CAP];
          if (part_len == 0) {
            strcpy(dirpart, ".");
          } else if (part_len < sizeof(dirpart)) {
            memcpy(dirpart, part, part_len);
            dirpart[part_len] = '\0';
          } else {
            dirpart[0] = '\0';
          }
          if (dirpart[0] &&
              path_join(candidate, sizeof(candidate), dirpart, argv0) &&
              access(candidate, X_OK) == 0 && realpath(candidate, executable)) {
            found = true;
          }
          if (!end)
            break;
          part = end + 1;
        }
      }
    }
    if (!found) {
      if (!argv0 || !realpath(argv0, executable))
        return false;
    }
  }
#endif
  separator = strrchr(executable, '/');
#ifdef _WIN32
  {
    char *backslash = strrchr(executable, '\\');
    if (!separator || (backslash && backslash > separator))
      separator = backslash;
  }
#endif
  if (!separator) {
    strcpy(directory, ".");
#ifdef _WIN32
  } else if (separator == executable + 2 && executable[1] == ':') {
    /* Preserve the root slash in paths such as C:\fluorine.exe. */
    separator[1] = '\0';
    if (strlen(executable) >= sizeof(directory))
      return false;
    strcpy(directory, executable);
#endif
  } else if (separator == executable) {
    separator[1] = '\0';
    strcpy(directory, executable);
  } else {
    *separator = '\0';
    if (strlen(executable) >= sizeof(directory))
      return false;
    strcpy(directory, executable);
  }
  if (strlen(directory) >= sizeof(fluorine_executable_directory))
    return false;
  strcpy(fluorine_executable_directory, directory);

  /* Common layout: <install>/bin/fluorine.exe, with Python under bin/python.
   * Also support an executable placed directly in <install>. */
  {
    char *last = strrchr(directory, '/');
#ifdef _WIN32
    char *backslash = strrchr(directory, '\\');
    if (!last || (backslash && backslash > last))
      last = backslash;
#endif
    const char *basename = last ? last + 1 : directory;
#ifdef _WIN32
    bool is_bin = _stricmp(basename, "bin") == 0;
#else
    bool is_bin = strcmp(basename, "bin") == 0;
#endif
    if (is_bin && last) {
#ifdef _WIN32
      if (last == directory + 2 && directory[1] == ':') {
        /* C:\bin -> C:\, not drive-relative C:. */
        last[1] = '\0';
        strcpy(fluorine_install_directory, directory);
      } else
#endif
          if (last == directory) {
        strcpy(fluorine_install_directory, "/");
      } else {
        *last = '\0';
        if (strlen(directory) >= sizeof(fluorine_install_directory))
          return false;
        strcpy(fluorine_install_directory, directory);
      }
    } else {
      strcpy(fluorine_install_directory, directory);
    }
  }
  return true;
}

static bool resolve_install_path(char *out, size_t capacity,
                                 const char *relative) {
  return path_join(out, capacity, fluorine_install_directory, relative);
}
static bool file_is_readable(const char *path) {
  if (!path || !*path)
    return false;
#ifdef _WIN32
  struct _stat info;
  if (_stat(path, &info) != 0 || (info.st_mode & _S_IFMT) != _S_IFREG)
    return false;
#else
  struct stat info;
  if (stat(path, &info) != 0 || !S_ISREG(info.st_mode))
    return false;
#endif
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  return fclose(f) == 0;
}

static bool file_is_runnable(const char *path) {
  if (!file_is_readable(path))
    return false;
#ifdef _WIN32
  return true; /* CreateProcess performs the definitive image validation. */
#else
  return access(path, X_OK) == 0;
#endif
}

/* Inline C/assembly builds use a private, unique temporary directory. Never
 * write generated sources or libraries into the installed application tree:
 * packages are commonly read-only and concurrent processes must not collide. */
static bool create_inline_temp_paths(char *directory, size_t directory_cap,
                                     char *source, size_t source_cap,
                                     char *library, size_t library_cap,
                                     bool is_c_source) {
#ifdef _WIN32
  char temp_base[FLUORINE_PATH_CAP];
  DWORD n = GetTempPathA((DWORD)sizeof(temp_base), temp_base);
  if (n == 0 || n >= sizeof(temp_base))
    return false;
  size_t base_len = strlen(temp_base);
  while (base_len > 0 &&
         (temp_base[base_len - 1] == '\\' || temp_base[base_len - 1] == '/'))
    temp_base[--base_len] = '\0';
  bool made = false;
  for (unsigned int i = 0; i < 1000; ++i) {
    int npath = snprintf(
        directory, directory_cap, "%s\\Fluorine-inline-%lu-%lu-%u", temp_base,
        (unsigned long)GetCurrentProcessId(), (unsigned long)GetTickCount(), i);
    if (npath < 0 || (size_t)npath >= directory_cap)
      return false;
    if (CreateDirectoryA(directory, NULL)) {
      made = true;
      break;
    }
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
  }
  if (!made)
    return false;
  if (!path_join(source, source_cap, directory,
                 is_c_source ? "source.c" : "source.S") ||
      !path_join(library, library_cap, directory, "module.dll")) {
    RemoveDirectoryA(directory);
    return false;
  }
  return true;
#else
  const char *base = getenv("TMPDIR");
  if (!base || !*base)
    base = "/tmp";
  char templ[FLUORINE_PATH_CAP];
  if (!path_join(templ, sizeof(templ), base, "fluorine-inline-XXXXXX"))
    return false;
  char *created = mkdtemp(templ);
  if (!created)
    return false;
  if (strlen(created) >= directory_cap) {
    rmdir(created);
    return false;
  }
  strcpy(directory, created);
  if (!path_join(source, source_cap, directory,
                 is_c_source ? "source.c" : "source.S") ||
      !path_join(library, library_cap, directory, "module.so")) {
    rmdir(directory);
    return false;
  }
  return true;
#endif
}

static void remove_inline_temp_paths(const char *directory, const char *source,
                                     const char *library) {
#ifdef _WIN32
  if (source)
    DeleteFileA(source);
  if (library)
    DeleteFileA(library);
  if (directory)
    RemoveDirectoryA(directory);
#else
  if (source)
    unlink(source);
  if (library)
    unlink(library);
  if (directory)
    rmdir(directory);
#endif
}
static bool resolve_import_path(char *out, size_t capacity,
                                const char *relative) {
  /* Prefer the executable's own directory, then the install root for the
   * common bin/fluorine.exe + stdlib/ layout. Both are independent of cwd. */
  if (!path_join(out, capacity, fluorine_executable_directory, relative))
    return false;
  if (file_is_readable(out))
    return true;
  if (strcmp(fluorine_install_directory, fluorine_executable_directory) != 0) {
    if (!path_join(out, capacity, fluorine_install_directory, relative))
      return false;
    if (file_is_readable(out))
      return true;
  }
  return path_join(out, capacity, fluorine_executable_directory, relative);
}

/* Bundled toolchains live under the installation's bin/ directory. */
static const char *bundled_python_path(void) {
  static char path[FLUORINE_PATH_CAP];
#ifdef _WIN32
  const char *relative = "bin/python/python.exe";
#else
  const char *relative = "bin/python/python3";
#endif
  if (!resolve_install_path(path, sizeof(path), relative))
    return NULL;
  return path;
}
static const char *bundled_gcc_path(void) {
  static char path[FLUORINE_PATH_CAP];
#ifdef _WIN32
  const char *relative = "bin/c/gcc.exe";
#else
  const char *relative = "bin/c/gcc";
#endif
  if (!resolve_install_path(path, sizeof(path), relative))
    return NULL;
  return path;
}
/* Standard-library helpers. The interpreter remains C99; networking uses the
 * platform socket API. The built-in HTTP server is deliberately HTTP/1.0-style
 * and bounded: one request per connection, 64 KiB request cap, no TLS. */
static bool std_string_arg(const char *fn, int argc, Value *a, int idx,
                           int line) {
  if (idx >= argc || a[idx].type != V_STRING) {
    runtime_error(line, "%s expects string argument %d.", fn, idx + 1);
    return false;
  }
  return true;
}
#ifdef _WIN32
static UINT encoding_codepage(const char *name);
#endif
static bool encoding_name_valid(const char *name) {
  if (!name || !*name)
    return false;
  /* iconv accepts the platform's complete installed encoding catalog. */
#ifdef _WIN32
  static const char *known[] = {
      "UTF-8",      "UTF8",   "ASCII",        "US-ASCII",
      "ISO-8859-1", "LATIN1", "WINDOWS-1252", "CP1252",
      "SHIFT_JIS",  "SJIS",   "GBK",          "GB18030",
      "BIG5",       "KOI8-R", "UTF-16",       "UTF-16LE",
      "UTF-16BE",   "UTF-32", "UTF-32LE",     "UTF-32BE",
      "UNICODE",    NULL};
  for (int i = 0; known[i]; i++)
    if (_stricmp(name, known[i]) == 0)
      return true;
  return encoding_codepage(name) != 0;
#else
  iconv_t cd = iconv_open("UTF-8", name);
  if (cd == (iconv_t)-1)
    return false;
  iconv_close(cd);
  return true;
#endif
}
#ifndef _WIN32
static bool convert_encoding(const char *input, size_t input_len,
                             const char *from, const char *to, char **output,
                             size_t *output_len) {
  iconv_t cd = iconv_open(to, from);
  if (cd == (iconv_t)-1)
    return false;
  size_t cap = input_len < 64 ? 256 : input_len * 4 + 64;
  if (cap < input_len) {
    iconv_close(cd);
    return false;
  }
  char *out = (char *)malloc(cap);
  if (!out) {
    iconv_close(cd);
    return false;
  }
  char *inptr = (char *)input, *outptr = out;
  size_t inleft = input_len, outleft = cap;
  while (iconv(cd, &inptr, &inleft, &outptr, &outleft) == (size_t)-1) {
    if (errno != E2BIG) {
      free(out);
      iconv_close(cd);
      return false;
    }
    size_t used = (size_t)(outptr - out);
    if (cap > SIZE_MAX / 2) {
      free(out);
      iconv_close(cd);
      return false;
    }
    cap *= 2;
    char *next = (char *)realloc(out, cap);
    if (!next) {
      free(out);
      iconv_close(cd);
      return false;
    }
    out = next;
    outptr = out + used;
    outleft = cap - used;
  }
  size_t used = (size_t)(outptr - out);
  iconv_close(cd);
  *output = out;
  *output_len = used;
  return true;
}
#else
static UINT encoding_codepage(const char *name) {
  if (_stricmp(name, "UTF-8") == 0 || _stricmp(name, "UTF8") == 0)
    return CP_UTF8;
  if (_stricmp(name, "ASCII") == 0 || _stricmp(name, "US-ASCII") == 0)
    return 20127;
  if (_stricmp(name, "ISO-8859-1") == 0 || _stricmp(name, "LATIN1") == 0)
    return 28591;
  if (_stricmp(name, "WINDOWS-1252") == 0 || _stricmp(name, "CP1252") == 0)
    return 1252;
  if (_stricmp(name, "SHIFT_JIS") == 0 || _stricmp(name, "SJIS") == 0)
    return 932;
  if (_stricmp(name, "GBK") == 0)
    return 936;
  if (_stricmp(name, "BIG5") == 0)
    return 950;
  if (_stricmp(name, "GB18030") == 0)
    return 54936;
  if (_stricmp(name, "KOI8-R") == 0)
    return 20866;
  const char *digits = NULL;
  if ((_toupper((unsigned char)name[0]) == 'C' &&
       _toupper((unsigned char)name[1]) == 'P'))
    digits = name + 2;
  else if (_strnicmp(name, "WINDOWS-", 8) == 0)
    digits = name + 8;
  else if (_strnicmp(name, "IBM", 3) == 0)
    digits = name + 3;
  else if (_strnicmp(name, "DOS-", 4) == 0)
    digits = name + 4;
  if (digits && *digits) {
    char *end = NULL;
    unsigned long cp = strtoul(digits, &end, 10);
    if (end && !*end && cp > 0 && cp <= 65535 && IsValidCodePage((UINT)cp))
      return (UINT)cp;
  }
  if (_strnicmp(name, "ISO-8859-", 9) == 0) {
    char *end = NULL;
    unsigned long part = strtoul(name + 9, &end, 10);
    static const UINT iso[] = {0,     28591, 28592, 28593, 28594, 28595,
                               28596, 28597, 28598, 28599, 28600, 28601,
                               28603, 28604, 28605, 28606, 874};
    if (end && !*end && part < sizeof(iso) / sizeof(iso[0]) && iso[part] &&
        IsValidCodePage(iso[part]))
      return iso[part];
  }
  return 0;
}
static bool enc_is(const char *name, const char *a, const char *b) {
  return _stricmp(name, a) == 0 || (b && _stricmp(name, b) == 0);
}
static bool convert_encoding(const char *input, size_t input_len,
                             const char *from, const char *to, char **output,
                             size_t *output_len) {
  bool from16 = enc_is(from, "UTF-16LE", NULL) ||
                enc_is(from, "UTF-16BE", NULL) ||
                enc_is(from, "UTF-16", "UNICODE");
  bool from32 = enc_is(from, "UTF-32LE", NULL) ||
                enc_is(from, "UTF-32BE", NULL) || enc_is(from, "UTF-32", NULL);
  bool to16 = enc_is(to, "UTF-16LE", NULL) || enc_is(to, "UTF-16BE", NULL) ||
              enc_is(to, "UTF-16", "UNICODE");
  bool to32 = enc_is(to, "UTF-32LE", NULL) || enc_is(to, "UTF-32BE", NULL) ||
              enc_is(to, "UTF-32", NULL);
  UINT cpfrom = encoding_codepage(from);
  int wn = 0;
  wchar_t *wide = NULL;
  if (from16) {
    if (input_len % 2)
      return false;
    wn = (int)(input_len / 2);
    wide = (wchar_t *)malloc(((size_t)wn + 1) * sizeof(wchar_t));
    if (!wide)
      return false;
    bool be = enc_is(from, "UTF-16BE", NULL);
    for (int i = 0; i < wn; i++) {
      unsigned char x = (unsigned char)input[i * 2],
                    y = (unsigned char)input[i * 2 + 1];
      wide[i] = (wchar_t)(be ? ((unsigned)x << 8) | y : ((unsigned)y << 8) | x);
    }
  } else if (from32) {
    if (input_len % 4)
      return false;
    size_t units = 0;
    bool be = enc_is(from, "UTF-32BE", NULL);
    for (size_t i = 0; i < input_len; i += 4) {
      const unsigned char *q = (const unsigned char *)input + i;
      uint32_t cp = be ? ((uint32_t)q[0] << 24) | ((uint32_t)q[1] << 16) |
                             ((uint32_t)q[2] << 8) | q[3]
                       : ((uint32_t)q[3] << 24) | ((uint32_t)q[2] << 16) |
                             ((uint32_t)q[1] << 8) | q[0];
      if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        return false;
      units += cp > 0xFFFF ? 2 : 1;
    }
    if (units > (size_t)INT_MAX)
      return false;
    wn = (int)units;
    wide = (wchar_t *)malloc((units + 1) * sizeof(wchar_t));
    if (!wide)
      return false;
    size_t j = 0;
    for (size_t i = 0; i < input_len; i += 4) {
      const unsigned char *q = (const unsigned char *)input + i;
      uint32_t cp = be ? ((uint32_t)q[0] << 24) | ((uint32_t)q[1] << 16) |
                             ((uint32_t)q[2] << 8) | q[3]
                       : ((uint32_t)q[3] << 24) | ((uint32_t)q[2] << 16) |
                             ((uint32_t)q[1] << 8) | q[0];
      if (cp <= 0xFFFF)
        wide[j++] = (wchar_t)cp;
      else {
        cp -= 0x10000;
        wide[j++] = (wchar_t)(0xD800 + (cp >> 10));
        wide[j++] = (wchar_t)(0xDC00 + (cp & 0x3FF));
      }
    }
  } else {
    if (!cpfrom || input_len > (size_t)INT_MAX) {
      return false;
    }
    DWORD flags = cpfrom == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
    wn = MultiByteToWideChar(cpfrom, flags, input, (int)input_len, NULL, 0);
    if (wn <= 0 && input_len)
      return false;
    wide = (wchar_t *)malloc(((size_t)wn + 1) * sizeof(wchar_t));
    if (!wide)
      return false;
    if (wn && MultiByteToWideChar(cpfrom, flags, input, (int)input_len, wide,
                                  wn) != wn) {
      free(wide);
      return false;
    }
  }
  size_t prefix = 0, bytes = 0;
  char *out = NULL;
  if (to16) {
    bool be = enc_is(to, "UTF-16BE", NULL);
    prefix = enc_is(to, "UTF-16", "UNICODE") ? 2 : 0;
    if ((size_t)wn > (SIZE_MAX - prefix) / 2) {
      free(wide);
      return false;
    }
    bytes = (size_t)wn * 2;
    out = (char *)malloc(prefix + bytes + 1);
    if (!out) {
      free(wide);
      return false;
    }
    if (prefix) {
      out[0] = (char)0xFF;
      out[1] = (char)0xFE;
    }
    for (int i = 0; i < wn; i++) {
      uint16_t u = (uint16_t)wide[i];
      out[prefix + (size_t)i * 2] = (char)(be ? (u >> 8) : (u & 255));
      out[prefix + (size_t)i * 2 + 1] = (char)(be ? (u & 255) : (u >> 8));
    }
    bytes += prefix;
  } else if (to32) {
    bool be = enc_is(to, "UTF-32BE", NULL);
    prefix = enc_is(to, "UTF-32", NULL) ? 4 : 0;
    if ((size_t)wn > (SIZE_MAX - prefix) / 4) {
      free(wide);
      return false;
    }
    size_t cap = (size_t)wn * 4 + prefix;
    out = (char *)malloc(cap + 1);
    if (!out) {
      free(wide);
      return false;
    }
    if (prefix) {
      out[0] = (char)0xFF;
      out[1] = (char)0xFE;
      out[2] = 0;
      out[3] = 0;
    }
    size_t j = prefix;
    for (int i = 0; i < wn; i++) {
      uint32_t cp = (uint16_t)wide[i];
      if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < wn) {
        uint32_t lo = (uint16_t)wide[i + 1];
        if (lo >= 0xDC00 && lo <= 0xDFFF) {
          cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
          i++;
        }
      }
      for (int k = 0; k < 4; k++)
        out[j++] = (char)(be ? (cp >> (24 - 8 * k)) : (cp >> (8 * k)));
    }
    bytes = j;
  } else {
    UINT cpto = encoding_codepage(to);
    if (!cpto) {
      free(wide);
      return false;
    }
    DWORD flags = cpto == CP_UTF8 ? 0 : WC_NO_BEST_FIT_CHARS;
    int need = WideCharToMultiByte(cpto, flags, wide, wn, NULL, 0, NULL, NULL);
    if (need < 0) {
      free(wide);
      return false;
    }
    out = (char *)malloc((size_t)need + 1);
    if (!out) {
      free(wide);
      return false;
    }
    if (need && WideCharToMultiByte(cpto, flags, wide, wn, out, need, NULL,
                                    NULL) != need) {
      free(wide);
      free(out);
      return false;
    }
    bytes = (size_t)need;
  }
  free(wide);
  out[bytes] = '\0';
  *output = out;
  *output_len = bytes;
  return true;
}
#endif
static const char *encoding_for_iconv(const char *name) {
  if (_stricmp(name, "Unicode") == 0)
    return "UTF-16";
  if (_stricmp(name, "UTF8") == 0)
    return "UTF-8";
  if (_stricmp(name, "ASCII") == 0)
    return "US-ASCII";
  if (_stricmp(name, "Latin1") == 0)
    return "ISO-8859-1";
  return name;
}
static bool file_read_encoded(const char *path, const char *encoding,
                              char **text, size_t *text_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return false;
  }
  long z = ftell(f);
  if (z < 0 || z > 16 * 1024 * 1024 || fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }
  char *raw = (char *)malloc((size_t)z + 4);
  if (!raw) {
    fclose(f);
    return false;
  }
  size_t got = fread(raw, 1, (size_t)z, f);
  bool io_ok = got == (size_t)z && !ferror(f);
  fclose(f);
  if (!io_ok) {
    free(raw);
    return false;
  }
  size_t off = 0;
  const char *from = encoding;
  if ((got >= 3 && (unsigned char)raw[0] == 0xEF &&
       (unsigned char)raw[1] == 0xBB && (unsigned char)raw[2] == 0xBF) &&
      (_stricmp(encoding, "UTF-8") == 0 || _stricmp(encoding, "UTF8") == 0 ||
       _stricmp(encoding, "Unicode") == 0))
    off = 3;
  else if (got >= 4 && (unsigned char)raw[0] == 0xFF &&
           (unsigned char)raw[1] == 0xFE && raw[2] == 0 && raw[3] == 0 &&
           (_stricmp(encoding, "Unicode") == 0 ||
            _stricmp(encoding, "UTF-32") == 0 ||
            _stricmp(encoding, "UTF-32LE") == 0)) {
    from = "UTF-32LE";
    off = 4;
  } else if (got >= 4 && raw[0] == 0 && raw[1] == 0 &&
             (unsigned char)raw[2] == 0xFE && (unsigned char)raw[3] == 0xFF &&
             (_stricmp(encoding, "Unicode") == 0 ||
              _stricmp(encoding, "UTF-32") == 0 ||
              _stricmp(encoding, "UTF-32BE") == 0)) {
    from = "UTF-32BE";
    off = 4;
  } else if (got >= 2 && (unsigned char)raw[0] == 0xFF &&
             (unsigned char)raw[1] == 0xFE &&
             (_stricmp(encoding, "Unicode") == 0 ||
              _stricmp(encoding, "UTF-16") == 0 ||
              _stricmp(encoding, "UTF-16LE") == 0)) {
    from = "UTF-16LE";
    off = 2;
  } else if (got >= 2 && (unsigned char)raw[0] == 0xFE &&
             (unsigned char)raw[1] == 0xFF &&
             (_stricmp(encoding, "Unicode") == 0 ||
              _stricmp(encoding, "UTF-16") == 0 ||
              _stricmp(encoding, "UTF-16BE") == 0)) {
    from = "UTF-16BE";
    off = 2;
  }
  char *converted = NULL;
  size_t converted_len = 0;
  if (!convert_encoding(raw + off, got - off, encoding_for_iconv(from), "UTF-8",
                        &converted, &converted_len)) {
    free(raw);
    return false;
  }
  free(raw);
  if (memchr(converted, 0, converted_len)) {
    free(converted);
    return false;
  }
  char *nul = (char *)realloc(converted, converted_len + 1);
  if (!nul) {
    free(converted);
    return false;
  }
  nul[converted_len] = '\0';
  *text = nul;
  *text_len = converted_len;
  return true;
}
static bool file_write_encoded(const char *path, const char *utf8,
                               const char *encoding, bool append,
                               size_t *bytes_written) {
  char *encoded = NULL;
  size_t encoded_len = 0;
  if (!convert_encoding(utf8, strlen(utf8), "UTF-8",
                        encoding_for_iconv(encoding), &encoded, &encoded_len))
    return false;
  FILE *f = fopen(path, append ? "ab" : "wb");
  if (!f) {
    free(encoded);
    return false;
  }
  bool ok = fwrite(encoded, 1, encoded_len, f) == encoded_len;
  if (fclose(f) != 0)
    ok = false;
  free(encoded);
  if (ok && bytes_written)
    *bytes_written = encoded_len;
  return ok;
}
static Value std_type_of(Value v) {
  switch (v.type) {
  case V_NULL:
    return v_string("null");
  case V_NUMBER:
  case V_BIGINT:
  case V_RATIONAL:
    return v_string("number");
  case V_STRING:
    return v_string("string");
  case V_BOOL:
    return v_string("boolean");
  case V_FUNCTION:
    return v_string("function");
  case V_NATIVE:
    return v_string("native-function");
  case V_ARRAY:
    return v_string("array");
  case V_MAP:
    return v_string("map");
  case V_CLASS:
    return v_string("class");
  case V_STRUCT_DEF:
    return v_string("struct");
  case V_POINTER:
    return v_string("safe-pointer");
  case V_THREAD:
    return v_string("thread");
  }
  return v_string("unknown");
}
static char *std_trim_copy(const char *s) {
  const char *a = s;
  while (*a && isspace((unsigned char)*a))
    a++;
  const char *b = s + strlen(s);
  while (b > a && isspace((unsigned char)b[-1]))
    b--;
  size_t n = (size_t)(b - a);
  char *o = (char *)malloc(n + 1);
  if (!o)
    exit(2);
  memcpy(o, a, n);
  o[n] = '\0';
  return o;
}
#ifdef _WIN32
typedef SOCKET fluorine_socket;
#define FLUORINE_INVALID_SOCKET INVALID_SOCKET
#define fluorine_close_socket closesocket
static int fluorine_socket_start(void) {
  WSADATA w;
  return WSAStartup(MAKEWORD(2, 2), &w);
}
#else
typedef int fluorine_socket;
#define FLUORINE_INVALID_SOCKET (-1)
#define fluorine_close_socket close
static int fluorine_socket_start(void) { return 0; }
#endif
static int socket_send_all(fluorine_socket s, const char *buf, size_t n) {
  size_t off = 0;
  while (off < n) {
#ifdef _WIN32
    int sent =
        send(s, buf, (int)((n - off) > INT_MAX ? INT_MAX : (n - off)), 0);
#else
    ssize_t sent = send(s, buf, n - off, 0);
#endif
    if (sent <= 0)
      return 0;
    off += (size_t)sent;
  }
  return 1;
}
static Value native_http_serve(int argc, Value *a, int line) {
  long port = 0;
  if (argc < 2 || argc > 3 || !numeric_to_long(a[0], &port) || port < 1 ||
      port > 65535 || (a[1].type != V_FUNCTION && a[1].type != V_NATIVE)) {
    runtime_error(line, "http_serve(port, handler, [max_requests]) expects a "
                        "valid port and callable handler.");
    return v_null();
  }
  long max_requests = 0;
  if (argc == 3) {
    if (!numeric_to_long(a[2], &max_requests) || max_requests < 1 ||
        max_requests > 1000000) {
      runtime_error(
          line,
          "http_serve max_requests must be an integer from 1 to 1000000.");
      return v_null();
    }
  }
  if (fluorine_socket_start() != 0) {
    runtime_error(line, "Could not initialize sockets.");
    return v_null();
  }
  fluorine_socket server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server == FLUORINE_INVALID_SOCKET) {
    runtime_error(line, "Could not create HTTP server socket.");
    return v_null();
  }
  int yes = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons((unsigned short)port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
      listen(server, 16) != 0) {
    fluorine_close_socket(server);
    runtime_error(
        line, "Could not bind/listen on 127.0.0.1:%ld (port may be in use).",
        port);
    return v_null();
  }
  fprintf(stderr, "Fluorine HTTP server listening at http://127.0.0.1:%ld/\n",
          port);
  fflush(stderr);
  long served = 0;
  while (!had_error && (!max_requests || served < max_requests)) {
    fluorine_socket client = accept(server, NULL, NULL);
    if (client == FLUORINE_INVALID_SOCKET) {
      if (had_error)
        break;
      continue;
    }
    char req[65537];
    size_t used = 0;
    int header_end = 0;
    req[0] = '\0';
    while (used < 65536) {
#ifdef _WIN32
      int got = recv(client, req + used, (int)(65536 - used), 0);
#else
      ssize_t got = recv(client, req + used, 65536 - used, 0);
#endif
      if (got <= 0)
        break;
      used += (size_t)got;
      req[used] = '\0';
      char *end = strstr(req, "\r\n\r\n");
      if (end) {
        header_end = (int)(end - req) + 4;
        break;
      }
    }
    if (used == 0 || !header_end) {
      static const char bad[] = "HTTP/1.1 400 Bad Request\r\nConnection: "
                                "close\r\nContent-Length: 0\r\n\r\n";
      socket_send_all(client, bad, sizeof(bad) - 1);
      fluorine_close_socket(client);
      continue;
    }
    char method[32] = "GET", path[2048] = "/";
    if (sscanf(req, "%31s %2047s", method, path) != 2) {
      strcpy(method, "GET");
      strcpy(path, "/");
    }
    size_t body_len = 0;
    char *cl = strstr(req, "Content-Length:");
    if (cl && cl < req + header_end) {
      cl += 15;
      while (*cl == ' ' || *cl == '\t')
        cl++;
      body_len = (size_t)strtoul(cl, NULL, 10);
      if (body_len > 32768)
        body_len = 32768;
    }
    while (used < (size_t)header_end + body_len && used < 65536) {
#ifdef _WIN32
      int got = recv(client, req + used, (int)(65536 - used), 0);
#else
      ssize_t got = recv(client, req + used, 65536 - used, 0);
#endif
      if (got <= 0)
        break;
      used += (size_t)got;
      req[used] = '\0';
    }
    char *body = req + header_end;
    size_t have = used > (size_t)header_end ? used - (size_t)header_end : 0;
    if (have > body_len)
      have = body_len;
    body[have] = '\0';
    Value headers = v_map();
    char *linep = strstr(req, "\r\n");
    if (linep) {
      linep += 2;
      while (linep < req + header_end - 2 && *linep) {
        char *e = strstr(linep, "\r\n");
        if (!e || e == linep)
          break;
        char *colon = memchr(linep, ':', (size_t)(e - linep));
        if (colon) {
          char save = *colon;
          *colon = '\0';
          char *v = colon + 1;
          while (*v == ' ' || *v == '\t')
            v++;
          char endsave = *e;
          *e = '\0';
          Value k = v_string(linep), vv = v_string(v);
          map_set(headers.as.map, k, vv);
          free_value(k);
          free_value(vv);
          *e = endsave;
          *colon = save;
        }
        linep = e + 2;
      }
    }
    Value argv[4];
    argv[0] = v_string(method);
    argv[1] = v_string(path);
    argv[2] = headers;
    argv[3] = v_string(body);
    Value response = call_value(a[1], 4, argv, line, v_null(), false);
    char *response_body = NULL;
    int status = 200;
    if (response.type == V_MAP) {
      Value bk = v_string("body"), sk = v_string("status");
      int bi = map_find(response.as.map, bk),
          si = map_find(response.as.map, sk);
      if (bi >= 0)
        response_body = value_to_string(response.as.map->items[bi].value);
      if (si >= 0) {
        long code = 0;
        if (numeric_to_long_trunc(response.as.map->items[si].value, &code) &&
            code >= INT_MIN && code <= INT_MAX)
          status = (int)code;
      }
      free_value(bk);
      free_value(sk);
    }
    if (!response_body)
      response_body = value_to_string(response);
    const char *reason = status == 200   ? "OK"
                         : status == 201 ? "Created"
                         : status == 204 ? "No Content"
                         : status == 400 ? "Bad Request"
                         : status == 401 ? "Unauthorized"
                         : status == 403 ? "Forbidden"
                         : status == 404 ? "Not Found"
                         : status == 500 ? "Internal Server Error"
                                         : "Response";
    char head[256];
    int hn = snprintf(
        head, sizeof(head),
        "HTTP/1.1 %d %s\r\nContent-Type: text/plain; "
        "charset=utf-8\r\nConnection: close\r\nContent-Length: %lu\r\n\r\n",
        status, reason, (unsigned long)strlen(response_body));
    if (hn > 0)
      socket_send_all(client, head, (size_t)hn);
    socket_send_all(client, response_body, strlen(response_body));
    free(response_body);
    free_value(response);
    for (int i = 0; i < 4; i++)
      free_value(argv[i]);
    fluorine_close_socket(client);
    served++;
  }
  fluorine_close_socket(server);
#ifdef _WIN32
  WSACleanup();
#endif
  return v_num((double)served);
}

/* Native HTTP client. The legacy socket implementation remains available for
 * builds without libcurl. Define FLUORINE_WITH_CURL and link libcurl to enable
 * verified HTTPS/TLS, redirects, and configurable timeouts/retries. */
#define FLUORINE_HTTP_MAX_RESPONSE (8u * 1024u * 1024u)
static bool http_header_safe(const char *s) {
  return s && !strchr(s, '\r') && !strchr(s, '\n');
}
#ifdef FLUORINE_WITH_CURL
typedef struct {
  char *data;
  size_t len, cap;
  bool overflow;
  Value headers;
} FluorineCurlResponse;
static size_t fluorine_curl_body(char *ptr, size_t size, size_t nmemb,
                                 void *userdata) {
  FluorineCurlResponse *r = (FluorineCurlResponse *)userdata;
  if (size && nmemb > SIZE_MAX / size)
    return 0;
  size_t n = size * nmemb;
  if (n > FLUORINE_HTTP_MAX_RESPONSE - r->len) {
    r->overflow = true;
    return 0;
  }
  if (r->len + n + 1 > r->cap) {
    size_t cap = r->cap ? r->cap : 8192;
    while (cap < r->len + n + 1) {
      if (cap > FLUORINE_HTTP_MAX_RESPONSE / 2) {
        cap = FLUORINE_HTTP_MAX_RESPONSE + 1;
        break;
      }
      cap *= 2;
    }
    char *q = (char *)realloc(r->data, cap);
    if (!q)
      return 0;
    r->data = q;
    r->cap = cap;
  }
  memcpy(r->data + r->len, ptr, n);
  r->len += n;
  r->data[r->len] = '\0';
  return n;
}
static size_t fluorine_curl_header(char *ptr, size_t size, size_t nmemb,
                                   void *userdata) {
  FluorineCurlResponse *r = (FluorineCurlResponse *)userdata;
  if (size && nmemb > SIZE_MAX / size)
    return 0;
  size_t n = size * nmemb;
  if (n >= 5 && _strnicmp(ptr, "HTTP/", 5) == 0) {
    free_value(r->headers);
    r->headers = v_map();
    r->len = 0;
    if (r->data)
      r->data[0] = '\0';
    r->overflow = false;
    return n;
  }
  if (n >= 2 && ptr[0] == '\r' && ptr[1] == '\n')
    return n;
  char *colon = memchr(ptr, ':', n);
  if (!colon)
    return n;
  size_t kn = (size_t)(colon - ptr);
  if (!kn)
    return n;
  size_t vstart = kn + 1;
  while (vstart < n && (ptr[vstart] == ' ' || ptr[vstart] == '\t'))
    vstart++;
  size_t vend = n;
  while (vend > vstart && (ptr[vend - 1] == '\r' || ptr[vend - 1] == '\n' ||
                           ptr[vend - 1] == ' ' || ptr[vend - 1] == '\t'))
    vend--;
  char *key = copy_n(ptr, kn), *val = copy_n(ptr + vstart, vend - vstart);
  Value k = v_string(key), v = v_string(val);
  map_set(r->headers.as.map, k, v);
  free_value(k);
  free_value(v);
  free(key);
  free(val);
  return n;
}
static bool fluorine_http_option(Value opts, const char *name, double *number,
                                 bool *boolean, bool want_bool) {
  if (opts.type != V_MAP)
    return false;
  Value k = v_string(name);
  int i = map_find(opts.as.map, k);
  free_value(k);
  if (i < 0)
    return false;
  Value v = opts.as.map->items[i].value;
  if (want_bool) {
    if (v.type == V_BOOL) {
      *boolean = v.as.boolean;
      return true;
    }
    if (v.type == V_NUMBER) {
      *boolean = v.as.number != 0;
      return true;
    }
    return false;
  }
  if (is_numeric(v)) {
    *number = numeric_double(v);
    return true;
  }
  if (v.type == V_BOOL) {
    *number = v.as.boolean ? 1 : 0;
    return true;
  }
  return false;
}
static Value native_http_request_curl(int argc, Value *a, int line) {
  if (argc < 2 || argc > 5 || a[0].type != V_STRING || a[1].type != V_STRING ||
      (argc >= 3 && a[2].type != V_MAP) ||
      (argc >= 4 && a[3].type != V_STRING) ||
      (argc >= 5 && a[4].type != V_MAP)) {
    runtime_error(line, "http_request(method, url, [headers_map], "
                        "[body_string], [options_map]) expects a method, URL, "
                        "optional headers, body, and options map.");
    return v_null();
  }
  const char *method = a[0].as.string, *url = a[1].as.string,
             *body = argc >= 4 ? a[3].as.string : "";
  if (!*method || strlen(method) > 32 || !http_header_safe(method)) {
    runtime_error(line, "HTTP method must be a short valid token.");
    return v_null();
  }
  for (const unsigned char *m = (const unsigned char *)method; *m; m++)
    if (!isalnum(*m) && *m != '-' && *m != '_') {
      runtime_error(line, "HTTP method may contain only letters, digits, "
                          "hyphens, and underscores.");
      return v_null();
    }
  if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
    runtime_error(line, "HTTP URL must start with http:// or https://.");
    return v_null();
  }
  size_t body_len = strlen(body);
  if (body_len > FLUORINE_HTTP_MAX_RESPONSE) {
    runtime_error(line, "HTTP request body exceeds the 8 MiB safety limit.");
    return v_null();
  }
  Value opts = argc >= 5 ? a[4] : v_null();
  double timeout_n = 10000, connect_n = 5000, retries_n = 0, redirects_n = 10;
  bool follow = true, verify = true;
  fluorine_http_option(opts, "timeout_ms", &timeout_n, NULL, false);
  fluorine_http_option(opts, "connect_timeout_ms", &connect_n, NULL, false);
  fluorine_http_option(opts, "retries", &retries_n, NULL, false);
  fluorine_http_option(opts, "max_redirects", &redirects_n, NULL, false);
  fluorine_http_option(opts, "follow_redirects", NULL, &follow, true);
  fluorine_http_option(opts, "verify_tls", NULL, &verify, true);
  if (timeout_n < 1 || timeout_n > 3600000 || connect_n < 1 ||
      connect_n > 3600000 || retries_n < 0 || retries_n > 10 ||
      floor(retries_n) != retries_n || redirects_n < 0 || redirects_n > 50 ||
      floor(redirects_n) != redirects_n) {
    runtime_error(line,
                  "HTTP options require timeout_ms/connect_timeout_ms in "
                  "1..3600000, retries in 0..10, and max_redirects in 0..50.");
    return v_null();
  }
  if (!http_header_safe(url)) {
    runtime_error(line, "HTTP URL contains invalid line breaks.");
    return v_null();
  }
  static bool curl_initialized = false;
  if (!curl_initialized) {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
      runtime_error(line, "Could not initialize the HTTP/TLS library.");
      return v_null();
    }
    curl_initialized = true;
  }
  struct curl_slist *headers_list = NULL;
  if (argc >= 3) {
    for (int i = 0; i < a[2].as.map->count; i++) {
      Value k = a[2].as.map->items[i].key, v = a[2].as.map->items[i].value;
      if (k.type != V_STRING || v.type != V_STRING ||
          !http_header_safe(k.as.string) || !http_header_safe(v.as.string) ||
          strchr(k.as.string, ':')) {
        curl_slist_free_all(headers_list);
        runtime_error(
            line,
            "HTTP headers must use string names and values without CR/LF.");
        return v_null();
      }
      size_t need = strlen(k.as.string) + strlen(v.as.string) + 3;
      char *h = (char *)malloc(need);
      if (!h)
        exit(2);
      snprintf(h, need, "%s: %s", k.as.string, v.as.string);
      struct curl_slist *next = curl_slist_append(headers_list, h);
      free(h);
      if (!next) {
        curl_slist_free_all(headers_list);
        runtime_error(line, "Could not allocate HTTP headers.");
        return v_null();
      }
      headers_list = next;
    }
  }
  CURL *curl = curl_easy_init();
  if (!curl) {
    curl_slist_free_all(headers_list);
    runtime_error(line, "Could not create an HTTP request.");
    return v_null();
  }
  FluorineCurlResponse r;
  memset(&r, 0, sizeof(r));
  r.headers = v_map();
  r.data = (char *)malloc(1);
  if (!r.data)
    exit(2);
  r.data[0] = '\0';
  r.cap = 1;
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, follow ? 1L : 0L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, (long)redirects_n);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, (long)connect_n);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)timeout_n);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verify ? 1L : 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verify ? 2L : 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fluorine_curl_body);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, fluorine_curl_header);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &r);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Fluorine/0.7");
#if LIBCURL_VERSION_NUM >= 0x075500
  curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
  curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
  curl_easy_setopt(curl, CURLOPT_PROTOCOLS,
                   (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
  curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS,
                   (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
  if (body_len) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)body_len);
  }
  CURLcode code = CURLE_OK;
  long status = 0;
  int attempts = (int)retries_n + 1;
  for (int attempt = 0; attempt < attempts; attempt++) {
    if (attempt) {
      r.len = 0;
      r.data[0] = '\0';
      r.overflow = false;
      free_value(r.headers);
      r.headers = v_map();
    }
    code = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (code == CURLE_OK && !r.overflow &&
        !(status == 429 || status == 500 || status == 502 || status == 503 ||
          status == 504))
      break;
    if (attempt + 1 < attempts &&
        (code != CURLE_OK || status == 429 || status == 500 || status == 502 ||
         status == 503 || status == 504))
      continue;
    break;
  }
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers_list);
  if (code != CURLE_OK || r.overflow) {
    free_value(r.headers);
    free(r.data);
    runtime_error(
        line, "HTTP request failed: %s%s",
        r.overflow ? "response exceeded the 8 MiB limit"
                   : curl_easy_strerror(code),
        r.overflow ? "" : " (check URL, TLS certificate, or timeout settings)");
    return v_null();
  }
  Value result = v_map();
  Value k1 = v_string("status"), v1 = v_num((double)status),
        k2 = v_string("headers"), k3 = v_string("body"), v3 = v_string(r.data),
        k4 = v_string("ok"), v4 = v_bool(status >= 200 && status < 300);
  map_set(result.as.map, k1, v1);
  map_set(result.as.map, k2, r.headers);
  map_set(result.as.map, k3, v3);
  map_set(result.as.map, k4, v4);
  free_value(k1);
  free_value(v1);
  free_value(k2);
  free_value(k3);
  free_value(v3);
  free_value(k4);
  free_value(v4);
  free_value(r.headers);
  free(r.data);
  return result;
}
#endif
static Value native_http_request(int argc, Value *a, int line) {
#ifdef FLUORINE_WITH_CURL
  return native_http_request_curl(argc, a, line);
#else
  if (argc < 2 || argc > 4 || a[0].type != V_STRING || a[1].type != V_STRING ||
      (argc >= 3 && a[2].type != V_MAP) ||
      (argc >= 4 && a[3].type != V_STRING)) {
    runtime_error(
        line, "http_request(method, url, [headers_map], [body_string]) expects "
              "a method, URL, optional header map and optional body string; "
              "compile with FLUORINE_WITH_CURL to enable the options map.");
    return v_null();
  }
  const char *method = a[0].as.string, *url = a[1].as.string;
  if (!*method || strlen(method) > 32 || !http_header_safe(method)) {
    runtime_error(line,
                  "http_request method must be a short valid HTTP token.");
    return v_null();
  }
  for (const unsigned char *m = (const unsigned char *)method; *m; m++)
    if (!isalnum(*m) && *m != '-' && *m != '_') {
      runtime_error(line, "http_request method may contain only letters, "
                          "digits, hyphens, and underscores.");
      return v_null();
    }
  if (strncmp(url, "https://", 8) == 0) {
    runtime_error(line, "HTTPS is not available in this native build; use an "
                        "HTTP URL or a TLS-enabled external client.");
    return v_null();
  }
  if (strncmp(url, "http://", 7) != 0) {
    runtime_error(line, "http_request URL must start with http:// (HTTPS needs "
                        "a TLS-enabled build).");
    return v_null();
  }
  const char *authority = url + 7, *path = strpbrk(authority, "/?#");
  size_t authority_len = path ? (size_t)(path - authority) : strlen(authority);
  if (authority_len == 0 || authority_len > 1023 ||
      memchr(authority, '@', authority_len)) {
    runtime_error(
        line, "http_request URL has an invalid host or embedded credentials.");
    return v_null();
  }
  char hostport[1024];
  memcpy(hostport, authority, authority_len);
  hostport[authority_len] = '\0';
  char host[1024], port_text[8] = "80";
  char *colon = strrchr(hostport, ':');
  if (colon) {
    if (strchr(hostport, ']') == NULL || colon > strchr(hostport, ']')) {
      *colon = '\0';
      if (!colon[1]) {
        runtime_error(line, "http_request URL has an empty port.");
        return v_null();
      }
      snprintf(port_text, sizeof(port_text), "%.7s", colon + 1);
    }
  }
  if (!*hostport || strlen(hostport) >= sizeof(host)) {
    runtime_error(line, "http_request URL host is invalid.");
    return v_null();
  }
  if (hostport[0] == '[') {
    size_t hn = strlen(hostport);
    if (hn < 3 || hostport[hn - 1] != ']') {
      runtime_error(line,
                    "http_request IPv6 host must be enclosed in brackets.");
      return v_null();
    }
    hostport[hn - 1] = '\0';
    memmove(hostport, hostport + 1, hn - 1);
  }
  strcpy(host, hostport);
  char *port_end = NULL;
  long port = strtol(port_text, &port_end, 10);
  if (!port_end || *port_end || port < 1 || port > 65535) {
    runtime_error(line, "http_request URL port must be 1..65535.");
    return v_null();
  }
  char target[4096];
  if (!path || !*path)
    strcpy(target, "/");
  else if (*path == '#')
    strcpy(target, "/");
  else if (*path == '?') {
    if (snprintf(target, sizeof(target), "/%s", path) >= (int)sizeof(target)) {
      runtime_error(line, "http_request URL path is too long.");
      return v_null();
    }
  } else {
    const char *hash = strchr(path, '#');
    size_t n = hash ? (size_t)(hash - path) : strlen(path);
    if (n >= sizeof(target)) {
      runtime_error(line, "http_request URL path is too long.");
      return v_null();
    }
    memcpy(target, path, n);
    target[n] = '\0';
  }
  const char *body = argc == 4 ? a[3].as.string : "";
  size_t body_len = strlen(body);
  if (body_len > FLUORINE_HTTP_MAX_RESPONSE) {
    runtime_error(line, "http_request body exceeds the 8 MiB safety limit.");
    return v_null();
  }
  if (fluorine_socket_start() != 0) {
    runtime_error(line, "Could not initialize network sockets.");
    return v_null();
  }
  char portbuf[8];
  snprintf(portbuf, sizeof(portbuf), "%ld", port);
  struct addrinfo hints, *res = NULL, *it;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  if (getaddrinfo(host, portbuf, &hints, &res) != 0) {
#ifdef _WIN32
    WSACleanup();
#endif
    runtime_error(line, "Could not resolve HTTP host '%s'.", host);
    return v_null();
  }
  fluorine_socket sock = FLUORINE_INVALID_SOCKET;
  for (it = res; it; it = it->ai_next) {
    sock = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
    if (sock == FLUORINE_INVALID_SOCKET)
      continue;
#ifdef _WIN32
    DWORD timeout = 10000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout,
               sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout,
               sizeof(timeout));
#else
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
    if (connect(sock, it->ai_addr, (int)it->ai_addrlen) == 0)
      break;
    fluorine_close_socket(sock);
    sock = FLUORINE_INVALID_SOCKET;
  }
  freeaddrinfo(res);
  if (sock == FLUORINE_INVALID_SOCKET) {
#ifdef _WIN32
    WSACleanup();
#endif
    runtime_error(
        line,
        "Could not connect to %s:%ld (connection timed out or was refused).",
        host, port);
    return v_null();
  }
  size_t req_cap = 4096 + body_len;
  for (int i = 0; argc >= 3 && i < a[2].as.map->count; i++) {
    Value k = a[2].as.map->items[i].key, v = a[2].as.map->items[i].value;
    if (k.type != V_STRING || v.type != V_STRING ||
        !http_header_safe(k.as.string) || !http_header_safe(v.as.string) ||
        strchr(k.as.string, ':')) {
      fluorine_close_socket(sock);
#ifdef _WIN32
      WSACleanup();
#endif
      runtime_error(
          line, "HTTP headers must use string names and values without CR/LF.");
      return v_null();
    }
    req_cap += strlen(k.as.string) + strlen(v.as.string) + 4;
  }
  if (req_cap > FLUORINE_HTTP_MAX_RESPONSE) {
    fluorine_close_socket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    runtime_error(line, "HTTP request is too large.");
    return v_null();
  }
  char *request = (char *)malloc(req_cap + 2048);
  if (!request)
    exit(2);
  size_t used = 0;
  char host_header[1100];
  if (strchr(host, ':'))
    snprintf(host_header, sizeof(host_header), port == 80 ? "[%s]" : "[%s]:%ld",
             host, port);
  else
    snprintf(host_header, sizeof(host_header), port == 80 ? "%s" : "%s:%ld",
             host, port);
  int n = snprintf(request, req_cap + 2048,
                   "%s %s HTTP/1.1\r\nHost: %s\r\nConnection: "
                   "close\r\nUser-Agent: Fluorine/0.6\r\n",
                   method, target, host_header);
  if (n < 0 || (size_t)n >= req_cap + 2048) {
    free(request);
    fluorine_close_socket(sock);
    runtime_error(line, "Could not build HTTP request.");
    return v_null();
  }
  used = (size_t)n;
  bool has_length = false;
  for (int i = 0; argc >= 3 && i < a[2].as.map->count; i++) {
    Value k = a[2].as.map->items[i].key, v = a[2].as.map->items[i].value;
    if (_stricmp(k.as.string, "content-length") == 0)
      has_length = true;
    n = snprintf(request + used, req_cap + 2048 - used, "%s: %s\r\n",
                 k.as.string, v.as.string);
    if (n < 0 || (size_t)n >= req_cap + 2048 - used) {
      free(request);
      fluorine_close_socket(sock);
      runtime_error(line, "HTTP headers are too large.");
      return v_null();
    }
    used += (size_t)n;
  }
  if (body_len && !has_length) {
    n = snprintf(request + used, req_cap + 2048 - used,
                 "Content-Length: %lu\r\n", (unsigned long)body_len);
    if (n < 0 || (size_t)n >= req_cap + 2048 - used) {
      free(request);
      fluorine_close_socket(sock);
      runtime_error(line, "HTTP request headers are too large.");
      return v_null();
    }
    used += (size_t)n;
  }
  if (used + 2 + body_len > req_cap + 2048) {
    free(request);
    fluorine_close_socket(sock);
    runtime_error(line, "HTTP request is too large.");
    return v_null();
  }
  memcpy(request + used, "\r\n", 2);
  used += 2;
  if (body_len) {
    memcpy(request + used, body, body_len);
    used += body_len;
  }
  int sent_ok = socket_send_all(sock, request, used);
  free(request);
  if (!sent_ok) {
    fluorine_close_socket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    runtime_error(line, "Failed while sending HTTP request.");
    return v_null();
  }
  size_t cap = 8192, got_total = 0;
  char *raw = (char *)malloc(cap + 1);
  if (!raw)
    exit(2);
  bool read_error = false;
  for (;;) {
    if (got_total == FLUORINE_HTTP_MAX_RESPONSE) {
      char extra;
      int rr = recv(sock, &extra, 1, 0);
      if (rr > 0)
        read_error = true;
      break;
    }
    if (got_total == cap) {
      size_t next = cap * 2;
      if (next > FLUORINE_HTTP_MAX_RESPONSE)
        next = FLUORINE_HTTP_MAX_RESPONSE;
      char *grown = (char *)realloc(raw, next + 1);
      if (!grown) {
        free(raw);
        fluorine_close_socket(sock);
        runtime_error(line, "Out of memory reading HTTP response.");
        return v_null();
      }
      raw = grown;
      cap = next;
    }
#ifdef _WIN32
    int nr = recv(
        sock, raw + got_total,
        (int)((cap - got_total) > INT_MAX ? INT_MAX : (cap - got_total)), 0);
#else
    ssize_t nr = recv(sock, raw + got_total, cap - got_total, 0);
#endif
    if (nr == 0)
      break;
    if (nr < 0) {
      if (errno == EINTR)
        continue;
#ifdef _WIN32
      if (WSAGetLastError() == WSAEINTR)
        continue;
#endif
      read_error = true;
      break;
    }
    got_total += (size_t)nr;
  }
  fluorine_close_socket(sock);
#ifdef _WIN32
  WSACleanup();
#endif
  raw[got_total] = '\0';
  if (read_error) {
    free(raw);
    runtime_error(
        line, "HTTP response failed, timed out, or exceeded the 8 MiB limit.");
    return v_null();
  }
  char *header_end = strstr(raw, "\r\n\r\n");
  if (!header_end) {
    free(raw);
    runtime_error(line, "Server returned an invalid HTTP response (missing "
                        "header terminator).");
    return v_null();
  }
  int status = 0;
  if (sscanf(raw, "HTTP/%*d.%*d %d", &status) != 1 || status < 100 ||
      status > 999) {
    free(raw);
    runtime_error(line, "Server returned an invalid HTTP status line.");
    return v_null();
  }
  Value headers = v_map();
  char *linep = strstr(raw, "\r\n");
  if (linep) {
    linep += 2;
    while (linep < header_end && *linep) {
      char *e = strstr(linep, "\r\n");
      if (!e || e > header_end || e == linep)
        break;
      char *c = memchr(linep, ':', (size_t)(e - linep));
      if (c) {
        char *key = copy_n(linep, (size_t)(c - linep));
        char *val = c + 1;
        while (val < e && (*val == ' ' || *val == '\t'))
          val++;
        char *value = copy_n(val, (size_t)(e - val));
        Value k = v_string(key), v = v_string(value);
        map_set(headers.as.map, k, v);
        free_value(k);
        free_value(v);
        free(key);
        free(value);
      }
      linep = e + 2;
    }
  }
  char *response_body = header_end + 4;
  size_t response_len = got_total - (size_t)(response_body - raw);
  char *clh = NULL, *te = NULL;
  Value ck = v_string("Content-Length"), tk = v_string("Transfer-Encoding");
  int ci = map_find(headers.as.map, ck), ti = map_find(headers.as.map, tk);
  if (ci >= 0)
    clh = headers.as.map->items[ci].value.as.string;
  if (ti >= 0)
    te = headers.as.map->items[ti].value.as.string;
  free_value(ck);
  free_value(tk);
  if (clh) {
    char *end = NULL;
    unsigned long long want = strtoull(clh, &end, 10);
    if (end == clh || *end || want > response_len) {
      free_value(headers);
      free(raw);
      runtime_error(
          line,
          "HTTP response body is shorter than its Content-Length header.");
      return v_null();
    }
    response_len = (size_t)want;
  } else if (te && strstr(te, "chunked")) {
    size_t src = 0, dst = 0;
    while (src < response_len) {
      char *e = NULL;
      unsigned long chunk = strtoul(response_body + src, &e, 16);
      if (e == response_body + src || !e || (*e != '\r' && *e != ';'))
        break;
      char *crlf = strstr(e, "\r\n");
      if (!crlf)
        break;
      src = (size_t)(crlf + 2 - response_body);
      if (chunk == 0) {
        response_len = dst;
        break;
      }
      if (chunk > response_len - src || src + chunk + 2 > response_len)
        break;
      memmove(response_body + dst, response_body + src, (size_t)chunk);
      dst += (size_t)chunk;
      src += (size_t)chunk;
      if (response_body[src] == '\r' && response_body[src + 1] == '\n')
        src += 2;
      else
        break;
    }
    response_len = dst;
  }
  char *body_copy = (char *)malloc(response_len + 1);
  if (!body_copy)
    exit(2);
  memcpy(body_copy, response_body, response_len);
  body_copy[response_len] = '\0';
  Value result = v_map();
  Value k1 = v_string("status"), v1 = v_num(status), k2 = v_string("headers"),
        k3 = v_string("body"), v3 = v_string(body_copy), k4 = v_string("ok"),
        v4 = v_bool(status >= 200 && status < 300);
  map_set(result.as.map, k1, v1);
  map_set(result.as.map, k2, headers);
  map_set(result.as.map, k3, v3);
  map_set(result.as.map, k4, v4);
  free_value(k1);
  free_value(v1);
  free_value(k2);
  free_value(k3);
  free_value(v3);
  free_value(k4);
  free_value(v4);
  free_value(headers);
  free(body_copy);
  free(raw);
  return result;
#endif
}
static Value native_http_get(int argc, Value *a, int line) {
#ifdef FLUORINE_WITH_CURL
  if (argc < 1 || argc > 3 || a[0].type != V_STRING ||
      (argc >= 2 && a[1].type != V_MAP) || (argc >= 3 && a[2].type != V_MAP)) {
    runtime_error(line, "http_get(url, [headers_map], [options_map]) expects a "
                        "URL and optional headers/options maps.");
    return v_null();
  }
  Value args[5];
  args[0] = v_string("GET");
  args[1] = copy_value(a[0]);
  args[2] = argc >= 2 ? copy_value(a[1]) : v_map();
  args[3] = v_string("");
  int count = 4;
  if (argc >= 3) {
    args[4] = copy_value(a[2]);
    count = 5;
  }
  Value out = native_http_request(count, args, line);
  for (int i = 0; i < count; i++)
    free_value(args[i]);
  return out;
#else
  if (argc < 1 || argc > 2 || a[0].type != V_STRING ||
      (argc == 2 && a[1].type != V_MAP)) {
    runtime_error(
        line, "http_get(url, [headers_map]) expects a URL and optional header "
              "map; compile with FLUORINE_WITH_CURL for TLS/options.");
    return v_null();
  }
  Value args[4];
  args[0] = v_string("GET");
  args[1] = copy_value(a[0]);
  int count = 2;
  if (argc == 2) {
    args[2] = copy_value(a[1]);
    count = 3;
  }
  Value out = native_http_request(count, args, line);
  for (int i = 0; i < count; i++)
    free_value(args[i]);
  return out;
#endif
}

/* Native desktop input. Windows uses Win32; POSIX builds report unsupported
 * desktop operations instead of silently pretending that input was sent. */
#ifdef _WIN32
static int mouse_button_code(const char *s) {
  if (!s)
    return 0;
  if (_stricmp(s, "left") == 0 || _stricmp(s, "lmb") == 0)
    return 1;
  if (_stricmp(s, "right") == 0 || _stricmp(s, "rmb") == 0)
    return 2;
  if (_stricmp(s, "middle") == 0 || _stricmp(s, "mmb") == 0)
    return 3;
  return 0;
}
#endif
static Value native_input(const char *name, int argc, Value *a, int line) {
#ifdef _WIN32
  if (strcmp(name, "mouse_position") == 0) {
    POINT p;
    if (argc != 0) {
      runtime_error(line, "mouse_position expects no arguments.");
      return v_null();
    }
    if (!GetCursorPos(&p)) {
      runtime_error(line, "GetCursorPos failed (error %lu).",
                    (unsigned long)GetLastError());
      return v_null();
    }
    Value v = v_array();
    Array *ar = v.as.array;
    ar->items = (Value *)xrealloc(ar->items, 2 * sizeof(Value));
    ar->capacity = 2;
    ar->count = 2;
    ar->items[0] = v_num(p.x);
    ar->items[1] = v_num(p.y);
    return v;
  }
  if (strcmp(name, "mouse_screen_size") == 0) {
    if (argc != 0) {
      runtime_error(line, "mouse_screen_size expects no arguments.");
      return v_null();
    }
    Value v = v_array();
    Array *ar = v.as.array;
    ar->items = (Value *)xrealloc(ar->items, 2 * sizeof(Value));
    ar->capacity = 2;
    ar->count = 2;
    ar->items[0] = v_num(GetSystemMetrics(SM_CXSCREEN));
    ar->items[1] = v_num(GetSystemMetrics(SM_CYSCREEN));
    return v;
  }
  if (strcmp(name, "mouse_move") == 0 ||
      strcmp(name, "mouse_set_position") == 0 ||
      strcmp(name, "mouse_left_click_at") == 0 ||
      strcmp(name, "mouse_right_click_at") == 0) {
    long x = 0, y = 0;
    if (argc != 2 || !numeric_to_long(a[0], &x) || !numeric_to_long(a[1], &y)) {
      runtime_error(line, "%s expects integer x, y.", name);
      return v_null();
    }
    if (!SetCursorPos((int)x, (int)y)) {
      runtime_error(line, "%s failed (error %lu).", name,
                    (unsigned long)GetLastError());
      return v_null();
    }
    if (strcmp(name, "mouse_left_click_at") == 0 ||
        strcmp(name, "mouse_right_click_at") == 0) {
      INPUT in[2];
      memset(in, 0, sizeof(in));
      in[0].type = in[1].type = INPUT_MOUSE;
      DWORD down = strcmp(name, "mouse_left_click_at") == 0
                       ? MOUSEEVENTF_LEFTDOWN
                       : MOUSEEVENTF_RIGHTDOWN;
      DWORD up = strcmp(name, "mouse_left_click_at") == 0 ? MOUSEEVENTF_LEFTUP
                                                          : MOUSEEVENTF_RIGHTUP;
      in[0].mi.dwFlags = down;
      in[1].mi.dwFlags = up;
      if (SendInput(2, in, sizeof(INPUT)) != 2) {
        runtime_error(line, "SendInput failed (error %lu).",
                      (unsigned long)GetLastError());
        return v_null();
      }
    }
    return v_null();
  }
  if (strcmp(name, "mouse_smooth_move") == 0) {
    long x = 0, y = 0, duration = 0;
    if (argc != 3 || !numeric_to_long(a[0], &x) || !numeric_to_long(a[1], &y) ||
        !numeric_to_long(a[2], &duration) || duration < 0 || duration > 60000) {
      runtime_error(line,
                    "mouse_smooth_move expects x, y, duration_ms (0..60000).");
      return v_null();
    }
    POINT p;
    if (!GetCursorPos(&p))
      return v_bool(false);
    int sx = p.x, sy = p.y;
    int steps = (int)(duration / 8);
    if (steps < 1)
      steps = 1;
    if (steps > 7500)
      steps = 7500;
    for (int i = 1; i <= steps; i++) {
      double t = (double)i / (double)steps;
      int nx = (int)(sx + ((double)x - sx) * t),
          ny = (int)(sy + ((double)y - sy) * t);
      if (!SetCursorPos(nx, ny)) {
        runtime_error(line, "SetCursorPos failed.");
        return v_bool(false);
      }
      if (duration > 0)
        Sleep((DWORD)(duration / steps));
    }
    return v_null();
  }
  if (strcmp(name, "mouse_left_click") == 0 ||
      strcmp(name, "mouse_right_click") == 0 ||
      strcmp(name, "mouse_middle_click") == 0 ||
      strcmp(name, "mouse_double_click") == 0 ||
      strcmp(name, "mouse_press") == 0 || strcmp(name, "mouse_release") == 0 ||
      strcmp(name, "mouse_scroll") == 0) {
    INPUT in[4];
    memset(in, 0, sizeof(in));
    int count = 1;
    DWORD down = 0, up = 0;
    long amount = 0;
    if (strcmp(name, "mouse_scroll") == 0) {
      if (argc != 1 || !numeric_to_long(a[0], &amount)) {
        runtime_error(line, "mouse_scroll expects integer wheel units.");
        return v_null();
      }
      in[0].type = INPUT_MOUSE;
      in[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
      in[0].mi.mouseData = (DWORD)(amount * WHEEL_DELTA);
    } else if (strcmp(name, "mouse_press") == 0 ||
               strcmp(name, "mouse_release") == 0) {
      if (argc != 1 || a[0].type != V_STRING) {
        runtime_error(line, "%s expects button string: left, right, or middle.",
                      name);
        return v_null();
      }
      int b = mouse_button_code(a[0].as.string);
      if (!b) {
        runtime_error(line, "Fluorine mouse button '%s'.", a[0].as.string);
        return v_null();
      }
      down = b == 1   ? MOUSEEVENTF_LEFTDOWN
             : b == 2 ? MOUSEEVENTF_RIGHTDOWN
                      : MOUSEEVENTF_MIDDLEDOWN;
      up = b == 1   ? MOUSEEVENTF_LEFTUP
           : b == 2 ? MOUSEEVENTF_RIGHTUP
                    : MOUSEEVENTF_MIDDLEUP;
      in[0].type = INPUT_MOUSE;
      in[0].mi.dwFlags = strcmp(name, "mouse_press") == 0 ? down : up;
    } else {
      if (argc != 0) {
        runtime_error(line, "%s expects no arguments.", name);
        return v_null();
      }
      down = strcmp(name, "mouse_left_click") == 0 ||
                     strcmp(name, "mouse_double_click") == 0
                 ? MOUSEEVENTF_LEFTDOWN
             : strcmp(name, "mouse_right_click") == 0 ? MOUSEEVENTF_RIGHTDOWN
                                                      : MOUSEEVENTF_MIDDLEDOWN;
      up = strcmp(name, "mouse_left_click") == 0 ||
                   strcmp(name, "mouse_double_click") == 0
               ? MOUSEEVENTF_LEFTUP
           : strcmp(name, "mouse_right_click") == 0 ? MOUSEEVENTF_RIGHTUP
                                                    : MOUSEEVENTF_MIDDLEUP;
      count = strcmp(name, "mouse_double_click") == 0 ? 4 : 2;
      for (int i = 0; i < count; i++) {
        in[i].type = INPUT_MOUSE;
        in[i].mi.dwFlags = (i % 2 == 0) ? down : up;
      }
    }
    if (SendInput((UINT)count, in, sizeof(INPUT)) != (UINT)count) {
      runtime_error(line, "SendInput failed (error %lu).",
                    (unsigned long)GetLastError());
      return v_bool(false);
    }
    return v_null();
  }
  if (strcmp(name, "mouse_is_button_down") == 0) {
    if (argc != 1 || a[0].type != V_STRING) {
      runtime_error(line, "mouse_is_button_down expects a button string.");
      return v_null();
    }
    int b = mouse_button_code(a[0].as.string);
    if (!b) {
      runtime_error(line, "Fluorine mouse button '%s'.", a[0].as.string);
      return v_null();
    }
    int vk = b == 1 ? VK_LBUTTON : b == 2 ? VK_RBUTTON : VK_MBUTTON;
    return v_bool((GetAsyncKeyState(vk) & 0x8000) != 0);
  }
  if (strcmp(name, "mouse_show_cursor") == 0) {
    if (argc != 1 || a[0].type != V_BOOL) {
      runtime_error(line, "mouse_show_cursor expects a boolean.");
      return v_null();
    }
    if (a[0].as.boolean) {
      while (ShowCursor(TRUE) < 0) {
      }
    } else {
      while (ShowCursor(FALSE) >= 0) {
      }
    }
    return v_bool(true);
  }
  if (strcmp(name, "mouse_set_system_cursor") == 0) {
    if (argc != 1 || a[0].type != V_STRING) {
      runtime_error(line,
                    "mouse_set_system_cursor expects a cursor file path.");
      return v_null();
    }
    LPCSTR path = a[0].as.string;
    int ids[] = {OCR_NORMAL,   OCR_IBEAM,  OCR_WAIT,        OCR_CROSS,
                 OCR_UP,       OCR_SIZE,   OCR_ICON,        OCR_SIZENWSE,
                 OCR_SIZENESW, OCR_SIZEWE, OCR_SIZENS,      OCR_SIZEALL,
                 OCR_NO,       OCR_HAND,   OCR_APPSTARTING, OCR_HELP};
    int ok = 0;
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
      HCURSOR c = (HCURSOR)LoadImageA(NULL, path, IMAGE_CURSOR, 0, 0,
                                      LR_LOADFROMFILE | LR_DEFAULTSIZE);
      if (c && SetSystemCursor(c, (DWORD)ids[i]))
        ok++;
    }
    if (!ok) {
      runtime_error(line, "Could not load or install cursor '%s' (error %lu).",
                    path, (unsigned long)GetLastError());
      return v_bool(false);
    }
    return v_bool(true);
  }
  if (strcmp(name, "mouse_restore_system_cursor") == 0) {
    if (argc != 0) {
      runtime_error(line, "mouse_restore_system_cursor expects no arguments.");
      return v_null();
    }
    return v_bool(
        SystemParametersInfoA(SPI_SETCURSORS, 0, NULL, SPIF_SENDCHANGE) != 0);
  }
  if (strcmp(name, "mouse_is_available") == 0) {
    if (argc != 0) {
      runtime_error(line, "mouse_is_available expects no arguments.");
      return v_null();
    }
    return v_bool(true);
  }
  if (strcmp(name, "mouse_wait_button") == 0) {
    if (argc != 3 || a[0].type != V_STRING || a[1].type != V_BOOL) {
      runtime_error(line, "mouse_wait_button(button, down, timeout_ms) expects "
                          "string, boolean, integer.");
      return v_null();
    }
    long ms = 0;
    if (!numeric_to_long(a[2], &ms) || ms < 0 || ms > 86400000) {
      runtime_error(line, "mouse_wait_button timeout must be 0..86400000 ms.");
      return v_null();
    }
    int b = mouse_button_code(a[0].as.string);
    if (!b) {
      runtime_error(line, "Fluorine mouse button '%s'.", a[0].as.string);
      return v_null();
    }
    int vk = b == 1 ? VK_LBUTTON : b == 2 ? VK_RBUTTON : VK_MBUTTON;
    for (long elapsed = 0; elapsed <= ms; elapsed += 10) {
      bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
      if (down == a[1].as.boolean)
        return v_bool(true);
      if (elapsed < ms)
        Sleep(10);
    }
    return v_bool(false);
  }
  if (strcmp(name, "keyboard_wait_key") == 0) {
    if (argc != 2 || a[0].type != V_STRING) {
      runtime_error(line, "keyboard_wait_key(key, timeout_ms) expects key "
                          "string and integer timeout.");
      return v_null();
    }
    long ms = 0;
    if (!numeric_to_long(a[1], &ms) || ms < 0 || ms > 86400000) {
      runtime_error(line, "keyboard_wait_key timeout must be 0..86400000 ms.");
      return v_null();
    }
    SHORT v = VkKeyScanA(a[0].as.string[0]);
    if (v == -1) {
      runtime_error(line, "Unsupported key name.");
      return v_null();
    }
    for (long elapsed = 0; elapsed <= ms; elapsed += 10) {
      if (GetAsyncKeyState((int)(v & 0xff)) & 0x8000)
        return v_bool(true);
      if (elapsed < ms)
        Sleep(10);
    }
    return v_bool(false);
  }
  if (strcmp(name, "keyboard_type_text") == 0) {
    if (argc != 1 || a[0].type != V_STRING) {
      runtime_error(line, "keyboard_type_text expects text.");
      return v_null();
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, a[0].as.string, -1, NULL, 0);
    if (n <= 0)
      return v_bool(false);
    WCHAR *wide = (WCHAR *)malloc((size_t)n * sizeof(WCHAR));
    if (!wide)
      exit(2);
    MultiByteToWideChar(CP_UTF8, 0, a[0].as.string, -1, wide, n);
    for (int i = 0; i < n - 1; i++) {
      INPUT in[2];
      memset(in, 0, sizeof(in));
      in[0].type = in[1].type = INPUT_KEYBOARD;
      in[0].ki.wScan = in[1].ki.wScan = wide[i];
      in[0].ki.dwFlags = KEYEVENTF_UNICODE;
      in[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
      if (SendInput(2, in, sizeof(INPUT)) != 2) {
        free(wide);
        runtime_error(line, "SendInput failed while typing text.");
        return v_bool(false);
      }
    }
    free(wide);
    return v_null();
  }
  if (strcmp(name, "keyboard_press") == 0 ||
      strcmp(name, "keyboard_key_down") == 0 ||
      strcmp(name, "keyboard_key_up") == 0) {
    if (argc != 1 || a[0].type != V_STRING || !a[0].as.string[0]) {
      runtime_error(line, "%s expects one key name or character.", name);
      return v_null();
    }
    const char *k = a[0].as.string;
    struct KeyMap {
      const char *n;
      WORD vk;
    };
    static const struct KeyMap keys[] = {
        {"enter", VK_RETURN},   {"return", VK_RETURN},   {"esc", VK_ESCAPE},
        {"escape", VK_ESCAPE},  {"tab", VK_TAB},         {"space", VK_SPACE},
        {"backspace", VK_BACK}, {"delete", VK_DELETE},   {"insert", VK_INSERT},
        {"home", VK_HOME},      {"end", VK_END},         {"pageup", VK_PRIOR},
        {"pagedown", VK_NEXT},  {"left", VK_LEFT},       {"right", VK_RIGHT},
        {"up", VK_UP},          {"down", VK_DOWN},       {"shift", VK_SHIFT},
        {"ctrl", VK_CONTROL},   {"control", VK_CONTROL}, {"alt", VK_MENU},
        {"win", VK_LWIN},       {"f1", VK_F1},           {"f2", VK_F2},
        {"f3", VK_F3},          {"f4", VK_F4},           {"f5", VK_F5},
        {"f6", VK_F6},          {"f7", VK_F7},           {"f8", VK_F8},
        {"f9", VK_F9},          {"f10", VK_F10},         {"f11", VK_F11},
        {"f12", VK_F12}};
    WORD vk = 0;
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
      if (_stricmp(k, keys[i].n) == 0) {
        vk = keys[i].vk;
        break;
      }
    SHORT scan = 0;
    WORD mods = 0;
    if (!vk) {
      scan = VkKeyScanA(k[0]);
      if (scan == -1) {
        runtime_error(line, "Unsupported key name '%s'.", k);
        return v_null();
      }
      vk = (WORD)(scan & 0xff);
      if (scan & 0x0100)
        mods |= VK_SHIFT;
      if (scan & 0x0200)
        mods |= VK_CONTROL;
      if (scan & 0x0400)
        mods |= VK_MENU;
    }
    WORD modifier_keys[3] = {VK_SHIFT, VK_CONTROL, VK_MENU};
    INPUT in[8];
    memset(in, 0, sizeof(in));
    UINT count = 0;
    if (strcmp(name, "keyboard_press") == 0) {
      for (int mi = 0; mi < 3; mi++)
        if (mods & modifier_keys[mi]) {
          in[count].type = INPUT_KEYBOARD;
          in[count++].ki.wVk = modifier_keys[mi];
        }
      in[count].type = INPUT_KEYBOARD;
      in[count++].ki.wVk = vk;
      in[count].type = INPUT_KEYBOARD;
      in[count].ki.wVk = vk;
      in[count++].ki.dwFlags = KEYEVENTF_KEYUP;
      for (int mi = 2; mi >= 0; mi--)
        if (mods & modifier_keys[mi]) {
          in[count].type = INPUT_KEYBOARD;
          in[count].ki.wVk = modifier_keys[mi];
          in[count++].ki.dwFlags = KEYEVENTF_KEYUP;
        }
    } else if (strcmp(name, "keyboard_key_down") == 0) {
      for (int mi = 0; mi < 3; mi++)
        if (mods & modifier_keys[mi]) {
          in[count].type = INPUT_KEYBOARD;
          in[count++].ki.wVk = modifier_keys[mi];
        }
      in[count].type = INPUT_KEYBOARD;
      in[count++].ki.wVk = vk;
    } else {
      in[count].type = INPUT_KEYBOARD;
      in[count].ki.wVk = vk;
      in[count++].ki.dwFlags = KEYEVENTF_KEYUP;
      for (int mi = 2; mi >= 0; mi--)
        if (mods & modifier_keys[mi]) {
          in[count].type = INPUT_KEYBOARD;
          in[count].ki.wVk = modifier_keys[mi];
          in[count++].ki.dwFlags = KEYEVENTF_KEYUP;
        }
    }
    if (SendInput(count, in, sizeof(INPUT)) != count) {
      runtime_error(line, "Windows could not send the requested key input.");
      return v_bool(false);
    }
    return v_null();
  }
  if (strcmp(name, "keyboard_is_key_down") == 0) {
    if (argc != 1 || a[0].type != V_STRING || !a[0].as.string[0]) {
      runtime_error(line, "keyboard_is_key_down expects key string.");
      return v_null();
    }
    SHORT vk = VkKeyScanA(a[0].as.string[0]);
    if (vk == -1)
      return v_bool(false);
    return v_bool((GetAsyncKeyState((int)(vk & 0xff)) & 0x8000) != 0);
  }
#else
  (void)a;
  if (strcmp(name, "mouse_is_available") == 0) {
    if (argc != 0) {
      runtime_error(line, "mouse_is_available expects no arguments.");
      return v_null();
    }
    return v_bool(false);
  }
  runtime_error(line,
                "%s is not available in this build: a native desktop input "
                "backend is required (Windows is supported).",
                name);
  return v_null();
#endif
  runtime_error(line, "Fluorine input operation '%s'.", name);
  return v_null();
}

static Value native_os(const char *name, int argc, Value *a, int line) {
  if (strcmp(name, "os_name") == 0) {
    if (argc)
      return (runtime_error(line, "os_name expects no arguments."), v_null());
#ifdef _WIN32
    return v_string("windows");
#elif defined(__APPLE__)
    return v_string("macos");
#elif defined(__linux__)
    return v_string("linux");
#elif defined(__FreeBSD__)
    return v_string("freebsd");
#else
    return v_string("unknown");
#endif
  }
  if (strcmp(name, "os_version") == 0) {
    if (argc)
      return (runtime_error(line, "os_version expects no arguments."),
              v_null());
#ifdef _WIN32
    {
      OSVERSIONINFOA v;
      memset(&v, 0, sizeof(v));
      v.dwOSVersionInfoSize = sizeof(v);
      if (GetVersionExA(&v)) {
        char b[128];
        snprintf(
            b, sizeof(b), "%lu.%lu build %lu", (unsigned long)v.dwMajorVersion,
            (unsigned long)v.dwMinorVersion, (unsigned long)v.dwBuildNumber);
        return v_string(b);
      }
      return v_string("Windows (version unavailable)");
    }
#else
    {
      struct utsname u;
      if (uname(&u) == 0)
        return v_string(u.release);
      return v_string("unknown");
    }
#endif
  }
  if (strcmp(name, "os_architecture") == 0) {
    if (argc)
      return (runtime_error(line, "os_architecture expects no arguments."),
              v_null());
#if defined(_M_X64) || defined(__x86_64__)
    return v_string("x86_64");
#elif defined(_M_IX86) || defined(__i386__)
    return v_string("x86");
#elif defined(_M_ARM64) || defined(__aarch64__)
    return v_string("arm64");
#elif defined(_M_ARM) || defined(__arm__)
    return v_string("arm");
#else
    return v_string("unknown");
#endif
  }
  if (strcmp(name, "os_hostname") == 0) {
    if (argc)
      return (runtime_error(line, "os_hostname expects no arguments."),
              v_null());
    char b[256] = {0};
#ifdef _WIN32
    DWORD n = (DWORD)sizeof(b);
    if (GetComputerNameA(b, &n))
      return v_string(b);
#else
    if (gethostname(b, sizeof(b) - 1) == 0)
      return v_string(b);
#endif
    return v_string("unknown");
  }
  if (strcmp(name, "os_username") == 0) {
    if (argc)
      return (runtime_error(line, "os_username expects no arguments."),
              v_null());
    const char *u = getenv("USERNAME");
    if (!u || !*u)
      u = getenv("USER");
    if (u && *u)
      return v_string(u);
    return v_string("unknown");
  }
  if (strcmp(name, "os_path_join") == 0) {
    if (argc != 2 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line))
      return v_null();
    const char *x = a[0].as.string, *y = a[1].as.string;
    size_t nx = strlen(x), ny = strlen(y);
    bool absolute = ny > 0 && (y[0] == '/' || y[0] == '\\');
    if (absolute)
      return v_string(y);
    bool slash = nx > 0 && x[nx - 1] != '/' && x[nx - 1] != '\\';
    char *out = (char *)malloc(nx + ny + (slash ? 2 : 1));
    if (!out)
      exit(2);
    memcpy(out, x, nx);
    size_t off = nx;
    if (slash)
      out[off++] = '/';
    memcpy(out + off, y, ny + 1);
    Value v = v_string(out);
    free(out);
    return v;
  }
  if (strcmp(name, "os_basename") == 0 || strcmp(name, "os_dirname") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    const char *path = a[0].as.string;
    size_t n = strlen(path);
    while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\'))
      n--;
    size_t split = n;
    while (split > 0 && path[split - 1] != '/' && path[split - 1] != '\\')
      split--;
    if (strcmp(name, "os_basename") == 0) {
      char *q = copy_n(path + split, n - split);
      Value v = v_string(q);
      free(q);
      return v;
    }
    if (split == 0)
      return v_string(".");
    while (split > 1 && (path[split - 1] == '/' || path[split - 1] == '\\'))
      split--;
    char *q = copy_n(path, split);
    Value v = v_string(q);
    free(q);
    return v;
  }
  if (strcmp(name, "os_cwd") == 0) {
    if (argc)
      return (runtime_error(line, "os_cwd expects no arguments."), v_null());
    char b[4096];
    if (getcwd(b, sizeof(b)))
      return v_string(b);
    runtime_error(line, "getcwd failed: %s", strerror(errno));
    return v_null();
  }
  if (strcmp(name, "os_cd") == 0 || strcmp(name, "os_chdir") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
#ifdef _WIN32
    if (!SetCurrentDirectoryA(a[0].as.string)) {
      runtime_error(line, "%s('%s') failed (error %lu).", name, a[0].as.string,
                    (unsigned long)GetLastError());
      return v_bool(false);
    }
#else
    if (chdir(a[0].as.string) != 0) {
      runtime_error(line, "%s('%s') failed: %s", name, a[0].as.string,
                    strerror(errno));
      return v_bool(false);
    }
#endif
    return v_bool(true);
  }
  if (strcmp(name, "os_mkdir") == 0 || strcmp(name, "os_makedirs") == 0) {
    if ((argc != 1 && !(strcmp(name, "os_mkdir") == 0 && argc == 2)) ||
        !std_string_arg(name, argc, a, 0, line))
      return v_null();
    if (argc == 2 && a[1].type != V_BOOL) {
      runtime_error(line,
                    "os_mkdir optional parents argument must be boolean.");
      return v_null();
    }
#ifdef _WIN32
    if (CreateDirectoryA(a[0].as.string, NULL))
      return v_bool(true);
#else
    if (mkdir(a[0].as.string, 0777) == 0)
      return v_bool(true);
#endif
    if (errno == EEXIST) {
      struct stat st;
      if (stat(a[0].as.string, &st) == 0 && S_ISDIR(st.st_mode))
        return v_bool(true);
    }
    if (argc == 2 && a[1].as.boolean) {
      char *path = copy_s(a[0].as.string);
      size_t n = strlen(path);
      for (size_t i = 1; i <= n; i++) {
        if (path[i] == '/' || path[i] == '\\' || path[i] == '\0') {
          char save = path[i];
          path[i] = '\0';
          if (path[0]) {
#ifdef _WIN32
            if (!CreateDirectoryA(path, NULL) &&
                GetLastError() != ERROR_ALREADY_EXISTS) {
              free(path);
              return v_bool(false);
            }
#else
            if (mkdir(path, 0777) != 0 && errno != EEXIST) {
              free(path);
              return v_bool(false);
            }
#endif
          }
          path[i] = save;
        }
      }
      free(path);
      return v_bool(true);
    }
    return v_bool(false);
  }
  if (strcmp(name, "os_rmdir") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
#ifdef _WIN32
    return v_bool(RemoveDirectoryA(a[0].as.string) != 0);
#else
    return v_bool(rmdir(a[0].as.string) == 0);
#endif
  }
  if (strcmp(name, "os_exists") == 0 || strcmp(name, "os_is_file") == 0 ||
      strcmp(name, "os_is_dir") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    struct stat st;
    bool ok = stat(a[0].as.string, &st) == 0;
    if (strcmp(name, "os_exists") == 0)
      return v_bool(ok);
    if (strcmp(name, "os_is_dir") == 0)
      return v_bool(ok && S_ISDIR(st.st_mode));
    return v_bool(ok && S_ISREG(st.st_mode));
  }
  if (strcmp(name, "os_remove") == 0 || strcmp(name, "os_unlink") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    return v_bool(remove(a[0].as.string) == 0);
  }
  if (strcmp(name, "os_rename") == 0 || strcmp(name, "os_copy_file") == 0) {
    if (argc != 2 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line))
      return v_null();
    if (strcmp(name, "os_rename") == 0)
      return v_bool(rename(a[0].as.string, a[1].as.string) == 0);
    FILE *src = fopen(a[0].as.string, "rb");
    if (!src)
      return v_bool(false);
    FILE *dst = fopen(a[1].as.string, "wb");
    if (!dst) {
      fclose(src);
      return v_bool(false);
    }
    char buf[16384];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
      if (fwrite(buf, 1, n, dst) != n) {
        ok = false;
        break;
      }
    if (ferror(src))
      ok = false;
    if (fclose(src))
      ok = false;
    if (fclose(dst))
      ok = false;
    if (!ok)
      remove(a[1].as.string);
    return v_bool(ok);
  }
  if (strcmp(name, "os_getenv") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    const char *v = getenv(a[0].as.string);
    return v ? v_string(v) : v_null();
  }
  if (strcmp(name, "os_setenv") == 0) {
    if (argc != 2 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line))
      return v_null();
#ifdef _WIN32
    return v_bool(SetEnvironmentVariableA(a[0].as.string, a[1].as.string) != 0);
#else
    return v_bool(setenv(a[0].as.string, a[1].as.string, 1) == 0);
#endif
  }
  if (strcmp(name, "os_environ") == 0) {
    if (argc)
      return (runtime_error(line, "os_environ expects no arguments."),
              v_null());
    Value out = v_map();
#ifdef _WIN32
    {
      LPCH block = GetEnvironmentStringsA();
      if (block) {
        for (const char *p = block; *p;) {
          const char *eq = strchr(p, '=');
          if (eq && eq > p) {
            char *k = copy_n(p, (size_t)(eq - p));
            Value key = v_string(k), val = v_string(eq + 1);
            map_set(out.as.map, key, val);
            free_value(key);
            free_value(val);
            free(k);
          }
          p += strlen(p) + 1;
        }
        FreeEnvironmentStringsA(block);
      }
    }
#else
    {
      extern char **environ;
      for (char **p = environ; p && *p; p++) {
        const char *eq = strchr(*p, '=');
        if (eq) {
          char *k = copy_n(*p, (size_t)(eq - *p));
          Value key = v_string(k), val = v_string(eq + 1);
          map_set(out.as.map, key, val);
          free_value(key);
          free_value(val);
          free(k);
        }
      }
    }
#endif
    return out;
  }
  if (strcmp(name, "os_listdir") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    Value out = v_array();
#ifdef _WIN32
    {
      char pattern[4096];
      int n = snprintf(pattern, sizeof(pattern), "%s\\*", a[0].as.string);
      if (n < 0 || (size_t)n >= sizeof(pattern))
        return out;
      WIN32_FIND_DATAA fd;
      HANDLE h = FindFirstFileA(pattern, &fd);
      if (h == INVALID_HANDLE_VALUE)
        return out;
      do {
        if (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..")) {
          Value v = v_string(fd.cFileName);
          Array *ar = out.as.array;
          if (ar->count == ar->capacity) {
            ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
            ar->items = (Value *)xrealloc(ar->items,
                                          (size_t)ar->capacity * sizeof(Value));
          }
          ar->items[ar->count++] = v;
        }
      } while (FindNextFileA(h, &fd));
      FindClose(h);
    }
#else
    {
      DIR *d = opendir(a[0].as.string);
      if (!d)
        return out;
      struct dirent *de;
      while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
          Value v = v_string(de->d_name);
          Array *ar = out.as.array;
          if (ar->count == ar->capacity) {
            ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
            ar->items = (Value *)xrealloc(ar->items,
                                          (size_t)ar->capacity * sizeof(Value));
          }
          ar->items[ar->count++] = v;
        }
      }
      closedir(d);
    }
#endif
    return out;
  }
  if (strcmp(name, "os_home_dir") == 0) {
    if (argc)
      return (runtime_error(line, "os_home_dir expects no arguments."),
              v_null());
    const char *v = getenv("HOME");
    if (!v)
      v = getenv("USERPROFILE");
    return v ? v_string(v) : v_null();
  }
  if (strcmp(name, "os_temp_dir") == 0) {
    if (argc)
      return (runtime_error(line, "os_temp_dir expects no arguments."),
              v_null());
#ifdef _WIN32
    char b[MAX_PATH + 1];
    DWORD n = GetTempPathA(MAX_PATH, b);
    return n && n <= MAX_PATH ? v_string(b) : v_string(".");
#else
    const char *v = getenv("TMPDIR");
    return v ? v_string(v) : v_string("/tmp");
#endif
  }
  if (strcmp(name, "os_executable_path") == 0) {
    if (argc)
      return (runtime_error(line, "os_executable_path expects no arguments."),
              v_null());
#ifdef _WIN32
    char b[32768];
    DWORD n = GetModuleFileNameA(NULL, b, (DWORD)sizeof(b));
    if (n && n < sizeof(b))
      return v_string(b);
    return v_null();
#elif defined(__linux__)
    {
      char b[4096];
      ssize_t n = readlink("/proc/self/exe", b, sizeof(b) - 1);
      if (n >= 0) {
        b[n] = '\0';
        return v_string(b);
      }
      return v_null();
    }
#else
    return v_null();
#endif
  }
  if (strcmp(name, "os_process_id") == 0) {
    if (argc)
      return (runtime_error(line, "os_process_id expects no arguments."),
              v_null());
#ifdef _WIN32
    return v_num((double)GetCurrentProcessId());
#else
    return v_num((double)getpid());
#endif
  }
  if (strcmp(name, "os_sleep") == 0) {
    long ms = 0;
    if (argc != 1 || !numeric_to_long(a[0], &ms) || ms < 0 || ms > 86400000) {
      runtime_error(line, "os_sleep expects milliseconds from 0 to 86400000.");
      return v_null();
    }
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    {
      struct timespec ts;
      ts.tv_sec = (time_t)(ms / 1000);
      ts.tv_nsec = (long)(ms % 1000) * 1000000L;
      while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
      }
    }
#endif
    return v_null();
  }
  if (strcmp(name, "os_timestamp") == 0) {
    if (argc)
      return (runtime_error(line, "os_timestamp expects no arguments."),
              v_null());
    return v_num((double)time(NULL));
  }
  if (strcmp(name, "os_last_error") == 0) {
    if (argc)
      return (runtime_error(line, "os_last_error expects no arguments."),
              v_null());
    return v_string(strerror(errno));
  }
  if (strcmp(name, "os_system") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    return v_num((double)system(a[0].as.string));
  }
  if (strcmp(name, "os_execute") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    size_t cap = 4096, used = 0;
    char *output = (char *)malloc(cap);
    if (!output)
      exit(2);
    output[0] = '\0';
    size_t cmdn = strlen(a[0].as.string);
    char *cmd = (char *)malloc(cmdn + 8);
    if (!cmd)
      exit(2);
    snprintf(cmd, cmdn + 8, "%s 2>&1", a[0].as.string);
#ifdef _WIN32
    FILE *pipe = _popen(cmd, "r");
#else
    FILE *pipe = popen(cmd, "r");
#endif
    free(cmd);
    if (!pipe) {
      free(output);
      runtime_error(line, "os_execute could not start command: %s",
                    strerror(errno));
      return v_null();
    }
    char buf[2048];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0) {
      if (used + n > 16 * 1024 * 1024) {
        free(output);
#ifdef _WIN32
        (void)_pclose(pipe);
#else
        (void)pclose(pipe);
#endif
        runtime_error(line, "os_execute output exceeded 16 MiB.");
        return v_null();
      }
      if (used + n + 1 > cap) {
        while (cap < used + n + 1)
          cap *= 2;
        output = (char *)xrealloc(output, cap);
      }
      memcpy(output + used, buf, n);
      used += n;
      output[used] = '\0';
    }
    int status;
#ifdef _WIN32
    status = _pclose(pipe);
#else
    status = pclose(pipe);
    if (status != -1 && WIFEXITED(status))
      status = WEXITSTATUS(status);
    else if (status != -1 && WIFSIGNALED(status))
      status = 128 + WTERMSIG(status);
#endif
    Value out = v_map(), k1 = v_string("output"), v1 = v_string(output),
          k2 = v_string("status"), v2 = v_num((double)status);
    map_set(out.as.map, k1, v1);
    map_set(out.as.map, k2, v2);
    free_value(k1);
    free_value(v1);
    free_value(k2);
    free_value(v2);
    free(output);
    return out;
  }
  if (strcmp(name, "os_uptime") == 0) {
    if (argc)
      return (runtime_error(line, "os_uptime expects no arguments."), v_null());
#ifdef _WIN32
    return v_num((double)GetTickCount64() / 1000.0);
#else
    {
      struct timespec ts;
      if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return v_num((double)ts.tv_sec + (double)ts.tv_nsec / 1e9);
      return v_num(-1);
    }
#endif
  }
  if (strcmp(name, "os_shell_execute") == 0) {
    if (argc < 2 || argc > 3 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line) ||
        (argc == 3 && !std_string_arg(name, argc, a, 2, line))) {
      runtime_error(
          line,
          "os_shell_execute(operation, target, [arguments]) expects strings.");
      return v_null();
    }
#ifdef _WIN32
    {
      HINSTANCE r =
          ShellExecuteA(NULL, a[0].as.string, a[1].as.string,
                        argc == 3 ? a[2].as.string : NULL, NULL, SW_SHOWNORMAL);
      return v_bool((INT_PTR)r > 32);
    }
#else
    if (_stricmp(a[0].as.string, "open") != 0) {
      runtime_error(line,
                    "os_shell_execute operation is only supported on Windows.");
      return v_bool(false);
    }
    pid_t pid = fork();
    if (pid < 0)
      return v_bool(false);
    if (pid == 0) {
      pid_t w = fork();
      if (w < 0)
        _exit(127);
      if (w > 0)
        _exit(0);
      setsid();
      execlp("xdg-open", "xdg-open", a[1].as.string, (char *)NULL);
      _exit(127);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    return v_bool(WIFEXITED(st) && WEXITSTATUS(st) == 0);
#endif
  }
  if (strcmp(name, "os_open_url") == 0 || strcmp(name, "os_open_file") == 0 ||
      strcmp(name, "os_open_folder") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
#ifdef _WIN32
    {
      HINSTANCE r = ShellExecuteA(NULL, "open", a[0].as.string, NULL, NULL,
                                  SW_SHOWNORMAL);
      return v_bool((INT_PTR)r > 32);
    }
#else
    {
      pid_t pid = fork();
      if (pid < 0)
        return v_bool(false);
      if (pid == 0) {
        pid_t w = fork();
        if (w < 0)
          _exit(127);
        if (w > 0)
          _exit(0);
        setsid();
        execlp("xdg-open", "xdg-open", a[0].as.string, (char *)NULL);
        _exit(127);
      }
      int st = 0;
      waitpid(pid, &st, 0);
      return v_bool(WIFEXITED(st) && WEXITSTATUS(st) == 0);
    }
#endif
  }
  if (strcmp(name, "os_spawn") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
#ifdef _WIN32
    {
      STARTUPINFOA si;
      PROCESS_INFORMATION pi;
      memset(&si, 0, sizeof(si));
      memset(&pi, 0, sizeof(pi));
      si.cb = sizeof(si);
      size_t n = strlen(a[0].as.string);
      char *cmd = (char *)malloc(n + 3);
      if (!cmd)
        exit(2);
      snprintf(cmd, n + 3, "\"%s\"", a[0].as.string);
      BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                               CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi);
      free(cmd);
      if (!ok) {
        runtime_error(line, "os_spawn failed (error %lu).",
                      (unsigned long)GetLastError());
        return v_null();
      }
      DWORD pid = pi.dwProcessId;
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      return v_num((double)pid);
    }
#else
    {
      pid_t pid = fork();
      if (pid < 0) {
        runtime_error(line, "os_spawn failed: %s", strerror(errno));
        return v_null();
      }
      if (pid == 0) {
        execlp(a[0].as.string, a[0].as.string, (char *)NULL);
        _exit(127);
      }
      return v_num((double)pid);
    }
#endif
  }
  if (strcmp(name, "os_wait") == 0) {
    long pid = 0;
    if (argc != 1 || !numeric_to_long(a[0], &pid) || pid <= 0) {
      runtime_error(line, "os_wait expects a positive process id.");
      return v_null();
    }
#ifdef _WIN32
    {
      HANDLE h = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
                             FALSE, (DWORD)pid);
      if (!h)
        return v_num(-1);
      WaitForSingleObject(h, INFINITE);
      DWORD code = 0;
      BOOL ok = GetExitCodeProcess(h, &code);
      CloseHandle(h);
      return v_num(ok ? (double)code : -1);
    }
#else
    {
      int status = 0;
      pid_t r;
      do {
        r = waitpid((pid_t)pid, &status, 0);
      } while (r < 0 && errno == EINTR);
      if (r < 0)
        return v_num(-1);
      if (WIFEXITED(status))
        return v_num((double)WEXITSTATUS(status));
      if (WIFSIGNALED(status))
        return v_num((double)(128 + WTERMSIG(status)));
      return v_num(-1);
    }
#endif
  }
  if (strcmp(name, "os_terminate") == 0) {
    long pid = 0;
    if (argc != 1 || !numeric_to_long(a[0], &pid) || pid <= 0) {
      runtime_error(line, "os_terminate expects a positive process id.");
      return v_null();
    }
#ifdef _WIN32
    {
      HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
      if (!h)
        return v_bool(false);
      BOOL ok = TerminateProcess(h, 1);
      CloseHandle(h);
      return v_bool(ok != 0);
    }
#else
    return v_bool(kill((pid_t)pid, SIGTERM) == 0);
#endif
  }
  if (strcmp(name, "os_message_box") == 0) {
    if ((argc != 2 && argc != 3) || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line) ||
        (argc == 3 && !std_string_arg(name, argc, a, 2, line))) {
      runtime_error(line,
                    "os_message_box(title, message, [kind]) expects strings.");
      return v_null();
    }
#ifdef _WIN32
    {
      UINT flags = MB_OK | MB_ICONINFORMATION;
      if (argc == 3) {
        if (_stricmp(a[2].as.string, "warning") == 0)
          flags = MB_OK | MB_ICONWARNING;
        else if (_stricmp(a[2].as.string, "error") == 0)
          flags = MB_OK | MB_ICONERROR;
        else if (_stricmp(a[2].as.string, "question") == 0)
          flags = MB_YESNO | MB_ICONQUESTION;
      }
      return v_num(
          (double)MessageBoxA(NULL, a[1].as.string, a[0].as.string, flags));
    }
#else
    runtime_error(line,
                  "os_message_box is only supported on Windows in this build.");
    return v_null();
#endif
  }
  if (strcmp(name, "os_lock_screen") == 0) {
    if (argc)
      return (runtime_error(line, "os_lock_screen expects no arguments."),
              v_null());
#ifdef _WIN32
    return v_bool(LockWorkStation() != 0);
#else
    runtime_error(line,
                  "os_lock_screen is only supported on Windows in this build.");
    return v_bool(false);
#endif
  }
  if (strcmp(name, "os_logoff") == 0 || strcmp(name, "os_shutdown") == 0 ||
      strcmp(name, "os_restart") == 0) {
    if (argc != 1 || a[0].type != V_BOOL || !a[0].as.boolean) {
      runtime_error(line, "%s requires the explicit boolean confirmation true.",
                    name);
      return v_bool(false);
    }
#ifdef _WIN32
    UINT flags = strcmp(name, "os_logoff") == 0    ? EWX_LOGOFF
                 : strcmp(name, "os_restart") == 0 ? EWX_REBOOT
                                                   : EWX_SHUTDOWN;
    return v_bool(ExitWindowsEx(flags | EWX_FORCEIFHUNG,
                                SHTDN_REASON_MAJOR_APPLICATION |
                                    SHTDN_REASON_FLAG_PLANNED) != 0);
#else
    runtime_error(line,
                  "%s is unsupported by this build; no system session change "
                  "was attempted.",
                  name);
    return v_bool(false);
#endif
  }
  if (strcmp(name, "os_is_admin") == 0) {
    if (argc)
      return (runtime_error(line, "os_is_admin expects no arguments."),
              v_null());
#ifdef _WIN32
    BOOL admin = FALSE;
    PSID sid = NULL;
    SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&nt, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &sid)) {
      CheckTokenMembership(NULL, sid, &admin);
      FreeSid(sid);
    }
    return v_bool(admin != FALSE);
#else
    return v_bool(geteuid() == 0);
#endif
  }
  runtime_error(line, "Fluorine OS operation '%s'.", name);
  return v_null();
}

/* Small dependency-free standard-library primitives. */
static Value native_uuid_v4(int argc, Value *a, int line) {
  (void)a;
  if (argc) {
    runtime_error(line, "uuid_v4 expects no arguments.");
    return v_null();
  }
  unsigned char b[16];
  size_t n = 0;
#ifndef _WIN32
  FILE *f = fopen("/dev/urandom", "rb");
  if (f) {
    n = fread(b, 1, sizeof(b), f);
    fclose(f);
  }
#endif
#ifdef _WIN32
  if (n != sizeof(b)) {
    HMODULE adv = GetModuleHandleA("advapi32.dll");
    if (!adv)
      adv = LoadLibraryA("advapi32.dll");
    if (adv) {
      typedef BOOLEAN(WINAPI * RngFn)(PVOID, ULONG);
      RngFn rng = (RngFn)GetProcAddress(adv, "SystemFunction036");
      if (rng && rng(b, (ULONG)sizeof(b)))
        n = sizeof(b);
    }
  }
#endif
  if (n != sizeof(b)) {
    static int seeded = 0;
    if (!seeded) {
      srand((unsigned)(time(NULL) ^ (uintptr_t)&b));
      seeded = 1;
    }
    for (size_t i = 0; i < sizeof(b); i++)
      b[i] = (unsigned char)(rand() & 255);
  }
  b[6] = (unsigned char)((b[6] & 15) | 64);
  b[8] = (unsigned char)((b[8] & 63) | 128);
  char out[37];
  snprintf(
      out, sizeof(out),
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11],
      b[12], b[13], b[14], b[15]);
  return v_string(out);
}
static Value native_uuid_v7(int argc, Value *a, int line) {
  (void)a;
  if (argc) {
    runtime_error(line, "std_uuid_v7 expects no arguments.");
    return v_null();
  }
  unsigned char b[16];
  size_t n = 0;
#ifndef _WIN32
  FILE *f = fopen("/dev/urandom", "rb");
  if (f) {
    n = fread(b, 1, sizeof(b), f);
    fclose(f);
  }
#else
  {
    HMODULE adv = GetModuleHandleA("advapi32.dll");
    if (!adv)
      adv = LoadLibraryA("advapi32.dll");
    if (adv) {
      typedef BOOLEAN(WINAPI * RngFn)(PVOID, ULONG);
      RngFn rng = (RngFn)GetProcAddress(adv, "SystemFunction036");
      if (rng && rng(b, (ULONG)sizeof(b)))
        n = sizeof(b);
    }
  }
#endif
  if (n != sizeof(b)) {
    static int seeded = 0;
    if (!seeded) {
      srand((unsigned)(time(NULL) ^ (uintptr_t)&b));
      seeded = 1;
    }
    for (size_t i = 0; i < sizeof(b); i++)
      b[i] = (unsigned char)(rand() & 255);
  }
  uint64_t ms = (uint64_t)time(NULL) * 1000u;
#ifdef _WIN32
  {
    FILETIME ft;
    ULARGE_INTEGER ui;
    GetSystemTimeAsFileTime(&ft);
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    ms = (uint64_t)((ui.QuadPart - 116444736000000000ULL) / 10000ULL);
  }
#else
  {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
      ms = (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
  }
#endif
  b[0] = (unsigned char)(ms >> 40);
  b[1] = (unsigned char)(ms >> 32);
  b[2] = (unsigned char)(ms >> 24);
  b[3] = (unsigned char)(ms >> 16);
  b[4] = (unsigned char)(ms >> 8);
  b[5] = (unsigned char)ms;
  b[6] = (unsigned char)((b[6] & 15) | 0x70);
  b[8] = (unsigned char)((b[8] & 63) | 0x80);
  char out[37];
  snprintf(
      out, sizeof(out),
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11],
      b[12], b[13], b[14], b[15]);
  return v_string(out);
}
static uint32_t sha_rotr(uint32_t x, unsigned n) {
  return (x >> n) | (x << (32 - n));
}
static Value native_sha256(int argc, Value *a, int line) {
  static const uint32_t k[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
      0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
      0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
      0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
      0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
  if (argc != 1 || a[0].type != V_STRING) {
    runtime_error(line, "sha256 expects one string.");
    return v_null();
  }
  const unsigned char *src = (const unsigned char *)a[0].as.string;
  size_t len = strlen((const char *)src);
  if (len > SIZE_MAX - 72) {
    runtime_error(line, "sha256 input too large.");
    return v_null();
  }
  size_t total = ((len + 9 + 63) / 64) * 64;
  unsigned char *m = (unsigned char *)calloc(total, 1);
  if (!m)
    exit(2);
  memcpy(m, src, len);
  m[len] = 128;
  uint64_t bits = (uint64_t)len * 8;
  for (int i = 0; i < 8; i++)
    m[total - 1 - (size_t)i] = (unsigned char)(bits >> (i * 8));
  uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                   0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  for (size_t off = 0; off < total; off += 64) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
      w[i] = ((uint32_t)m[off + 4 * i] << 24) |
             ((uint32_t)m[off + 4 * i + 1] << 16) |
             ((uint32_t)m[off + 4 * i + 2] << 8) | m[off + 4 * i + 3];
    for (int i = 16; i < 64; i++) {
      uint32_t x = sha_rotr(w[i - 15], 7) ^ sha_rotr(w[i - 15], 18) ^
                   (w[i - 15] >> 3),
               y = sha_rotr(w[i - 2], 17) ^ sha_rotr(w[i - 2], 19) ^
                   (w[i - 2] >> 10);
      w[i] = w[i - 16] + x + w[i - 7] + y;
    }
    uint32_t A = h[0], B = h[1], C = h[2], D = h[3], E = h[4], F = h[5],
             G = h[6], H = h[7];
    for (int i = 0; i < 64; i++) {
      uint32_t x = sha_rotr(E, 6) ^ sha_rotr(E, 11) ^ sha_rotr(E, 25),
               ch = (E & F) ^ (~E & G), t1 = H + x + ch + k[i] + w[i],
               y = sha_rotr(A, 2) ^ sha_rotr(A, 13) ^ sha_rotr(A, 22),
               maj = (A & B) ^ (A & C) ^ (B & C), t2 = y + maj;
      H = G;
      G = F;
      F = E;
      E = D + t1;
      D = C;
      C = B;
      B = A;
      A = t1 + t2;
    }
    h[0] += A;
    h[1] += B;
    h[2] += C;
    h[3] += D;
    h[4] += E;
    h[5] += F;
    h[6] += G;
    h[7] += H;
  }
  free(m);
  char out[65];
  for (int i = 0; i < 8; i++)
    snprintf(out + 8 * i, 9, "%08x", h[i]);
  out[64] = 0;
  return v_string(out);
}
static void js_append(char **b, size_t *n, size_t *c, const char *s, size_t z) {
  if (*n + z + 1 > *c) {
    while (*n + z + 1 > *c)
      *c *= 2;
    *b = (char *)xrealloc(*b, *c);
  }
  memcpy(*b + *n, s, z);
  *n += z;
  (*b)[*n] = 0;
}
static void js_quote(char **b, size_t *n, size_t *c, const char *s) {
  js_append(b, n, c, "\"", 1);
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    const char *q = NULL;
    char tmp[8];
    size_t z = 1;
    switch (*p) {
    case '"':
      q = "\\\"";
      z = 2;
      break;
    case '\\':
      q = "\\\\";
      z = 2;
      break;
    case '\n':
      q = "\\n";
      z = 2;
      break;
    case '\r':
      q = "\\r";
      z = 2;
      break;
    case '\t':
      q = "\\t";
      z = 2;
      break;
    default:
      if (*p < 32) {
        snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
        q = tmp;
        z = 6;
      } else {
        tmp[0] = (char)*p;
        q = tmp;
      }
    }
    js_append(b, n, c, q, z);
  }
  js_append(b, n, c, "\"", 1);
}
static void js_emit(Value v, char **b, size_t *n, size_t *c, int d) {
  if (d > 100) {
    js_append(b, n, c, "null", 4);
    return;
  }
  if (v.type == V_NULL)
    js_append(b, n, c, "null", 4);
  else if (v.type == V_BOOL) {
    const char *q = v.as.boolean ? "true" : "false";
    js_append(b, n, c, q, strlen(q));
  } else if (is_numeric(v)) {
    char *q = value_to_string(v);
    js_append(b, n, c, q, strlen(q));
    free(q);
  } else if (v.type == V_STRING)
    js_quote(b, n, c, v.as.string);
  else if (v.type == V_ARRAY) {
    js_append(b, n, c, "[", 1);
    for (int i = 0; i < v.as.array->count; i++) {
      if (i)
        js_append(b, n, c, ",", 1);
      js_emit(v.as.array->items[i], b, n, c, d + 1);
    }
    js_append(b, n, c, "]", 1);
  } else if (v.type == V_MAP) {
    js_append(b, n, c, "{", 1);
    for (int i = 0; i < v.as.map->count; i++) {
      if (i)
        js_append(b, n, c, ",", 1);
      char *q = value_to_string(v.as.map->items[i].key);
      js_quote(b, n, c, q);
      free(q);
      js_append(b, n, c, ":", 1);
      js_emit(v.as.map->items[i].value, b, n, c, d + 1);
    }
    js_append(b, n, c, "}", 1);
  } else
    js_append(b, n, c, "null", 4);
}
static Value native_json_stringify(int argc, Value *a, int line) {
  if (argc != 1) {
    runtime_error(line, "json_stringify expects one value.");
    return v_null();
  }
  size_t cap = 128, n = 0;
  char *b = (char *)calloc(cap, 1);
  if (!b)
    exit(2);
  js_emit(a[0], &b, &n, &cap, 0);
  Value v = v_string(b);
  free(b);
  return v;
}
typedef struct {
  const char *p;
  int line, depth;
} JP;
static void jp_ws(JP *p) {
  while (isspace((unsigned char)*p->p))
    p->p++;
}
static char *jp_str(JP *p) {
  if (*p->p != '"')
    return NULL;
  p->p++;
  size_t c = 32, n = 0;
  char *s = (char *)malloc(c);
  if (!s)
    exit(2);
  while (*p->p && *p->p != '"') {
    unsigned char ch = (unsigned char)*p->p++;
    if (ch == '\\') {
      ch = (unsigned char)*p->p++;
      switch (ch) {
      case '"':
      case '\\':
      case '/':
        break;
      case 'n':
        ch = '\n';
        break;
      case 'r':
        ch = '\r';
        break;
      case 't':
        ch = '\t';
        break;
      case 'b':
        ch = '\b';
        break;
      case 'f':
        ch = '\f';
        break;
      case 'u': {
        unsigned u = 0;
        for (int j = 0; j < 4; j++) {
          char h = *p->p++;
          if (!isxdigit((unsigned char)h)) {
            free(s);
            return NULL;
          }
          u = u * 16u + (unsigned)(isdigit((unsigned char)h)
                                       ? h - '0'
                                       : tolower((unsigned char)h) - 'a' + 10);
        }
        if (u > 127) {
          free(s);
          return NULL;
        }
        ch = (unsigned char)u;
        break;
      }
      default:
        free(s);
        return NULL;
      }
    }
    if (n + 2 > c) {
      c *= 2;
      s = (char *)xrealloc(s, c);
    }
    s[n++] = (char)ch;
  }
  if (*p->p != '"') {
    free(s);
    return NULL;
  }
  p->p++;
  s[n] = 0;
  return s;
}
static Value jp_value(JP *p) {
  jp_ws(p);
  if (++p->depth > 100) {
    runtime_error(p->line, "JSON nesting too deep.");
    p->depth--;
    return v_null();
  }
  Value v = v_null();
  if (*p->p == '"') {
    char *s = jp_str(p);
    if (!s)
      runtime_error(p->line, "Invalid JSON string.");
    else {
      v = v_string(s);
      free(s);
    }
  } else if (*p->p == '[') {
    p->p++;
    v = v_array();
    jp_ws(p);
    while (*p->p && *p->p != ']') {
      Value x = jp_value(p);
      Array *r = v.as.array;
      if (r->count == r->capacity) {
        r->capacity = r->capacity ? r->capacity * 2 : 8;
        r->items =
            (Value *)xrealloc(r->items, (size_t)r->capacity * sizeof(Value));
      }
      r->items[r->count++] = copy_value(x);
      free_value(x);
      jp_ws(p);
      if (*p->p == ',') {
        p->p++;
        continue;
      }
      break;
    }
    if (*p->p == ']')
      p->p++;
    else
      runtime_error(p->line, "Expected ']' in JSON.");
  } else if (*p->p == '{') {
    p->p++;
    v = v_map();
    jp_ws(p);
    while (*p->p && *p->p != '}') {
      char *s = jp_str(p);
      if (!s) {
        runtime_error(p->line, "Expected JSON key.");
        break;
      }
      jp_ws(p);
      if (*p->p != ':') {
        free(s);
        runtime_error(p->line, "Expected ':' in JSON.");
        break;
      }
      p->p++;
      Value k = v_string(s), x = jp_value(p);
      map_set(v.as.map, k, x);
      free_value(k);
      free_value(x);
      free(s);
      jp_ws(p);
      if (*p->p == ',') {
        p->p++;
        jp_ws(p);
        continue;
      }
      break;
    }
    if (*p->p == '}')
      p->p++;
    else
      runtime_error(p->line, "Expected '}' in JSON.");
  } else if (!strncmp(p->p, "true", 4)) {
    p->p += 4;
    v = v_bool(true);
  } else if (!strncmp(p->p, "false", 5)) {
    p->p += 5;
    v = v_bool(false);
  } else if (!strncmp(p->p, "null", 4)) {
    p->p += 4;
    v = v_null();
  } else {
    char *e = NULL;
    double d = strtod(p->p, &e);
    if (e == p->p || !isfinite(d))
      runtime_error(p->line, "Invalid JSON value.");
    else {
      v = v_num(d);
      p->p = e;
    }
  }
  p->depth--;
  return v;
}
static Value native_json_parse(int argc, Value *a, int line) {
  if (argc != 1 || a[0].type != V_STRING) {
    runtime_error(line, "json_parse expects a JSON string.");
    return v_null();
  }
  JP p = {a[0].as.string, line, 0};
  Value v = jp_value(&p);
  jp_ws(&p);
  if (*p.p)
    runtime_error(line, "Trailing characters after JSON value.");
  return v;
}
static Value native_datetime_iso(int argc, Value *a, int line) {
  if (argc > 1) {
    runtime_error(line, "datetime_iso expects zero or one Unix timestamp.");
    return v_null();
  }
  time_t t = argc ? (time_t)numeric_double(a[0]) : time(NULL);
  struct tm z;
#ifdef _WIN32
  if (gmtime_s(&z, &t) != 0)
    return v_null();
#else
  if (!gmtime_r(&t, &z))
    return v_null();
#endif
  char b[40];
  strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%SZ", &z);
  return v_string(b);
}
static Value native_datetime_parts(int argc, Value *a, int line) {
  if (argc > 1) {
    runtime_error(line, "datetime_parts expects zero or one timestamp.");
    return v_null();
  }
  time_t t = argc ? (time_t)numeric_double(a[0]) : time(NULL);
  struct tm z;
#ifdef _WIN32
  if (gmtime_s(&z, &t) != 0)
    return v_null();
#else
  if (!gmtime_r(&t, &z))
    return v_null();
#endif
  Value m = v_map();
  const char *keys[] = {"year",   "month",  "day",     "hour",
                        "minute", "second", "weekday", "yearday"};
  int vals[] = {z.tm_year + 1900, z.tm_mon + 1, z.tm_mday, z.tm_hour,
                z.tm_min,         z.tm_sec,     z.tm_wday, z.tm_yday + 1};
  for (int i = 0; i < 8; i++) {
    Value k = v_string(keys[i]), x = v_num(vals[i]);
    map_set(m.as.map, k, x);
    free_value(k);
    free_value(x);
  }
  return m;
}
static char *std_trim(char *s) {
  while (isspace((unsigned char)*s))
    s++;
  size_t n = strlen(s);
  while (n && isspace((unsigned char)s[n - 1]))
    s[--n] = 0;
  return s;
}
static Value std_scalar(const char *s, int line) {
  char *q = copy_s(s);
  char *t = std_trim(q);
  Value v = v_null();
  size_t n = strlen(t);
  if (!strcmp(t, "true"))
    v = v_bool(true);
  else if (!strcmp(t, "false"))
    v = v_bool(false);
  else if (!strcmp(t, "null") || !strcmp(t, "~"))
    v = v_null();
  else if (n >= 2 && ((t[0] == '"' && t[n - 1] == '"') ||
                      (t[0] == '\'' && t[n - 1] == '\''))) {
    t[n - 1] = 0;
    char *out = (char *)malloc(n + 1);
    if (!out)
      exit(2);
    size_t j = 0;
    for (size_t i = 1; t[i]; i++) {
      if (t[i] == '\\' && t[i + 1] &&
          (t[i + 1] == 'n' || t[i + 1] == 'r' || t[i + 1] == 't' ||
           t[i + 1] == '\\' || t[i + 1] == '"')) {
        i++;
        out[j++] = t[i] == 'n'   ? '\n'
                   : t[i] == 'r' ? '\r'
                   : t[i] == 't' ? '\t'
                                 : t[i];
      } else if (t[i] == '\'' && t[i + 1] == '\'' && t[0] == '\'') {
        out[j++] = '\'';
        i++;
      } else
        out[j++] = t[i];
    }
    out[j] = 0;
    v = v_string(out);
    free(out);
  } else {
    char *e = NULL;
    errno = 0;
    double d = strtod(t, &e);
    if (*t && e && *e == 0 && errno != ERANGE && isfinite(d))
      v = v_num(d);
    else
      v = v_string(t);
  }
  free(q);
  (void)line;
  return v;
}
static Value native_parse_flat(int argc, Value *a, int line, bool yaml) {
  if (argc != 1 || a[0].type != V_STRING) {
    runtime_error(line, "flat data parser expects one string.");
    return v_null();
  }
  char *src = copy_s(a[0].as.string);
  Value out = v_map();
  char *cursor = src;
  while (cursor && *cursor) {
    char *ln = cursor;
    char *nl = strchr(cursor, '\n');
    if (nl) {
      *nl = 0;
      cursor = nl + 1;
    } else
      cursor = NULL;
    char *t = std_trim(ln);
    if (!*t || *t == '#')
      continue;
    bool quote = false;
    char qc = 0;
    char *sep = NULL;
    for (char *q = t; *q; q++) {
      if (quote) {
        if (*q == qc && (q == t || q[-1] != '\\'))
          quote = false;
      } else if (*q == '\'' || *q == '"') {
        quote = true;
        qc = *q;
      } else if ((yaml && *q == ':') || (!yaml && *q == '=')) {
        sep = q;
        break;
      }
    }
    if (!sep) {
      free(src);
      runtime_error(line, "Invalid flat %s line; expected key %s value.",
                    yaml ? "YAML" : "TOML", yaml ? ":" : "=");
      return v_null();
    }
    *sep = 0;
    char *k = std_trim(t), *val = std_trim(sep + 1);
    char *comment = NULL;
    quote = false;
    qc = 0;
    for (char *q = val; *q; q++) {
      if (quote) {
        if (*q == qc && (q == val || q[-1] != '\\'))
          quote = false;
      } else if (*q == '\'' || *q == '"') {
        quote = true;
        qc = *q;
      } else if (*q == '#') {
        comment = q;
        break;
      }
    }
    if (comment) {
      *comment = 0;
      val = std_trim(val);
    }
    Value key = v_string(k), value = std_scalar(val, line);
    map_set(out.as.map, key, value);
    free_value(key);
    free_value(value);
  }
  free(src);
  return out;
}
static Value native_toml_parse(int argc, Value *a, int line) {
  return native_parse_flat(argc, a, line, false);
}
static Value native_yaml_parse(int argc, Value *a, int line) {
  return native_parse_flat(argc, a, line, true);
}
static Value native_cli_args(int argc, Value *a, int line) {
  (void)a;
  if (argc) {
    runtime_error(line, "std_cli_args expects no arguments.");
    return v_null();
  }
  Binding *b = find_binding(global_env, "args");
  if (b && b->value.type == V_ARRAY)
    return copy_value(b->value);
  Value v = v_array();
  return v;
}
static Value native_csv_parse(int argc, Value *a, int line) {
  if (argc != 1 || a[0].type != V_STRING) {
    runtime_error(line, "std_csv_parse expects one CSV string.");
    return v_null();
  }
  const char *s = a[0].as.string;
  Value rows = v_array(), row = v_array();
  size_t cap = 64, n = 0;
  char *field = (char *)malloc(cap);
  if (!field)
    exit(2);
  bool quoted = false, at_start = true;
  for (size_t i = 0;; i++) {
    char c = s[i];
    if (quoted) {
      if (c == '\0') {
        free(field);
        runtime_error(line, "Unterminated quoted CSV field.");
        return v_null();
      }
      if (c == '"') {
        if (s[i + 1] == '"') {
          if (n + 2 > cap) {
            cap *= 2;
            field = (char *)xrealloc(field, cap);
          }
          field[n++] = '"';
          i++;
        } else
          quoted = false;
      } else {
        if (n + 2 > cap) {
          cap *= 2;
          field = (char *)xrealloc(field, cap);
        }
        field[n++] = c;
      }
      continue;
    }
    if (c == '"' && at_start) {
      quoted = true;
      at_start = false;
      continue;
    }
    if (c == ',' || c == '\n' || c == '\r' || c == '\0') {
      if (c == '\0' && n == 0 && at_start && row.as.array->count == 0 &&
          rows.as.array->count > 0)
        break;
      field[n] = 0;
      Value fv = v_string(field);
      Array *ra = row.as.array;
      if (ra->count == ra->capacity) {
        ra->capacity = ra->capacity ? ra->capacity * 2 : 8;
        ra->items =
            (Value *)xrealloc(ra->items, (size_t)ra->capacity * sizeof(Value));
      }
      ra->items[ra->count++] = fv;
      n = 0;
      at_start = true;
      if (c == ',')
        continue;
      if (c == '\r' && s[i + 1] == '\n')
        i++;
      if (row.as.array->count > 0) {
        Array *rr = rows.as.array;
        if (rr->count == rr->capacity) {
          rr->capacity = rr->capacity ? rr->capacity * 2 : 8;
          rr->items = (Value *)xrealloc(rr->items,
                                        (size_t)rr->capacity * sizeof(Value));
        }
        rr->items[rr->count++] = copy_value(row);
        free_value(row);
        row = v_array();
      }
      if (c == '\0')
        break;
      continue;
    }
    if (n + 2 > cap) {
      cap *= 2;
      field = (char *)xrealloc(field, cap);
    }
    field[n++] = c;
    at_start = false;
  }
  free(field);
  free_value(row);
  return rows;
}

/* Optional file-format conversion backend.  The interpreter remains
 * dependency-free; conversion uses installed command-line tools (ImageMagick,
 * FFmpeg, LibreOffice, or Pandoc). Paths are quoted before passing them to the
 * platform command shell. */
static char *format_ext(const char *path) {
  const char *base = path, *p;
  for (p = path; *p; p++)
    if (*p == '/' || *p == '\\')
      base = p + 1;
  const char *dot = strrchr(base, '.');
  if (!dot || dot == base || !dot[1])
    return copy_s("");
  size_t n = strlen(dot + 1);
  char *out = (char *)malloc(n + 1);
  if (!out)
    exit(2);
  for (size_t i = 0; i < n; i++)
    out[i] = (char)tolower((unsigned char)dot[1 + i]);
  out[n] = 0;
  return out;
}
static char *format_quote(const char *s) {
#ifdef _WIN32
  size_t n = strlen(s), cap = n * 2 + 3, j = 0;
  char *q = (char *)malloc(cap);
  if (!q)
    exit(2);
  q[j++] = '"';
  for (size_t i = 0; i < n; i++) {
    if (s[i] == '"' || s[i] == '\\')
      q[j++] = '\\';
    q[j++] = s[i];
  }
  q[j++] = '"';
  q[j] = 0;
  return q;
#else
  size_t n = strlen(s), cap = n * 4 + 3, j = 0;
  char *q = (char *)malloc(cap);
  if (!q)
    exit(2);
  q[j++] = '\'';
  for (size_t i = 0; i < n; i++) {
    if (s[i] == '\'') {
      q[j++] = '\'';
      q[j++] = '\\';
      q[j++] = '\'';
      q[j++] = '\'';
    } else
      q[j++] = s[i];
  }
  q[j++] = '\'';
  q[j] = 0;
  return q;
#endif
}
static bool format_has_ext(const char *ext, const char *list) {
  size_t n = strlen(ext);
  const char *p = list;
  while (*p) {
    const char *e = strchr(p, ',');
    size_t m = e ? (size_t)(e - p) : strlen(p);
    if (m == n && memcmp(p, ext, n) == 0)
      return true;
    if (!e)
      break;
    p = e + 1;
  }
  return false;
}
static int format_run(const char *cmd) { return system(cmd); }
static Value native_format_convert(int argc, Value *a, int line) {
  if (argc != 2 || a[0].type != V_STRING || a[1].type != V_STRING) {
    runtime_error(line,
                  "format_convert(input_path, output_path) expects two paths.");
    return v_null();
  }
  const char *src = a[0].as.string, *dst = a[1].as.string;
  struct stat st;
  if (stat(src, &st) != 0 || !S_ISREG(st.st_mode)) {
    runtime_error(
        line, "format_convert input is not an existing regular file: %s", src);
    return v_null();
  }
  char *se = format_ext(src), *de = format_ext(dst), *qs = format_quote(src),
       *qd = format_quote(dst);
  if (!*se || !*de) {
    free(se);
    free(de);
    free(qs);
    free(qd);
    runtime_error(line,
                  "format_convert requires input and output file extensions.");
    return v_null();
  }
  if (strcmp(se, de) == 0) {
    free(se);
    free(de);
    free(qs);
    free(qd);
    runtime_error(line, "Input and output extensions are identical; choose a "
                        "different output format.");
    return v_null();
  }
  int rc = -1;
  char cmd[16384];
#ifdef _WIN32
  const char *quiet = " >NUL 2>&1";
#else
  const char *quiet = " >/dev/null 2>&1";
#endif
  bool image =
      format_has_ext(
          se, "png,jpg,jpeg,bmp,gif,tif,tiff,webp,ico,ppm,pgm,pbm,avif,heic") &&
      format_has_ext(
          de, "png,jpg,jpeg,bmp,gif,tif,tiff,webp,ico,ppm,pgm,pbm,avif,heic");
  bool media =
      format_has_ext(se, "mp4,mov,mkv,avi,webm,mp3,wav,flac,aac,ogg,m4a") ||
      format_has_ext(de, "mp4,mov,mkv,avi,webm,mp3,wav,flac,aac,ogg,m4a");
  bool office =
      format_has_ext(
          se, "pdf,doc,docx,odt,rtf,txt,html,htm,ppt,pptx,xls,xlsx,ods,odp") &&
      format_has_ext(
          de, "pdf,doc,docx,odt,rtf,txt,html,htm,ppt,pptx,xls,xlsx,ods,odp");
  bool textfmt =
      format_has_ext(se, "md,markdown,txt,html,htm,rtf,docx,odt,epub") &&
      format_has_ext(de, "md,markdown,txt,html,htm,rtf,docx,odt,epub");
  if (image) {
    snprintf(cmd, sizeof(cmd), "magick %s %s%s", qs, qd, quiet);
    rc = format_run(cmd);
    if (rc != 0) {
      snprintf(cmd, sizeof(cmd), "convert %s %s%s", qs, qd, quiet);
      rc = format_run(cmd);
    }
    if (rc != 0) {
      snprintf(cmd, sizeof(cmd), "ffmpeg -y -i %s %s%s", qs, qd, quiet);
      rc = format_run(cmd);
    }
  }
  if (rc != 0 && media) {
    snprintf(cmd, sizeof(cmd), "ffmpeg -y -i %s %s%s", qs, qd, quiet);
    rc = format_run(cmd);
  }
  if (rc != 0 && office) {
    /* LibreOffice writes output using the input basename into the destination
     * directory. */
    char *dir = NULL, *base = NULL;
    const char *slash1 = strrchr(dst, '/'), *slash2 = strrchr(dst, '\\'),
               *slash = slash1;
    if (!slash || (slash2 && slash2 > slash))
      slash = slash2;
    if (slash) {
      dir = copy_n(dst, (size_t)(slash - dst));
      base = copy_s(slash + 1);
    } else {
      dir = copy_s(".");
      base = copy_s(dst);
    }
    char *qdir = format_quote(dir);
    char *expected_base = copy_s(src);
    char *last1 = strrchr(expected_base, '/'),
         *last2 = strrchr(expected_base, '\\'), *last = last1;
    if (!last || (last2 && last2 > last))
      last = last2;
    if (last)
      memmove(expected_base, last + 1, strlen(last + 1) + 1);
    char *dot = strrchr(expected_base, '.');
    if (dot)
      *dot = 0;
    size_t need = strlen(dir) + strlen(expected_base) + strlen(de) + 4;
    char *expected = (char *)malloc(need);
    if (!expected)
      exit(2);
    snprintf(expected, need, "%s/%s.%s", dir, expected_base, de);
    char *qexpected = format_quote(expected);
    snprintf(cmd, sizeof(cmd),
             "libreoffice --headless --convert-to %s --outdir %s %s%s", de,
             qdir, qs, quiet);
    rc = format_run(cmd);
    if (rc != 0) {
      snprintf(cmd, sizeof(cmd),
               "soffice --headless --convert-to %s --outdir %s %s%s", de, qdir,
               qs, quiet);
      rc = format_run(cmd);
    }
    if (rc == 0 && strcmp(expected, dst) != 0) {
      remove(dst);
      if (rename(expected, dst) != 0)
        rc = -1;
    }
    free(dir);
    free(base);
    free(qdir);
    free(expected_base);
    free(expected);
    free(qexpected);
  }
  if (rc != 0 && textfmt) {
    snprintf(cmd, sizeof(cmd), "pandoc %s -o %s%s", qs, qd, quiet);
    rc = format_run(cmd);
  }
  free(se);
  free(de);
  free(qs);
  free(qd);
  if (rc != 0) {
    runtime_error(line,
                  "Could not convert file. Install a suitable converter: "
                  "ImageMagick for images, FFmpeg for audio/video, LibreOffice "
                  "for office/PDF, or Pandoc for text/document formats.");
    return v_null();
  }
  return v_bool(true);
}

static Value native_printf(int argc, Value *a, int line) {
  if (argc < 1 || a[0].type != V_STRING) {
    runtime_error(
        line,
        "printf expects a format string followed by zero or more values.");
    return v_null();
  }
  const char *fmt = a[0].as.string;
  size_t cap = 128, used = 0;
  char *out = (char *)malloc(cap);
  if (!out)
    exit(2);
  out[0] = '\0';
#define PRINTF_APPEND(ptr_, len_)                                              \
  do {                                                                         \
    size_t add_ = (len_);                                                      \
    if (add_ > 1024u * 1024u || used > SIZE_MAX - add_ - 1) {                  \
      free(out);                                                               \
      runtime_error(line, "printf output exceeds safe memory limits.");        \
      return v_null();                                                         \
    }                                                                          \
    if (used + add_ + 1 > cap) {                                               \
      while (used + add_ + 1 > cap) {                                          \
        if (cap > SIZE_MAX / 2) {                                              \
          free(out);                                                           \
          runtime_error(line, "Integer overflow in buffer allocation.");       \
          return v_null();                                                     \
        }                                                                      \
        cap *= 2;                                                              \
      }                                                                        \
      out = (char *)xrealloc(out, cap);                                        \
    }                                                                          \
    memcpy(out + used, (ptr_), add_);                                          \
    used += add_;                                                              \
    out[used] = '\0';                                                          \
  } while (0)
  int argi = 1;
  for (size_t i = 0; fmt[i];) {
    if (fmt[i] != '%') {
      PRINTF_APPEND(fmt + i, 1);
      i++;
      continue;
    }
    if (fmt[i + 1] == '%') {
      PRINTF_APPEND("%", 1);
      i += 2;
      continue;
    }
    size_t start = i++;
    while (fmt[i] && strchr("-+ #0", fmt[i]))
      i++;
    while (isdigit((unsigned char)fmt[i]))
      i++;
    if (fmt[i] == '.') {
      i++;
      while (isdigit((unsigned char)fmt[i]))
        i++;
    }
    /* Length modifiers are accepted but values are formatted using safe host
     * types. */
    while (fmt[i] && strchr("hlLzjt", fmt[i]))
      i++;
    char spec = fmt[i];
    if (!spec) {
      free(out);
      runtime_error(line,
                    "printf format ends with an incomplete '%%' placeholder.");
      return v_null();
    }
    i++;
    if (argi >= argc) {
      free(out);
      runtime_error(line, "printf has more format placeholders than values.");
      return v_null();
    }
    char format[64];
    size_t flen = i - start;
    if (flen >= sizeof(format)) {
      free(out);
      runtime_error(line, "printf format placeholder is too long.");
      return v_null();
    }
    memcpy(format, fmt + start, flen);
    format[flen] = '\0';
    Value v = a[argi++];
    char temp[4096];
    int n = -1;
    if (strchr("diuoxX", spec)) {
      long long iv = 0;
      if (is_numeric(v)) {
        double d = numeric_double(v);
        if (!isfinite(d) || d < (double)LLONG_MIN || d >= -(double)LLONG_MIN) {
          free(out);
          runtime_error(line, "printf integer placeholder needs a finite "
                              "integer-sized number.");
          return v_null();
        }
        iv = (long long)d;
      } else if (v.type == V_BOOL)
        iv = v.as.boolean ? 1 : 0;
      else {
        free(out);
        runtime_error(
            line, "printf %%%c placeholder needs a number; received %s.", spec,
            (v.type == V_STRING ? "string"
             : is_numeric(v)    ? "number"
             : v.type == V_BOOL ? "boolean"
                                : "value"));
        return v_null();
      }
      /* Remove length modifiers: our argument is always long long. */
      char safe_fmt[64];
      size_t j = 0;
      for (size_t k = 0; format[k] && j + 2 < sizeof(safe_fmt); k++) {
        if (strchr("hlLzjt", format[k]))
          continue;
        safe_fmt[j++] = format[k];
      }
      safe_fmt[j] = '\0';
      char adjusted[64];
      snprintf(adjusted, sizeof(adjusted), "%sll%c", "", spec);
      (void)adjusted;
      /* Rebuild flags/width/precision with a portable ll modifier. */
      char f2[64];
      size_t p = 0;
      f2[p++] = '%';
      for (size_t k = 1; format[k] && strchr("-+ #0", format[k]); k++)
        f2[p++] = format[k];
      size_t k = 1;
      while (format[k] && strchr("-+ #0", format[k]))
        k++;
      while (format[k] && isdigit((unsigned char)format[k]))
        f2[p++] = format[k++];
      if (format[k] == '.') {
        f2[p++] = format[k++];
        while (format[k] && isdigit((unsigned char)format[k]))
          f2[p++] = format[k++];
      }
      f2[p++] = 'l';
      f2[p++] = 'l';
      f2[p++] = spec;
      f2[p] = '\0';
      if (strchr("uoxX", spec))
        n = snprintf(temp, sizeof(temp), f2, (unsigned long long)iv);
      else
        n = snprintf(temp, sizeof(temp), f2, iv);
    } else if (strchr("fFeEgGaA", spec)) {
      double d = is_numeric(v)
                     ? numeric_double(v)
                     : (v.type == V_BOOL ? (v.as.boolean ? 1.0 : 0.0) : NAN);
      if (!isfinite(d) && v.type != V_NUMBER && !is_numeric(v) &&
          v.type != V_BOOL) {
        free(out);
        runtime_error(
            line, "printf %%%c placeholder needs a number; received %s.", spec,
            (v.type == V_STRING ? "string"
             : is_numeric(v)    ? "number"
             : v.type == V_BOOL ? "boolean"
                                : "value"));
        return v_null();
      }
      char f2[64];
      size_t p = 0;
      f2[p++] = '%';
      for (size_t k = 1; format[k] && strchr("-+ #0", format[k]); k++)
        f2[p++] = format[k];
      size_t k = 1;
      while (format[k] && strchr("-+ #0", format[k]))
        k++;
      while (format[k] && isdigit((unsigned char)format[k]))
        f2[p++] = format[k++];
      if (format[k] == '.') {
        f2[p++] = format[k++];
        while (format[k] && isdigit((unsigned char)format[k]))
          f2[p++] = format[k++];
      }
      f2[p++] = spec;
      f2[p] = '\0';
      n = snprintf(temp, sizeof(temp), f2, d);
    } else if (spec == 's') {
      char *str = value_to_string(v);
      char f2[64];
      size_t p = 0;
      f2[p++] = '%';
      for (size_t k = 1; format[k] && strchr("-+ #0", format[k]); k++)
        f2[p++] = format[k];
      size_t k = 1;
      while (format[k] && strchr("-+ #0", format[k]))
        k++;
      while (format[k] && isdigit((unsigned char)format[k]))
        f2[p++] = format[k++];
      if (format[k] == '.') {
        f2[p++] = format[k++];
        while (format[k] && isdigit((unsigned char)format[k]))
          f2[p++] = format[k++];
      }
      f2[p++] = 's';
      f2[p] = '\0';
      n = snprintf(temp, sizeof(temp), f2, str);
      free(str);
    } else if (spec == 'c') {
      uint32_t cp = 0;
      if (v.type == V_STRING) {
        size_t used = 0;
        if (!utf8_decode_one((const unsigned char *)v.as.string, &used, &cp)) {
          free(out);
          runtime_error(line,
                        "printf %%c needs a non-empty valid UTF-8 character.");
          return v_null();
        }
      } else if (v.type == V_BOOL)
        cp = v.as.boolean ? 1u : 0u;
      else if (is_numeric(v)) {
        double d = numeric_double(v);
        if (!isfinite(d) || floor(d) != d || d < 0 || d > 0x10FFFF) {
          free(out);
          runtime_error(
              line,
              "printf %%c code point must be an integer from 0 to 1114111.");
          return v_null();
        }
        cp = (uint32_t)d;
      } else {
        free(out);
        runtime_error(
            line,
            "printf %%c placeholder needs a character or Unicode code point.");
        return v_null();
      }
      size_t encoded = utf8_encode_codepoint(cp, temp);
      if (!encoded) {
        free(out);
        runtime_error(line,
                      "printf %%c received an invalid Unicode code point.");
        return v_null();
      }
      n = (int)encoded;
    } else {
      free(out);
      runtime_error(
          line,
          "printf does not support format placeholder '%%%c'. Supported: %%s "
          "%%d %%i %%u %%x %%X %%o %%f %%e %%g %%c and %%%%.",
          spec);
      return v_null();
    }
    if (n < 0 || (size_t)n >= sizeof(temp)) {
      free(out);
      runtime_error(line, "printf formatted value is too long.");
      return v_null();
    }
    PRINTF_APPEND(temp, (size_t)n);
  }
  if (argi < argc) { /* Extra values are harmless, like common printf
                        implementations. */
  }
  if (used)
    fwrite(out, 1, used, stdout);
  fflush(stdout);
  free(out);
#undef PRINTF_APPEND
  return v_num((double)used);
}

static Value native_call(const char *name, int argc, Value *a, int line) {
  if (strcmp(name, "std_format_convert") == 0)
    return native_format_convert(argc, a, line);
  if (strcmp(name, "std_uuid_v4") == 0)
    return native_uuid_v4(argc, a, line);
  if (strcmp(name, "std_uuid_v7") == 0)
    return native_uuid_v7(argc, a, line);
  if (strcmp(name, "std_sha256") == 0)
    return native_sha256(argc, a, line);
  if (strcmp(name, "std_json_stringify") == 0)
    return native_json_stringify(argc, a, line);
  if (strcmp(name, "std_json_parse") == 0)
    return native_json_parse(argc, a, line);
  if (strcmp(name, "std_datetime_iso") == 0)
    return native_datetime_iso(argc, a, line);
  if (strcmp(name, "std_datetime_parts") == 0)
    return native_datetime_parts(argc, a, line);
  if (strcmp(name, "std_cli_args") == 0)
    return native_cli_args(argc, a, line);
  if (strcmp(name, "std_csv_parse") == 0)
    return native_csv_parse(argc, a, line);
  if (strcmp(name, "std_toml_parse") == 0)
    return native_toml_parse(argc, a, line);
  if (strcmp(name, "std_yaml_parse") == 0)
    return native_yaml_parse(argc, a, line);
  if (strncmp(name, "os_", 3) == 0 && strcmp(name, "os_os") != 0 &&
      strcmp(name, "os_system") != 0 && strcmp(name, "os_shellexec") != 0)
    return native_os(name, argc, a, line);
  /* Explicit-width endian helpers operate on numeric byte arrays. Reads and
   * writes validate the complete range before touching memory. */
  {
    const char *width_names[] = {"read_u16_le",
                                 "read_u16_be",
                                 "read_u32_le",
                                 "read_u32_be",
                                 "read_u64_le",
                                 "read_u64_be",
                                 "write_u16_le",
                                 "write_u16_be",
                                 "write_u32_le",
                                 "write_u32_be",
                                 "write_u64_le",
                                 "write_u64_be",
                                 "mouse_move",
                                 "mouse_left_click",
                                 "mouse_right_click",
                                 "mouse_middle_click",
                                 "mouse_left_click_at",
                                 "mouse_right_click_at",
                                 "mouse_position",
                                 "mouse_scroll",
                                 "mouse_smooth_move",
                                 "mouse_set_position",
                                 "mouse_double_click",
                                 "mouse_press",
                                 "mouse_release",
                                 "mouse_is_button_down",
                                 "mouse_screen_size",
                                 "mouse_show_cursor",
                                 "mouse_set_system_cursor",
                                 "mouse_restore_system_cursor",
                                 "mouse_is_available",
                                 "mouse_wait_button",
                                 "keyboard_wait_key",
                                 "keyboard_is_key_down",
                                 "keyboard_type_text",
                                 "keyboard_press",
                                 "keyboard_key_down",
                                 "keyboard_key_up",
                                 "std_format_convert",
                                 "std_uuid_v4",
                                 "std_uuid_v7",
                                 "std_sha256",
                                 "std_json_stringify",
                                 "std_json_parse",
                                 "std_datetime_iso",
                                 "std_datetime_parts",
                                 "std_cli_args",
                                 "std_csv_parse",
                                 "std_toml_parse",
                                 "std_yaml_parse"};
    int which = -1;
    for (int wi = 0; wi < 12; wi++)
      if (strcmp(name, width_names[wi]) == 0) {
        which = wi;
        break;
      }
    if (which >= 0) {
      bool writing = which >= 6;
      int slot = which % 6;
      unsigned bytes = (slot < 2) ? 2u : (slot < 4) ? 4u : 8u;
      bool big_endian = (slot % 2) == 1;
      long offset = 0;
      if (argc != (writing ? 3 : 2) || a[0].type != V_ARRAY ||
          !numeric_to_long(a[1], &offset)) {
        runtime_error(line, "%s expects (byte_array, offset%s).", name,
                      writing ? ", value" : "");
        return v_null();
      }
      Array *ar = a[0].as.array;
      if (offset < 0 || (uint64_t)offset > (uint64_t)ar->count ||
          bytes > (uint64_t)ar->count - (uint64_t)offset) {
        runtime_error(line, "%s byte range is outside the array.", name);
        return v_null();
      }
      if (writing) {
        uint64_t value = 0;
        if (!numeric_to_u64(a[2], &value) ||
            (bytes < 8 && value >= ((uint64_t)1 << (bytes * 8)))) {
          runtime_error(
              line,
              "%s value must be an unsigned integer that fits in %u bits.",
              name, bytes * 8);
          return v_null();
        }
        for (unsigned j = 0; j < bytes; j++) {
          unsigned shift = (big_endian ? (bytes - 1u - j) : j) * 8u;
          unsigned byte = (unsigned)((value >> shift) & UINT64_C(255));
          char tmp[4];
          snprintf(tmp, sizeof(tmp), "%u", byte);
          Value v = v_bigint(tmp);
          free_value(ar->items[offset + (long)j]);
          ar->items[offset + (long)j] = v;
        }
        return v_bool(true);
      }
      uint64_t value = 0;
      for (unsigned j = 0; j < bytes; j++) {
        long at = offset + (long)j;
        uint32_t byte = 0;
        if (!numeric_to_u32(ar->items[at], &byte) || byte > 255) {
          runtime_error(line,
                        "%s encountered a non-byte value at array index %ld.",
                        name, at);
          return v_null();
        }
        unsigned shift = (big_endian ? (bytes - 1u - j) : j) * 8u;
        value |= ((uint64_t)byte) << shift;
      }
      char tmp[32];
      snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)value);
      return v_bigint(tmp);
    }
  }
  if (strcmp(name, "bit_and") == 0 || strcmp(name, "bit_or") == 0 ||
      strcmp(name, "bit_xor") == 0 || strcmp(name, "bit_not") == 0 ||
      strcmp(name, "bit_shl") == 0 || strcmp(name, "bit_shr") == 0) {
    int unary = strcmp(name, "bit_not") == 0;
    if (argc != (unary ? 1 : 2)) {
      runtime_error(line, "%s expects %d argument(s).", name, unary ? 1 : 2);
      return v_null();
    }
    uint32_t values[2] = {0, 0};
    for (int i = 0; i < argc; i++)
      if (!numeric_to_u32(a[i], &values[i])) {
        runtime_error(line, "%s arguments must be unsigned 32-bit integers.",
                      name);
        return v_null();
      }
    uint32_t x = values[0], y = unary ? 0u : values[1], z = 0;
    if (strcmp(name, "bit_and") == 0)
      z = x & y;
    else if (strcmp(name, "bit_or") == 0)
      z = x | y;
    else if (strcmp(name, "bit_xor") == 0)
      z = x ^ y;
    else if (strcmp(name, "bit_not") == 0)
      z = ~x;
    else {
      if (y > 31) {
        runtime_error(line, "%s shift count must be from 0 to 31.", name);
        return v_null();
      }
      z = strcmp(name, "bit_shl") == 0 ? (uint32_t)(x << y)
                                       : (uint32_t)(x >> y);
    }
    char number[32];
    snprintf(number, sizeof(number), "%u", (unsigned int)z);
    return v_bigint(number);
  }
  if (strcmp(name, "byte_get") == 0 || strcmp(name, "byte_set") == 0) {
    bool set = strcmp(name, "byte_set") == 0;
    long idx = 0;
    if (argc != (set ? 3 : 2) || a[0].type != V_ARRAY ||
        !numeric_to_long(a[1], &idx)) {
      runtime_error(line, "%s expects (array, integer index%s).", name,
                    set ? ", byte value" : "");
      return v_null();
    }
    Array *ar = a[0].as.array;
    if (idx < 0)
      idx += ar->count;
    if (idx < 0 || idx >= ar->count) {
      runtime_error(line, "%s index is out of range.", name);
      return v_null();
    }
    if (set) {
      uint32_t byte = 0;
      if (!numeric_to_u32(a[2], &byte) || byte > 255) {
        runtime_error(line, "byte_set value must be an integer from 0 to 255.");
        return v_null();
      }
      char number[8];
      snprintf(number, sizeof(number), "%u", (unsigned int)byte);
      Value v = v_bigint(number);
      free_value(ar->items[idx]);
      ar->items[idx] = copy_value(v);
      return v;
    }
    uint32_t byte = 0;
    if (!numeric_to_u32(ar->items[idx], &byte) || byte > 255) {
      runtime_error(line,
                    "byte_get array element is not a byte (integer 0-255).");
      return v_null();
    }
    char number[8];
    snprintf(number, sizeof(number), "%u", (unsigned int)byte);
    return v_bigint(number);
  }
  if (strcmp(name, "os_os") == 0) {
    if (argc != 0) {
      runtime_error(line, "os_os expects no arguments.");
      return v_null();
    }
#ifdef _WIN32
    return v_string("windows");
#elif defined(__APPLE__)
    return v_string("macos");
#elif defined(__linux__)
    return v_string("linux");
#elif defined(__FreeBSD__)
    return v_string("freebsd");
#else
    return v_string("unknown");
#endif
  }
  if (strcmp(name, "os_system") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line)) {
      runtime_error(line, "os_system expects exactly one command string.");
      return v_null();
    }
    /* Deliberately synchronous and shell-mediated, matching C system(). */
    int status = system(a[0].as.string);
    return v_num((double)status);
  }
  if (strcmp(name, "os_shellexec") == 0 || strcmp(name, "shellexec") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line)) {
      runtime_error(line, "%s expects exactly one target string.", name);
      return v_null();
    }
#ifdef _WIN32
    {
      HINSTANCE r = ShellExecuteA(NULL, "open", a[0].as.string, NULL, NULL,
                                  SW_SHOWNORMAL);
      if ((INT_PTR)r <= 32) {
        runtime_error(line, "%s failed to open '%s' (ShellExecute code %ld).",
                      name, a[0].as.string, (long)(INT_PTR)r);
        return v_bool(false);
      }
      return v_bool(true);
    }
#else
    {
      pid_t pid = fork();
      if (pid < 0) {
        runtime_error(line, "%s could not start: %s", name, strerror(errno));
        return v_bool(false);
      }
      if (pid == 0) {
        pid_t worker = fork();
        if (worker < 0)
          _exit(127);
        if (worker > 0)
          _exit(0);
        /* Detached grandchild: the interpreter never waits for xdg-open. */
        (void)setsid();
        int nullfd = open("/dev/null", O_RDWR);
        if (nullfd >= 0) {
          (void)dup2(nullfd, STDIN_FILENO);
          (void)dup2(nullfd, STDOUT_FILENO);
          (void)dup2(nullfd, STDERR_FILENO);
          if (nullfd > 2)
            close(nullfd);
        }
        execlp("xdg-open", "xdg-open", a[0].as.string, (char *)NULL);
        _exit(127);
      }
      int child_status = 0;
      while (waitpid(pid, &child_status, 0) < 0 && errno == EINTR) {
      }
      if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0) {
        runtime_error(line, "%s could not start background opener.", name);
        return v_bool(false);
      }
      return v_bool(true);
    }
#endif
  }
  if (strncmp(name, "mouse_", 6) == 0 || strncmp(name, "keyboard_", 9) == 0)
    return native_input(name, argc, a, line);
  if (strcmp(name, "http_serve") == 0)
    return native_http_serve(argc, a, line);
  if (strcmp(name, "http_request") == 0)
    return native_http_request(argc, a, line);
  if (strcmp(name, "http_get") == 0)
    return native_http_get(argc, a, line);
  if (strcmp(name, "url_encode") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    const unsigned char *s = (const unsigned char *)a[0].as.string;
    size_t n = strlen((const char *)s);
    if (n > (SIZE_MAX - 1) / 3) {
      runtime_error(line, "url_encode input too large.");
      return v_null();
    }
    char *out = (char *)malloc(n * 3 + 1);
    if (!out)
      exit(2);
    static const char hex[] = "0123456789ABCDEF";
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
      unsigned char c = s[i];
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
          c == '~')
        out[j++] = (char)c;
      else if (c == ' ')
        out[j++] = '+';
      else {
        out[j++] = '%';
        out[j++] = hex[c >> 4];
        out[j++] = hex[c & 15];
      }
    }
    out[j] = '\0';
    Value v = v_string(out);
    free(out);
    return v;
  }
  if (strcmp(name, "url_decode") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    const char *s = a[0].as.string;
    size_t n = strlen(s);
    char *out = (char *)malloc(n + 1);
    if (!out)
      exit(2);
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
      if (s[i] == '+')
        out[j++] = ' ';
      else if (s[i] == '%') {
        if (i + 2 >= n || !isxdigit((unsigned char)s[i + 1]) ||
            !isxdigit((unsigned char)s[i + 2])) {
          free(out);
          runtime_error(line,
                        "url_decode encountered an invalid percent escape.");
          return v_null();
        }
        char h[3] = {s[i + 1], s[i + 2], 0};
        unsigned long decoded = strtoul(h, NULL, 16);
        if (decoded == 0) {
          free(out);
          runtime_error(line,
                        "url_decode does not support embedded NUL bytes.");
          return v_null();
        }
        out[j++] = (char)decoded;
        i += 2;
      } else
        out[j++] = s[i];
    }
    out[j] = '\0';
    Value v = v_string(out);
    free(out);
    return v;
  }
  if (strcmp(name, "path_join") == 0) {
    if (argc < 1 || argc > 8) {
      runtime_error(line, "path_join expects one to eight path strings.");
      return v_null();
    }
    char *out = copy_s("");
    for (int i = 0; i < argc; i++) {
      if (a[i].type != V_STRING) {
        free(out);
        runtime_error(line, "path_join arguments must be strings.");
        return v_null();
      }
      const char *part = a[i].as.string;
      size_t on = strlen(out), pn = strlen(part);
      if (i == 0 || part[0] == '/' || part[0] == '\\' ||
          (pn > 1 && part[1] == ':')) {
        char *nxt = copy_s(part);
        free(out);
        out = nxt;
        continue;
      }
      bool sep = on > 0 && out[on - 1] != '/' && out[on - 1] != '\\';
      if (on + pn + (sep ? 1 : 0) > 1024 * 1024) {
        free(out);
        runtime_error(line, "path_join result exceeds 1 MiB.");
        return v_null();
      }
      out = (char *)xrealloc(out, on + pn + (sep ? 2 : 1));
      if (sep)
        out[on++] = '/';
      memcpy(out + on, part, pn + 1);
    }
    Value v = v_string(out);
    free(out);
    return v;
  }
  if (strcmp(name, "basename") == 0 || strcmp(name, "dirname") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    char *copy = copy_s(a[0].as.string);
    size_t n = strlen(copy);
    while (n > 1 && (copy[n - 1] == '/' || copy[n - 1] == '\\'))
      copy[--n] = '\0';
    char *last1 = strrchr(copy, '/'), *last2 = strrchr(copy, '\\'),
         *last = last1;
    if (!last || (last2 && last2 > last))
      last = last2;
    if (strcmp(name, "basename") == 0) {
      char *out = copy_s(last ? last + 1 : copy);
      free(copy);
      Value v = v_string(out);
      free(out);
      return v;
    }
    if (!last) {
      free(copy);
      return v_string(".");
    }
    if (last == copy) {
      last[1] = '\0';
    } else
      *last = '\0';
    Value v = v_string(copy);
    free(copy);
    return v;
  }
  if (strcmp(name, "unix_time") == 0) {
    if (argc != 0) {
      runtime_error(line, "unix_time expects no arguments.");
      return v_null();
    }
    return v_num((double)time(NULL));
  }
  if (strcmp(name, "time_string") == 0) {
    if (argc > 2 || (argc >= 1 && !is_numeric(a[0]) && a[0].type != V_STRING) ||
        (argc == 2 && (!is_numeric(a[0]) || a[1].type != V_STRING))) {
      runtime_error(line, "time_string([timestamp], [format]) expects an "
                          "optional integer timestamp and format.");
      return v_null();
    }
    time_t stamp = time(NULL);
    const char *fmt = "%Y-%m-%d %H:%M:%S";
    if (argc == 1) {
      if (is_numeric(a[0])) {
        long ts = 0;
        if (!numeric_to_long(a[0], &ts)) {
          runtime_error(
              line,
              "time_string timestamp must fit the platform's time range.");
          return v_null();
        }
        stamp = (time_t)ts;
      } else
        fmt = a[0].as.string;
    } else if (argc == 2) {
      long ts = 0;
      if (!numeric_to_long(a[0], &ts)) {
        runtime_error(
            line, "time_string timestamp must fit the platform's time range.");
        return v_null();
      }
      stamp = (time_t)ts;
      fmt = a[1].as.string;
    }
    struct tm *tmv = localtime(&stamp);
    if (!tmv) {
      runtime_error(line, "time_string timestamp is out of range.");
      return v_null();
    }
    char out[1024];
    size_t n = strftime(out, sizeof(out), fmt, tmv);
    if (n == 0) {
      runtime_error(
          line, "time_string format is invalid or output exceeds 1023 bytes.");
      return v_null();
    }
    return v_string(out);
  }
  if (strcmp(name, "seed_random") == 0) {
    uint32_t seed = 0;
    if (argc != 1 || !numeric_to_u32(a[0], &seed)) {
      runtime_error(line,
                    "seed_random expects an integer from 0 to 4294967295.");
      return v_null();
    }
    srand((unsigned int)seed);
    return v_null();
  }

  if (strcmp(name, "thread_start") == 0)
    return native_thread_start(argc, a, line);
  if (strcmp(name, "thread_join") == 0)
    return native_thread_join(argc, a, line);
  if (strcmp(name, "thread_is_done") == 0)
    return native_thread_is_done(argc, a, line);
  if (strcmp(name, "thread_count") == 0)
    return native_thread_count(argc, a, line);
  if (strcmp(name, "to_string") == 0) {
    if (argc != 1) {
      runtime_error(line, "to_string expects one argument.");
      return v_null();
    }
    char *s = value_to_string(a[0]);
    Value v = v_string(s);
    free(s);
    return v;
  }
  if (strcmp(name, "to_number") == 0) {
    if (argc != 1) {
      runtime_error(line, "to_number expects one argument.");
      return v_null();
    }
    if (is_numeric(a[0]))
      return copy_value(a[0]);
    if (a[0].type == V_BOOL)
      return v_num(a[0].as.boolean ? 1 : 0);
    if (a[0].type != V_STRING) {
      runtime_error(line, "to_number expects a number, boolean, numeric "
                          "string, or single Unicode character.");
      return v_null();
    }
    const char *src = a[0].as.string;
    while (isspace((unsigned char)*src))
      src++;
    size_t slen = strlen(src);
    while (slen && isspace((unsigned char)src[slen - 1]))
      slen--;
    char *clean = copy_n(src, slen);
    uint32_t cp = 0;
    if (utf8_single_codepoint(clean, &cp)) {
      free(clean);
      return v_num((double)cp);
    }
    if (slen && valid_numeric_text(clean)) {
      Value v = numeric_literal_value(clean, line);
      free(clean);
      return v;
    }
    runtime_error(line,
                  "Cannot convert '%s' to a number: provide a numeric string "
                  "with multiple characters or exactly one Unicode character.",
                  a[0].as.string);
    free(clean);
    return v_null();
  }
  if (strcmp(name, "type_of") == 0) {
    if (argc != 1) {
      runtime_error(line, "type_of expects one argument.");
      return v_null();
    }
    return std_type_of(a[0]);
  }
  if (strcmp(name, "substring") == 0) {
    long raw_start = 0, raw_count = 0;
    if (argc < 2 || argc > 3 || !std_string_arg(name, argc, a, 0, line) ||
        !numeric_to_long_trunc(a[1], &raw_start) ||
        (argc == 3 && !numeric_to_long_trunc(a[2], &raw_count))) {
      runtime_error(line, "substring(string, start, [length]) expects indexes "
                          "that fit a machine-sized offset.");
      return v_null();
    }
    size_t n = strlen(a[0].as.string);
    if (n > (size_t)LONG_MAX) {
      runtime_error(line,
                    "substring input is too large for machine-sized indexing.");
      return v_null();
    }
    long length = (long)n;
    long start;
    if (raw_start < 0) {
      start = raw_start < -length ? 0 : length + raw_start;
    } else
      start = raw_start > length ? length : raw_start;
    long count = argc == 3 ? raw_count : length - start;
    if (count < 0)
      count = 0;
    if (count > length - start)
      count = length - start;
    char *out = (char *)malloc((size_t)count + 1);
    if (!out)
      exit(2);
    memcpy(out, a[0].as.string + start, (size_t)count);
    out[count] = '\0';
    Value v = v_string(out);
    free(out);
    return v;
  }
  if (strcmp(name, "split") == 0) {
    if (argc != 2 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line)) {
      runtime_error(line, "split expects (string, delimiter).");
      return v_null();
    }
    const char *s = a[0].as.string, *d = a[1].as.string;
    Value out = v_array();
    Array *ar = out.as.array;
    if (!*d) {
      for (size_t i = 0; s[i]; i++) {
        char c[2] = {s[i], 0};
        if (ar->count == ar->capacity) {
          ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
          ar->items = (Value *)xrealloc(ar->items,
                                        (size_t)ar->capacity * sizeof(Value));
        }
        ar->items[ar->count++] = v_string(c);
      }
      return out;
    }
    size_t dl = strlen(d);
    const char *p0 = s, *q;
    while ((q = strstr(p0, d)) != NULL) {
      char *part = (char *)malloc((size_t)(q - p0) + 1);
      if (!part)
        exit(2);
      memcpy(part, p0, (size_t)(q - p0));
      part[q - p0] = '\0';
      if (ar->count == ar->capacity) {
        ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
        ar->items =
            (Value *)xrealloc(ar->items, (size_t)ar->capacity * sizeof(Value));
      }
      ar->items[ar->count++] = v_string(part);
      free(part);
      p0 = q + dl;
    }
    if (ar->count == ar->capacity) {
      ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
      ar->items =
          (Value *)xrealloc(ar->items, (size_t)ar->capacity * sizeof(Value));
    }
    ar->items[ar->count++] = v_string(p0);
    return out;
  }
  if (strcmp(name, "join") == 0) {
    if (argc != 2 || a[0].type != V_ARRAY ||
        !std_string_arg(name, argc, a, 1, line)) {
      runtime_error(line, "join expects (array, separator). ");
      return v_null();
    }
    const char *sep = a[1].as.string;
    size_t sl = strlen(sep), need = 1;
    for (int i = 0; i < a[0].as.array->count; i++) {
      char *x = value_to_string(a[0].as.array->items[i]);
      need += strlen(x) + (i ? sl : 0);
      free(x);
      if (need > 16 * 1024 * 1024) {
        runtime_error(line, "join result exceeds 16 MiB.");
        return v_null();
      }
    }
    char *out = (char *)malloc(need);
    if (!out)
      exit(2);
    size_t off = 0;
    for (int i = 0; i < a[0].as.array->count; i++) {
      if (i) {
        memcpy(out + off, sep, sl);
        off += sl;
      }
      char *x = value_to_string(a[0].as.array->items[i]);
      size_t z = strlen(x);
      memcpy(out + off, x, z);
      off += z;
      free(x);
    }
    out[off] = '\0';
    Value v = v_string(out);
    free(out);
    return v;
  }
  if (strcmp(name, "trim") == 0 || strcmp(name, "upper") == 0 ||
      strcmp(name, "lower") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    char *out = strcmp(name, "trim") == 0 ? std_trim_copy(a[0].as.string)
                                          : copy_s(a[0].as.string);
    if (strcmp(name, "trim") != 0)
      for (char *p = out; *p; p++) {
        if (strcmp(name, "upper") == 0)
          *p = (char)toupper((unsigned char)*p);
        else
          *p = (char)tolower((unsigned char)*p);
      }
    Value v = v_string(out);
    free(out);
    return v;
  }
  if (strcmp(name, "contains") == 0 || strcmp(name, "starts_with") == 0 ||
      strcmp(name, "ends_with") == 0) {
    if (argc != 2 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line))
      return v_null();
    const char *s = a[0].as.string, *q = a[1].as.string;
    bool yes = false;
    if (strcmp(name, "contains") == 0)
      yes = strstr(s, q) != NULL;
    else if (strcmp(name, "starts_with") == 0)
      yes = strncmp(s, q, strlen(q)) == 0;
    else {
      size_t sl = strlen(s), ql = strlen(q);
      yes = ql <= sl && strcmp(s + sl - ql, q) == 0;
    }
    return v_bool(yes);
  }
  if (strcmp(name, "replace") == 0) {
    if (argc != 3 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line) ||
        !std_string_arg(name, argc, a, 2, line)) {
      runtime_error(line, "replace expects (source, old, new).");
      return v_null();
    }
    const char *s = a[0].as.string, *old = a[1].as.string,
               *rep = a[2].as.string;
    if (!*old) {
      runtime_error(line, "replace old substring cannot be empty.");
      return v_null();
    }
    size_t oldn = strlen(old), rn = strlen(rep), count = 0;
    const char *p0 = s, *q;
    while ((q = strstr(p0, old)) != NULL) {
      count++;
      p0 = q + oldn;
    }
    size_t sn = strlen(s);
    if (count && rn > oldn && (rn - oldn) > (SIZE_MAX - sn - 1) / count) {
      runtime_error(line, "replace result too large.");
      return v_null();
    }
    size_t cap = sn + count * (rn > oldn ? rn - oldn : 0) + 1;
    char *out = (char *)malloc(cap);
    if (!out)
      exit(2);
    size_t used = 0;
    p0 = s;
    while ((q = strstr(p0, old)) != NULL) {
      size_t n = (size_t)(q - p0);
      memcpy(out + used, p0, n);
      used += n;
      memcpy(out + used, rep, rn);
      used += rn;
      p0 = q + oldn;
    }
    strcpy(out + used, p0);
    Value v = v_string(out);
    free(out);
    return v;
  }
  if (strcmp(name, "ascii_array") == 0) {
    if (argc != 1 || a[0].type != V_STRING) {
      runtime_error(line, "ascii_array expects one string and returns Unicode "
                          "code points as integer values.");
      return v_null();
    }
    const unsigned char *bytes = (const unsigned char *)a[0].as.string;
    size_t len = strlen((const char *)bytes);
    if (len > (size_t)MAX_ARGS * 1024u) {
      runtime_error(line, "ascii_array input is too large.");
      return v_null();
    }
    Value out = v_array();
    Array *ar = out.as.array;
    size_t pos = 0;
    while (pos < len) {
      size_t used = 0;
      uint32_t cp = 0;
      if (!utf8_decode_one(bytes + pos, &used, &cp)) {
        free_value(out);
        runtime_error(line, "ascii_array input contains invalid UTF-8 text.");
        return v_null();
      }
      if (ar->count >= MAX_ARGS * 1024) {
        free_value(out);
        runtime_error(line, "ascii_array input has too many characters.");
        return v_null();
      }
      if (ar->count == ar->capacity) {
        ar->capacity = ar->capacity ? ar->capacity * 2 : 16;
        ar->items =
            (Value *)xrealloc(ar->items, (size_t)ar->capacity * sizeof(Value));
      }
      char tmp[16];
      snprintf(tmp, sizeof(tmp), "%u", (unsigned)cp);
      ar->items[ar->count++] = v_bigint(tmp);
      pos += used;
    }
    return out;
  }
  if (strcmp(name, "ord") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line) ||
        !a[0].as.string[0]) {
      runtime_error(line, "ord expects a non-empty string.");
      return v_null();
    }
    size_t used = 0;
    uint32_t cp = 0;
    if (!utf8_decode_one((const unsigned char *)a[0].as.string, &used, &cp)) {
      runtime_error(line, "ord expects valid UTF-8 text.");
      return v_null();
    }
    return v_num((double)cp);
  }
  if (strcmp(name, "chr") == 0) {
    uint32_t cp = 0;
    if (argc != 1 || !numeric_to_u32(a[0], &cp) || cp > 0x10FFFF ||
        (cp >= 0xD800 && cp <= 0xDFFF)) {
      runtime_error(line, "chr expects a Unicode code point from 0 to 1114111, "
                          "excluding surrogate values.");
      return v_null();
    }
    char encoded[5];
    if (!utf8_encode_codepoint(cp, encoded)) {
      runtime_error(line, "chr could not encode that Unicode code point.");
      return v_null();
    }
    return v_string(encoded);
  }
  if (strcmp(name, "random") == 0) {
    if (argc == 0)
      return v_num((double)rand() / (double)RAND_MAX);
    long low = 0, high = 0;
    if (argc == 2 && numeric_to_long(a[0], &low) &&
        numeric_to_long(a[1], &high) && low <= high && low >= -2147483647L &&
        high <= 2147483647L) {
      unsigned long long span =
          (unsigned long long)((long long)high - (long long)low) + 1ULL;
      long long result =
          (long long)low + (long long)((unsigned long long)rand() % span);
      char number[64];
      snprintf(number, sizeof(number), "%lld", result);
      return v_bigint(number);
    }
    runtime_error(line, "random expects no arguments or integer bounds between "
                        "-2147483647 and 2147483647.");
    return v_null();
  }
  if (strcmp(name, "read_text") == 0) {
    if ((argc != 1 && argc != 2) || !std_string_arg(name, argc, a, 0, line) ||
        (argc == 2 && !std_string_arg(name, argc, a, 1, line)))
      return v_null();
    const char *enc = argc == 2 ? a[1].as.string : "UTF-8";
    if (!encoding_name_valid(enc)) {
      runtime_error(line, "read_text: unsupported encoding '%s'.", enc);
      return v_null();
    }
    char *b = NULL;
    size_t n = 0;
    if (!file_read_encoded(a[0].as.string, enc, &b, &n)) {
      runtime_error(line,
                    "read_text could not decode '%s' as %s (file missing, "
                    "invalid byte sequence, or over 16 MiB).",
                    a[0].as.string, enc);
      return v_null();
    }
    Value v = v_string(b);
    free(b);
    return v;
  }
  if (strcmp(name, "write_text") == 0 || strcmp(name, "append_text") == 0) {
    if ((argc != 2 && argc != 3) || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line) ||
        (argc == 3 && !std_string_arg(name, argc, a, 2, line)))
      return v_null();
    const char *enc = argc == 3 ? a[2].as.string : "UTF-8";
    if (!encoding_name_valid(enc)) {
      runtime_error(line, "%s: unsupported encoding '%s'.", name, enc);
      return v_null();
    }
    size_t n = 0;
    if (!file_write_encoded(a[0].as.string, a[1].as.string, enc,
                            strcmp(name, "append_text") == 0, &n)) {
      runtime_error(line,
                    "%s failed for '%s' using encoding %s (invalid text, "
                    "conversion failure, or filesystem error).",
                    name, a[0].as.string, enc);
      return v_null();
    }
    return v_num((double)n);
  }
  if (strcmp(name, "file_exists") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    FILE *f = fopen(a[0].as.string, "rb");
    if (!f)
      return v_bool(false);
    fclose(f);
    return v_bool(true);
  }
  if (strcmp(name, "remove_file") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    return v_bool(remove(a[0].as.string) == 0);
  }
  if (strcmp(name, "file_size") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    struct stat st;
    if (stat(a[0].as.string, &st) != 0 || !S_ISREG(st.st_mode))
      return v_num(-1);
    return v_num((double)st.st_size);
  }
  if (strcmp(name, "rename_file") == 0) {
    if (argc != 2 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line))
      return v_null();
    return v_bool(rename(a[0].as.string, a[1].as.string) == 0);
  }
  if (strcmp(name, "copy_file") == 0) {
    if (argc != 2 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line))
      return v_null();
    FILE *src = fopen(a[0].as.string, "rb");
    if (!src)
      return v_bool(false);
    FILE *dst = fopen(a[1].as.string, "wb");
    if (!dst) {
      fclose(src);
      return v_bool(false);
    }
    unsigned char buffer[16384];
    size_t n;
    bool ok = true;
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0)
      if (fwrite(buffer, 1, n, dst) != n) {
        ok = false;
        break;
      }
    if (ferror(src))
      ok = false;
    if (fclose(src) != 0)
      ok = false;
    if (fclose(dst) != 0)
      ok = false;
    if (!ok)
      (void)remove(a[1].as.string);
    return v_bool(ok);
  }
  if (strcmp(name, "make_dir") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
#ifdef _WIN32
    return v_bool(CreateDirectoryA(a[0].as.string, NULL) != 0);
#else
    return v_bool(mkdir(a[0].as.string, 0777) == 0);
#endif
  }
  if (strcmp(name, "is_dir") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    struct stat st;
    return v_bool(stat(a[0].as.string, &st) == 0 && S_ISDIR(st.st_mode));
  }
  if (strcmp(name, "env") == 0) {
    if (argc != 1 || !std_string_arg(name, argc, a, 0, line))
      return v_null();
    const char *v = getenv(a[0].as.string);
    return v == NULL ? v_null() : v_string(v);
  }
  if (strcmp(name, "set_env") == 0) {
    if (argc != 2 || !std_string_arg(name, argc, a, 0, line) ||
        !std_string_arg(name, argc, a, 1, line))
      return v_null();
#ifdef _WIN32
    return v_bool(SetEnvironmentVariableA(a[0].as.string, a[1].as.string) != 0);
#else
    return v_bool(setenv(a[0].as.string, a[1].as.string, 1) == 0);
#endif
  }
  if (strcmp(name, "clock_ms") == 0) {
    if (argc != 0) {
      runtime_error(line, "clock_ms expects no arguments.");
      return v_null();
    }
    return v_num((double)clock() * 1000.0 / (double)CLOCKS_PER_SEC);
  }
  if (strcmp(name, "sleep_ms") == 0) {
    long milliseconds = 0;
    if (argc != 1 || !numeric_to_long(a[0], &milliseconds) ||
        milliseconds < 0 || milliseconds > 86400000) {
      runtime_error(line, "sleep_ms expects milliseconds from 0 to 86400000.");
      return v_null();
    }
#ifdef _WIN32
    Sleep((DWORD)milliseconds);
#else
    {
      struct timespec ts;
      ts.tv_sec = (time_t)(milliseconds / 1000);
      ts.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
      while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
      }
    }
#endif
    return v_null();
  }
  if (strcmp(name, "assert") == 0) {
    if (argc < 1 || argc > 2) {
      runtime_error(line, "assert expects condition and optional message.");
      return v_null();
    }
    if (!truthy(a[0])) {
      char *m = argc == 2 ? value_to_string(a[1]) : copy_s("assertion failed");
      runtime_error(line, "Assertion failed: %s", m);
      free(m);
      return v_null();
    }
    return v_bool(true);
  }
  if (strcmp(name, "printf") == 0)
    return native_printf(argc, a, line);
  if (strcmp(name, "print") == 0) {
    for (int i = 0; i < argc; i++) {
      if (i)
        fputc(' ', stdout);
      char *shown = value_to_string(a[i]);
      fputs(shown ? shown : "null", stdout);
      free(shown);
    }
    fputc('\n', stdout);
    fflush(stdout);
    return v_null();
  }
  if (strcmp(name, "input") == 0) {
    if (argc > 1) {
      runtime_error(line, "input expects zero or one argument.");
      return v_null();
    }
    if (argc == 1) {
      char *s = value_to_string(a[0]);
      fputs(s, stdout);
      free(s);
      fflush(stdout);
    }
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin))
      return v_string("");
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n')
      buf[n - 1] = '\0';
    return v_string(buf);
  }
  if (strcmp(name, "is_number") == 0) {
    if (argc != 1) {
      runtime_error(line, "is_number expects one argument.");
      return v_null();
    }
    return v_bool(is_numeric(a[0]));
  }
  if (strcmp(name, "len") == 0) {
    if (argc != 1) {
      runtime_error(line, "len expects one argument.");
      return v_null();
    }
    if (a[0].type == V_ARRAY)
      return v_num(a[0].as.array->count);
    if (a[0].type == V_MAP)
      return v_num(a[0].as.map->count);
    if (a[0].type == V_STRING)
      return v_num(strlen(a[0].as.string));
    runtime_error(line, "len expects an array, map, or string.");
    return v_null();
  }
  if (strcmp(name, "range") == 0) {
    if (argc < 1 || argc > 3) {
      runtime_error(line, "range expects one to three numeric arguments.");
      return v_null();
    }
    for (int i = 0; i < argc; i++)
      if (!is_numeric(a[i])) {
        runtime_error(line, "range arguments must be numbers.");
        return v_null();
      }
    Value start = argc == 1 ? v_bigint("0") : copy_value(a[0]);
    Value stop = copy_value(argc == 1 ? a[0] : a[1]);
    Value step = argc == 3 ? copy_value(a[2]) : v_bigint("1");
    Value zero = v_bigint("0");
    if (numeric_cmp(step, zero) == 0) {
      runtime_error(line, "range step cannot be zero.");
      free_value(start);
      free_value(stop);
      free_value(step);
      free_value(zero);
      return v_null();
    }
    bool exact = is_exact_numeric(start) && is_exact_numeric(stop) &&
                 is_exact_numeric(step);
    Value result = v_array();
    Array *ar = result.as.array;
    if (exact) {
      bool forward = numeric_cmp(step, zero) > 0;
      Value current = copy_value(start);
      while (forward ? numeric_cmp(current, stop) < 0
                     : numeric_cmp(current, stop) > 0) {
        if (ar->count >= 1000000) {
          runtime_error(line, "range exceeds one million items.");
          free_value(current);
          free_value(start);
          free_value(stop);
          free_value(step);
          free_value(zero);
          return v_null();
        }
        if (ar->count == ar->capacity) {
          if (ar->capacity > INT_MAX / 2) {
            runtime_error(line, "range storage limit exceeded.");
            free_value(current);
            free_value(start);
            free_value(stop);
            free_value(step);
            free_value(zero);
            return v_null();
          }
          ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
          ar->items = (Value *)xrealloc(ar->items,
                                        (size_t)ar->capacity * sizeof(Value));
        }
        ar->items[ar->count++] = copy_value(current);
        Value next = numeric_binary(T_PLUS, current, step, line);
        free_value(current);
        current = next;
        if (has_exception)
          break;
      }
      free_value(current);
    } else {
      double x = numeric_double(start), end = numeric_double(stop),
             delta = numeric_double(step);
      bool forward = delta > 0;
      for (; isfinite(x) && (forward ? x < end : x > end);) {
        if (ar->count >= 1000000) {
          runtime_error(line, "range exceeds one million items.");
          free_value(start);
          free_value(stop);
          free_value(step);
          free_value(zero);
          return v_null();
        }
        if (ar->count == ar->capacity) {
          if (ar->capacity > INT_MAX / 2) {
            runtime_error(line, "range storage limit exceeded.");
            free_value(start);
            free_value(stop);
            free_value(step);
            free_value(zero);
            return v_null();
          }
          ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
          ar->items = (Value *)xrealloc(ar->items,
                                        (size_t)ar->capacity * sizeof(Value));
        }
        ar->items[ar->count++] = v_num(x);
        double next = x + delta;
        if (next == x) {
          runtime_error(line, "range step is too small to advance the current "
                              "floating-point value.");
          break;
        }
        x = next;
      }
    }
    free_value(start);
    free_value(stop);
    free_value(step);
    free_value(zero);
    return result;
  }
  if (strcmp(name, "push") == 0) {
    if (argc != 2 || a[0].type != V_ARRAY) {
      runtime_error(line, "push expects (array, value).");
      return v_null();
    }
    Array *ar = a[0].as.array;
    if (ar->count == ar->capacity) {
      ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
      ar->items =
          (Value *)xrealloc(ar->items, (size_t)ar->capacity * sizeof(Value));
    }
    ar->items[ar->count++] = copy_value(a[1]);
    return v_num(ar->count);
  }
  if (strcmp(name, "pop") == 0) {
    if (argc != 1 || a[0].type != V_ARRAY) {
      runtime_error(line, "pop expects one array.");
      return v_null();
    }
    Array *ar = a[0].as.array;
    if (!ar->count) {
      runtime_error(line, "pop cannot remove from an empty array.");
      return v_null();
    }
    Value v = copy_value(ar->items[--ar->count]);
    free_value(ar->items[ar->count]);
    return v;
  }
  if (strcmp(name, "gc") == 0) {
    if (argc != 0) {
      runtime_error(line, "gc expects no arguments.");
      return v_null();
    }
    gc_pending = true;
    return v_num(0);
  }
  if (strcmp(name, "keys") == 0) {
    if (argc != 1 || a[0].type != V_MAP) {
      runtime_error(line, "keys expects one map.");
      return v_null();
    }
    Value out = v_array();
    for (int i = 0; i < a[0].as.map->count; i++) {
      Array *ar = out.as.array;
      if (ar->count == ar->capacity) {
        ar->capacity = ar->capacity ? ar->capacity * 2 : 8;
        ar->items =
            (Value *)xrealloc(ar->items, (size_t)ar->capacity * sizeof(Value));
      }
      ar->items[ar->count++] = copy_value(a[0].as.map->items[i].key);
    }
    return out;
  }
  if (strcmp(name, "has_key") == 0) {
    if (argc != 2 || a[0].type != V_MAP) {
      runtime_error(line, "has_key expects (map, key).");
      return v_null();
    }
    return v_bool(map_find(a[0].as.map, a[1]) >= 0);
  }
  if (strcmp(name, "py_inline") == 0) {
    if (argc != 1 || a[0].type != V_STRING) {
      runtime_error(line, "py_inline expects one source-code string.");
      return v_null();
    }
    const char *pyexe = bundled_python_path();
    if (!pyexe || !file_is_runnable(pyexe)) {
      runtime_error(
          line,
          "Bundled Python interpreter is missing or not executable at '%s'.",
          pyexe ? pyexe : "<unresolved path>");
      return v_null();
    }
    const char *child_argv[] = {pyexe, "-c", a[0].as.string, NULL};
    int rc = run_process(child_argv);
    if (rc < 0) {
      runtime_error(line, "Could not start bundled Python interpreter '%s'.",
                    pyexe);
      return v_null();
    }
    return v_num(rc);
  }
  if (strcmp(name, "c_inline") == 0 || strcmp(name, "asm_inline") == 0) {
    if (argc != 1 || a[0].type != V_STRING) {
      runtime_error(line, "inline native code expects one source-code string.");
      return v_null();
    }
    const char *cc = bundled_gcc_path();
    if (!cc || !file_is_runnable(cc)) {
      runtime_error(
          line, "Bundled GCC compiler is missing or not executable at '%s'.",
          cc ? cc : "<unresolved path>");
      return v_null();
    }
    char tempdir[FLUORINE_PATH_CAP] = "", srcpath[FLUORINE_PATH_CAP] = "",
         libpath[FLUORINE_PATH_CAP] = "";
    bool is_c_source = strcmp(name, "c_inline") == 0;
    if (!create_inline_temp_paths(tempdir, sizeof(tempdir), srcpath,
                                  sizeof(srcpath), libpath, sizeof(libpath),
                                  is_c_source)) {
      runtime_error(line, "Could not create a private temporary directory for "
                          "inline native compilation.");
      return v_null();
    }
    FILE *sf = fopen(srcpath, "wb");
    if (!sf) {
      remove_inline_temp_paths(tempdir, srcpath, libpath);
      runtime_error(line,
                    "Cannot create temporary inline native source file: %s",
                    strerror(errno));
      return v_null();
    }
    bool source_ok;
    if (is_c_source)
      source_ok =
          fprintf(sf,
                  "#include <math.h>\ndouble fluorine_inline(void) { %s\n}\n",
                  a[0].as.string) >= 0;
    else
      source_ok = fputs(a[0].as.string, sf) >= 0;
    if (fclose(sf) != 0)
      source_ok = false;
    if (!source_ok) {
      remove_inline_temp_paths(tempdir, srcpath, libpath);
      runtime_error(line, "Could not write temporary inline native source.");
      return v_null();
    }
#ifndef _WIN32
    const char *child_argv[] = {cc,   "-shared", "-fPIC", srcpath,
                                "-o", libpath,   "-lm",   NULL};
#else
    const char *child_argv[] = {cc,      "-shared", srcpath, "-o",
                                libpath, "-lm",     NULL};
#endif
    int rc = run_process(child_argv);
    if (rc != 0) {
      remove_inline_temp_paths(tempdir, srcpath, libpath);
      runtime_error(
          line, "Inline native compilation failed using '%s' (exit status %d).",
          cc, rc);
      return v_null();
    }
    void *h = NULL;
    void *sym = NULL;
#ifdef _WIN32
    h = (void *)LoadLibraryA(libpath);
    if (h)
      sym = (void *)GetProcAddress((HMODULE)h, "fluorine_inline");
#else
    h = dlopen(libpath, RTLD_NOW | RTLD_LOCAL);
    if (h)
      sym = dlsym(h, "fluorine_inline");
#endif
    if (!sym) {
#ifdef _WIN32
      if (h)
        FreeLibrary((HMODULE)h);
#else
      if (h)
        dlclose(h);
#endif
      remove_inline_temp_paths(tempdir, srcpath, libpath);
      runtime_error(
          line,
          "Inline native source must export double fluorine_inline(void).");
      return v_null();
    }
    union InlineFn {
      void *object;
      double (*fn)(void);
    } fn;
    fn.object = sym;
    double result = fn.fn();
#ifdef _WIN32
    FreeLibrary((HMODULE)h);
#else
    dlclose(h);
#endif
    remove_inline_temp_paths(tempdir, srcpath, libpath);
    return v_num(result);
  }
  if (strcmp(name, "py") == 0) {
    if (argc < 1 || argc > 2 || a[0].type != V_STRING) {
      runtime_error(line, "py expects Python source code and an optional map "
                          "of variables, e.g. py(\"print(x)\", {x: 42}).");
      return v_null();
    }
    if (argc == 2 && a[1].type != V_MAP) {
      runtime_error(line, "py's optional second argument must be a map of "
                          "variable names to values.");
      return v_null();
    }
    const char *exe = bundled_python_path();
    if (!exe || !file_is_runnable(exe)) {
      runtime_error(
          line,
          "Bundled Python interpreter is missing or not executable at '%s'.",
          exe ? exe : "<unresolved path>");
      return v_null();
    }
    char tempdir[FLUORINE_PATH_CAP] = "", dummy[FLUORINE_PATH_CAP] = "",
         libpath[FLUORINE_PATH_CAP] = "", script[FLUORINE_PATH_CAP] = "";
    if (!create_inline_temp_paths(tempdir, sizeof(tempdir), dummy,
                                  sizeof(dummy), libpath, sizeof(libpath),
                                  false) ||
        !path_join(script, sizeof(script), tempdir, "source.py")) {
      if (tempdir[0])
        remove_inline_temp_paths(tempdir, dummy, libpath);
      runtime_error(line,
                    "Could not create a private temporary Python script path.");
      return v_null();
    }
    FILE *sf = fopen(script, "wb");
    if (!sf) {
      remove_inline_temp_paths(tempdir, dummy, libpath);
      runtime_error(line, "Cannot create temporary Python script: %s",
                    strerror(errno));
      return v_null();
    }
    bool source_ok = true;
    if (argc == 2) {
      Map *m = a[1].as.map;
      for (int i = 0; i < m->count && source_ok; i++) {
        Value key = m->items[i].key, val = m->items[i].value;
        if (key.type != V_STRING || !key.as.string[0] ||
            (!(isalpha((unsigned char)key.as.string[0]) ||
               key.as.string[0] == '_'))) {
          runtime_error(line, "Python variable names must be valid identifiers "
                              "supplied as string keys.");
          source_ok = false;
          break;
        }
        for (const char *q = key.as.string + 1; *q; q++)
          if (!(isalnum((unsigned char)*q) || *q == '_')) {
            runtime_error(line,
                          "Python variable names must be valid identifiers.");
            source_ok = false;
            break;
          }
        if (!source_ok)
          break;
        if (!strcmp(key.as.string, "True") || !strcmp(key.as.string, "False") ||
            !strcmp(key.as.string, "None") ||
            !strcmp(key.as.string, "import") ||
            !strcmp(key.as.string, "exec") || !strcmp(key.as.string, "class") ||
            !strcmp(key.as.string, "def")) {
          runtime_error(line, "Python variable name is reserved.");
          source_ok = false;
          break;
        }
        if (fprintf(sf, "%s = ", key.as.string) < 0) {
          source_ok = false;
          break;
        }
        if (val.type == V_STRING) {
          if (fputc('\'', sf) == EOF) {
            source_ok = false;
            break;
          }
          for (const unsigned char *q = (const unsigned char *)val.as.string;
               *q; q++) {
            if (*q == '\\' || *q == '\'') {
              if (fputc('\\', sf) == EOF || fputc(*q, sf) == EOF) {
                source_ok = false;
                break;
              }
            } else if (*q == '\n') {
              if (fputs("\\n", sf) == EOF) {
                source_ok = false;
                break;
              }
            } else if (*q == '\r') {
              if (fputs("\\r", sf) == EOF) {
                source_ok = false;
                break;
              }
            } else if (*q == '\t') {
              if (fputs("\\t", sf) == EOF) {
                source_ok = false;
                break;
              }
            } else if (*q < 32) {
              if (fprintf(sf, "\\x%02x", (unsigned)*q) < 0) {
                source_ok = false;
                break;
              }
            } else if (fputc(*q, sf) == EOF) {
              source_ok = false;
              break;
            }
          }
          if (source_ok && fputc('\'', sf) == EOF)
            source_ok = false;
        } else if (val.type == V_BOOL) {
          if (fputs(val.as.boolean ? "True" : "False", sf) == EOF)
            source_ok = false;
        } else if (val.type == V_NULL) {
          if (fputs("None", sf) == EOF)
            source_ok = false;
        } else if (is_numeric(val)) {
          char *num = value_to_string(val);
          if (fputs(num, sf) == EOF)
            source_ok = false;
          free(num);
        } else {
          runtime_error(line, "Python variable values currently support "
                              "strings, numbers, booleans, and null.");
          source_ok = false;
        }
        if (source_ok && fputc('\n', sf) == EOF)
          source_ok = false;
      }
    }
    if (source_ok && fputs(a[0].as.string, sf) == EOF)
      source_ok = false;
    if (fclose(sf) != 0)
      source_ok = false;
    if (!source_ok) {
      remove_inline_temp_paths(tempdir, script, libpath);
      if (!had_error)
        runtime_error(line, "Could not write temporary Python script.");
      return v_null();
    }
    const char *child_argv[] = {exe, script, NULL};
    int rc = run_process(child_argv);
    remove_inline_temp_paths(tempdir, script, libpath);
    if (rc < 0) {
      runtime_error(line, "Could not start bundled Python interpreter '%s'.",
                    exe);
      return v_null();
    }
    return v_num(rc);
  }
  if (strcmp(name, "native") == 0) {
    if (argc < 2 || argc > 6 || a[0].type != V_STRING ||
        a[1].type != V_STRING) {
      runtime_error(
          line,
          "native expects (library, symbol, up to four numeric arguments).");
      return v_null();
    }
    for (int i = 2; i < argc; i++)
      if (!is_numeric(a[i]) || !isfinite(numeric_double(a[i]))) {
        runtime_error(
            line,
            "native arguments after symbol must fit finite C double values.");
        return v_null();
      }
    if (!safe_relative_path(a[0].as.string)) {
      runtime_error(line, "Native library path must stay inside bin/python.");
      return v_null();
    }
    char path[FLUORINE_PATH_CAP], relative[FLUORINE_PATH_CAP];
    int path_len =
        snprintf(relative, sizeof(relative), "bin/python/%s", a[0].as.string);
    if (path_len < 0 || (size_t)path_len >= sizeof(relative) ||
        !resolve_install_path(path, sizeof(path), relative)) {
      runtime_error(line, "Native library path is too long.");
      return v_null();
    }
    void *handle = NULL;
    void *symbol = NULL;
#ifdef _WIN32
    handle = (void *)LoadLibraryA(path);
    if (handle)
      symbol = (void *)GetProcAddress((HMODULE)handle, a[1].as.string);
#else
    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (handle)
      symbol = dlsym(handle, a[1].as.string);
#endif
    if (!symbol) {
      runtime_error(line, "Could not load native symbol '%s' from '%s'.",
                    a[1].as.string, path);
      return v_null();
    }
    double x[4] = {0, 0, 0, 0};
    for (int i = 2; i < argc; i++)
      x[i - 2] = numeric_double(a[i]);
    double result = 0;
    int n = argc - 2;
    union NativeSymbol {
      void *object;
      double (*f0)(void);
      double (*f1)(double);
      double (*f2)(double, double);
      double (*f3)(double, double, double);
      double (*f4)(double, double, double, double);
    } fn;
    fn.object = symbol;
    if (n == 0)
      result = fn.f0();
    else if (n == 1)
      result = fn.f1(x[0]);
    else if (n == 2)
      result = fn.f2(x[0], x[1]);
    else if (n == 3)
      result = fn.f3(x[0], x[1], x[2]);
    else
      result = fn.f4(x[0], x[1], x[2], x[3]);
#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
    return v_num(result);
  }

  if (strcmp(name, "min") == 0 || strcmp(name, "max") == 0) {
    if (!require_numbers(name, argc, a, line, 1, MAX_ARGS))
      return v_null();
    int best = 0;
    for (int i = 1; i < argc; i++) {
      int cmp = numeric_cmp(a[i], a[best]);
      if ((strcmp(name, "min") == 0 && cmp < 0) ||
          (strcmp(name, "max") == 0 && cmp > 0))
        best = i;
    }
    return copy_value(a[best]);
  }
  int min = 1, max = 1;
  if (strcmp(name, "pow") == 0)
    min = max = 2;
  if (!require_numbers(name, argc, a, line, min, max))
    return v_null();
  if (strcmp(name, "pow") == 0 && is_exact_numeric(a[0]) &&
      is_exact_numeric(a[1])) {
    char *en, *ed;
    exact_components(a[1], &en, &ed);
    bool integral = strcmp(ed, "1") == 0;
    free(en);
    free(ed);
    if (integral)
      return exact_power(a[0], a[1], line);
  }
  if ((strcmp(name, "floor") == 0 || strcmp(name, "ceil") == 0 ||
       strcmp(name, "round") == 0) &&
      is_exact_numeric(a[0])) {
    if (a[0].type == V_BIGINT)
      return copy_value(a[0]);
    char *num = a[0].as.rational.num, *den = a[0].as.rational.den;
    char *q = bi_div(num, den, false), *rem = bi_div(num, den, true);
    if (strcmp(rem, "0") != 0) {
      if (strcmp(name, "floor") == 0 && num[0] == '-') {
        char *t = bi_add(q, "-1");
        free(q);
        q = t;
      } else if (strcmp(name, "ceil") == 0 && num[0] != '-') {
        char *t = bi_add(q, "1");
        free(q);
        q = t;
      } else if (strcmp(name, "round") == 0) {
        char *twice = bi_mul(bi_mag(rem), "2");
        if (bi_cmpmag(twice, den) >= 0) {
          char *t = bi_add(q, num[0] == '-' ? "-1" : "1");
          free(q);
          q = t;
        }
        free(twice);
      }
    }
    Value rounded = v_bigint(q);
    free(q);
    free(rem);
    return rounded;
  }
  double x = numeric_double(a[0]), y = 0, r = 0;
  if (strcmp(name, "sqrt") == 0) {
    if (x < 0) {
      runtime_error(line, "sqrt domain error.");
      return v_null();
    }
    r = sqrt(x);
  } else if (strcmp(name, "abs") == 0) {
    if (a[0].type == V_BIGINT) {
      if (a[0].as.bigint[0] == '-')
        return v_bigint(a[0].as.bigint + 1);
      return copy_value(a[0]);
    }
    if (a[0].type == V_RATIONAL) {
      if (a[0].as.rational.num[0] == '-') {
        char *positive = bi_neg(a[0].as.rational.num);
        Value out = v_ratio(positive, a[0].as.rational.den);
        free(positive);
        return out;
      }
      return copy_value(a[0]);
    }
    r = fabs(x);
  } else if (strcmp(name, "floor") == 0)
    r = floor(x);
  else if (strcmp(name, "ceil") == 0)
    r = ceil(x);
  else if (strcmp(name, "round") == 0)
    r = round(x);
  else if (strcmp(name, "sin") == 0)
    r = sin(x);
  else if (strcmp(name, "cos") == 0)
    r = cos(x);
  else if (strcmp(name, "tan") == 0)
    r = tan(x);
  else if (strcmp(name, "asin") == 0)
    r = asin(x);
  else if (strcmp(name, "acos") == 0)
    r = acos(x);
  else if (strcmp(name, "atan") == 0)
    r = atan(x);
  else if (strcmp(name, "log") == 0) {
    if (x <= 0) {
      runtime_error(line, "log domain error.");
      return v_null();
    }
    r = log(x);
  } else if (strcmp(name, "log10") == 0) {
    if (x <= 0) {
      runtime_error(line, "log10 domain error.");
      return v_null();
    }
    r = log10(x);
  } else if (strcmp(name, "exp") == 0)
    r = exp(x);
  else if (strcmp(name, "pow") == 0) {
    y = numeric_double(a[1]);
    r = pow(x, y);
  } else {
    runtime_error(line, "Fluorine built-in '%s'.", name);
    return v_null();
  }
  return v_num(r);
}

static Flow normal_flow(Value v) {
  Flow f;
  f.is_return = f.is_break = f.is_continue = f.is_throw = false;
  f.value = v;
  return f;
}
static Flow exec_impl(Node *n, Env *env) {
  if (!n || had_error)
    return normal_flow(v_null());
  switch (n->type) {
  case N_IMPORT: {
    Value mod = run_import(n->as.import_stmt.path, env, n->line,
                           n->as.import_stmt.alias, n->as.import_stmt.quoted);
    if (n->as.import_stmt.alias)
      define_var(env, n->as.import_stmt.alias, mod, n->line);
    free_value(mod);
    return normal_flow(v_null());
  }
  case N_EXPR_STMT: {
    Value v = eval(n->as.expr_stmt.expr, env);
    free_value(v);
    if (has_exception) {
      Flow f = normal_flow(copy_value(pending_exception));
      f.is_throw = true;
      return f;
    }
    return normal_flow(v_null());
  }
  case N_LET: {
    Value v = n->as.let_stmt.init ? eval(n->as.let_stmt.init, env) : v_null();
    define_var(env, n->as.let_stmt.name, v, n->line);
    free_value(v);
    if (has_exception) {
      Flow f = normal_flow(copy_value(pending_exception));
      f.is_throw = true;
      return f;
    }
    return normal_flow(v_null());
  }
  case N_BLOCK: {
    Env *local = new_env(env);
    for (int i = 0; i < n->as.block.count && !had_error; i++) {
      Flow f = exec(n->as.block.items[i], local);
      if (f.is_return || f.is_break || f.is_continue || f.is_throw)
        return f;
    }
    return normal_flow(v_null());
  }
  case N_SWITCH: {
    Value selector = eval(n->as.switch_stmt.expr, env);
    if (had_error) {
      free_value(selector);
      return normal_flow(v_null());
    }
    int selected = -1, default_index = -1;
    for (int i = 0; i < n->as.switch_stmt.count; i++) {
      if (n->as.switch_stmt.defaults[i]) {
        default_index = i;
        continue;
      }
      Value candidate = eval(n->as.switch_stmt.values[i], env);
      if (had_error) {
        free_value(candidate);
        free_value(selector);
        return normal_flow(v_null());
      }
      bool match_case = equal_values(selector, candidate);
      free_value(candidate);
      if (match_case) {
        selected = i;
        break;
      }
    }
    free_value(selector);
    if (selected < 0)
      selected = default_index;
    if (selected < 0)
      return normal_flow(v_null());
    for (int i = selected; i < n->as.switch_stmt.count && !had_error; i++) {
      for (int j = 0; j < n->as.switch_stmt.counts[i] && !had_error; j++) {
        Flow f = exec(n->as.switch_stmt.bodies[i][j], env);
        if (f.is_break) {
          free_value(f.value);
          return normal_flow(v_null());
        }
        if (f.is_return || f.is_throw || f.is_continue)
          return f;
        free_value(f.value);
      }
    }
    return normal_flow(v_null());
  }
  case N_IF: {
    Value c = eval(n->as.if_stmt.condition, env);
    bool yes = truthy(c);
    free_value(c);
    if (yes)
      return exec(n->as.if_stmt.then_branch, env);
    if (n->as.if_stmt.else_branch)
      return exec(n->as.if_stmt.else_branch, env);
    return normal_flow(v_null());
  }
  case N_DO_WHILE: {
    do {
      Flow f = exec(n->as.while_stmt.body, env);
      if (f.is_return || f.is_throw)
        return f;
      if (f.is_break)
        break;
      Value c = eval(n->as.while_stmt.condition, env);
      bool yes = truthy(c);
      free_value(c);
      if (!yes)
        break;
    } while (!had_error);
    return normal_flow(v_null());
  }
  case N_WHILE: {
    while (!had_error) {
      Value c = eval(n->as.while_stmt.condition, env);
      bool yes = truthy(c);
      free_value(c);
      if (!yes)
        break;
      Flow f = exec(n->as.while_stmt.body, env);
      if (f.is_return || f.is_throw)
        return f;
      if (f.is_break)
        break;
      if (f.is_continue)
        continue;
    }
    return normal_flow(v_null());
  }
  case N_FOR: {
    Env *loop_env = new_env(env);
    if (n->as.for_stmt.init) {
      Flow f = exec(n->as.for_stmt.init, loop_env);
      if (f.is_return || f.is_break || f.is_continue || f.is_throw)
        return f;
    }
    while (!had_error) {
      if (n->as.for_stmt.condition) {
        Value c = eval(n->as.for_stmt.condition, loop_env);
        bool yes = truthy(c);
        free_value(c);
        if (!yes)
          break;
      }
      Flow f = exec(n->as.for_stmt.body, loop_env);
      if (f.is_return || f.is_throw)
        return f;
      if (f.is_break)
        break;
      if (n->as.for_stmt.increment) {
        Value v = eval(n->as.for_stmt.increment, loop_env);
        free_value(v);
      }
      /* continue naturally proceeds to increment */
    }
    return normal_flow(v_null());
  }
  case N_FOR_IN: {
    Value iterable = eval(n->as.for_in.iterable, env);
    if (iterable.type != V_ARRAY && iterable.type != V_MAP &&
        iterable.type != V_STRING) {
      runtime_error(n->line, "for-in requires an array, string, or map.");
      return normal_flow(v_null());
    }
    Env *loop_env = new_env(env);
    define_var(loop_env, n->as.for_in.name, v_null(), n->line);
    int iter_count =
        iterable.type == V_ARRAY
            ? iterable.as.array->count
            : (iterable.type == V_MAP ? iterable.as.map->count
                                      : (int)strlen(iterable.as.string));
    for (int i = 0; i < iter_count && !had_error; i++) {
      Value item;
      if (iterable.type == V_ARRAY)
        item = iterable.as.array->items[i];
      else if (iterable.type == V_MAP)
        item = iterable.as.map->items[i].key;
      else {
        char ch[2] = {iterable.as.string[i], 0};
        item = v_string(ch);
      }
      assign_var(loop_env, n->as.for_in.name, item, n->line);
      Flow f = exec(n->as.for_in.body, loop_env);
      if (iterable.type == V_STRING)
        free_value(item);
      if (f.is_return) {
        free_value(iterable);
        return f;
      }
      if (f.is_break)
        break;
      if (f.is_continue)
        continue;
    }
    free_value(iterable);
    return normal_flow(v_null());
  }
  case N_FLUORINE: {
    Node *call = n->as.fluorine_stmt.call;
    Value callee = v_null();
    char *native_name = NULL;
    if (call->as.call.callee->type == N_VAR &&
        find_binding(env, call->as.call.callee->as.var.name) == NULL &&
        is_native_builtin_name(call->as.call.callee->as.var.name)) {
      native_name = copy_s(call->as.call.callee->as.var.name);
    } else {
      callee = eval(call->as.call.callee, env);
      if (had_error) {
        free_value(callee);
        return normal_flow(v_null());
      }
      if (callee.type != V_FUNCTION) {
        runtime_error(n->line, "'fluorine' requires a user-defined function or "
                               "built-in function call.");
        free_value(callee);
        return normal_flow(v_null());
      }
    }
    int argc = call->as.call.count;
    Value *args = (Value *)calloc((size_t)(argc ? argc : 1), sizeof(Value));
    if (!args) {
      free_value(callee);
      free(native_name);
      exit(2);
    }
    for (int i = 0; i < argc && !had_error; i++)
      args[i] = eval(call->as.call.args[i], env);
    if (had_error) {
      for (int i = 0; i < argc; i++)
        free_value(args[i]);
      free(args);
      free_value(callee);
      free(native_name);
      return normal_flow(v_null());
    }
    enqueue_fluorine_task(callee, native_name, argc, args, n->line);
    free_value(callee);
    free(native_name);
    for (int i = 0; i < argc; i++)
      free_value(args[i]);
    free(args);
    return normal_flow(v_null());
  }
  case N_THREAD: {
    Node *call = n->as.fluorine_stmt.call;
    Value callee = v_null();
    char *native_name = NULL;
    if (call->as.call.callee->type == N_VAR &&
        find_binding(env, call->as.call.callee->as.var.name) == NULL &&
        is_native_builtin_name(call->as.call.callee->as.var.name)) {
      native_name = copy_s(call->as.call.callee->as.var.name);
    } else {
      callee = eval(call->as.call.callee, env);
      if (had_error) {
        free_value(callee);
        return normal_flow(v_null());
      }
      if (callee.type != V_FUNCTION) {
        runtime_error(n->line, "'fluorine' requires a user-defined function or "
                               "built-in function call.");
        free_value(callee);
        return normal_flow(v_null());
      }
    }
    int argc = call->as.call.count;
    Value *args = (Value *)calloc((size_t)(argc ? argc : 1), sizeof(Value));
    if (!args) {
      free_value(callee);
      free(native_name);
      exit(2);
    }
    for (int i = 0; i < argc && !had_error; i++)
      args[i] = eval(call->as.call.args[i], env);
    if (had_error) {
      for (int i = 0; i < argc; i++)
        free_value(args[i]);
      free(args);
      free_value(callee);
      free(native_name);
      return normal_flow(v_null());
    }
    Value launched =
        start_runtime_thread(callee, native_name, argc, args, n->line);
    free_value(launched);
    free_value(callee);
    free(native_name);
    for (int i = 0; i < argc; i++)
      free_value(args[i]);
    free(args);
    return normal_flow(v_null());
  }
  case N_CLASS: {
    Env *class_env = new_env(env);
    if (n->as.class_stmt.parent) {
      Binding *pb = find_binding(env, n->as.class_stmt.parent);
      if (!pb || pb->value.type != V_CLASS) {
        runtime_error(n->line,
                      "Parent class '%s' is not defined or is not a class.",
                      n->as.class_stmt.parent);
        return normal_flow(v_null());
      }
      for (int pi = 0; pi < pb->value.as.map->count; pi++) {
        Value pk = pb->value.as.map->items[pi].key;
        if (pk.type != V_STRING)
          continue;
        if (strcmp(pk.as.string, "__fluorine_parent") == 0 ||
            strcmp(pk.as.string, "__fluorine_class_name") == 0)
          continue;
        bool overridden = false;
        for (int ci = 0; ci < n->as.class_stmt.body->as.block.count; ci++) {
          Node *member = n->as.class_stmt.body->as.block.items[ci];
          if (member->type == N_FUNCTION &&
              strcmp(member->as.function.name, pk.as.string) == 0)
            overridden = true;
          if (member->type == N_LET &&
              strcmp(member->as.let_stmt.name, pk.as.string) == 0)
            overridden = true;
        }
        if (!overridden)
          define_var(class_env, pk.as.string, pb->value.as.map->items[pi].value,
                     n->line);
      }
    }
    for (int ci = 0; ci < n->as.class_stmt.body->as.block.count; ci++) {
      Flow body = exec(n->as.class_stmt.body->as.block.items[ci], class_env);
      if (body.is_throw || had_error)
        return body;
    }
    Value klass = v_map();
    for (int i = 0; i < class_env->count; i++) {
      Value k = v_string(class_env->bindings[i].name);
      map_set(klass.as.map, k, class_env->bindings[i].value);
      free_value(k);
    }
    Value class_name_key = v_string("__fluorine_class_name"),
          class_name_value = v_string(n->as.class_stmt.name);
    map_set(klass.as.map, class_name_key, class_name_value);
    free_value(class_name_key);
    free_value(class_name_value);
    if (n->as.class_stmt.parent) {
      Value parent_key = v_string("__fluorine_parent"),
            parent_value = v_string(n->as.class_stmt.parent);
      map_set(klass.as.map, parent_key, parent_value);
      free_value(parent_key);
      free_value(parent_value);
    }
    klass.type = V_CLASS;
    define_var(env, n->as.class_stmt.name, klass, n->line);
    return normal_flow(v_null());
  }
  case N_NAMESPACE: {
    Env *ns_env = new_env(env);
    for (int ci = 0;
         ci < n->as.namespace_stmt.body->as.block.count && !had_error; ci++) {
      Flow f = exec(n->as.namespace_stmt.body->as.block.items[ci], ns_env);
      if (f.is_throw || f.is_return || f.is_break || f.is_continue)
        return f;
    }
    Value ns = v_map();
    for (int i = 0; i < ns_env->count; i++) {
      Value k = v_string(ns_env->bindings[i].name);
      map_set(ns.as.map, k, ns_env->bindings[i].value);
      free_value(k);
    }
    define_var(env, n->as.namespace_stmt.name, ns, n->line);
    return normal_flow(v_null());
  }
  case N_TYPEDEF: {
    Binding *source = find_binding(env, n->as.typedef_stmt.target);
    if (!source) {
      static const char *primitive_types[] = {
          "int",  "float", "double", "char",   "bool", "string",
          "void", "long",  "short",  "number", "let",  NULL};
      bool primitive = false;
      for (int ti = 0; primitive_types[ti]; ti++)
        if (strcmp(n->as.typedef_stmt.target, primitive_types[ti]) == 0) {
          primitive = true;
          break;
        }
      if (!primitive) {
        runtime_error(n->line,
                      "Cannot create type alias '%s': '%s' is not a known type "
                      "or existing binding.",
                      n->as.typedef_stmt.name, n->as.typedef_stmt.target);
        return normal_flow(v_null());
      }
      char marker[96];
      snprintf(marker, sizeof(marker), "__fluorine_type:%s",
               n->as.typedef_stmt.target);
      Value type_value = v_string(marker);
      define_var(env, n->as.typedef_stmt.name, type_value, n->line);
      free_value(type_value);
      return normal_flow(v_null());
    }
    /* Fluorine is dynamically typed. A typedef aliases an existing
     * binding without imposing a new runtime representation. */
    define_var(env, n->as.typedef_stmt.name, source->value, n->line);
    return normal_flow(v_null());
  }
  case N_STRUCT: {
    Env *st_env = new_env(env);
    for (int ci = 0; ci < n->as.struct_stmt.body->as.block.count && !had_error;
         ci++) {
      Flow f = exec(n->as.struct_stmt.body->as.block.items[ci], st_env);
      if (f.is_throw || had_error)
        return f;
    }
    Value def = v_map();
    Value fields = v_array();
    for (int i = 0; i < st_env->count; i++) {
      Value k = v_string(st_env->bindings[i].name);
      map_set(def.as.map, k, st_env->bindings[i].value);
      if (st_env->bindings[i].value.type != V_FUNCTION) {
        Array *a = fields.as.array;
        if (a->count == a->capacity) {
          a->capacity = a->capacity ? a->capacity * 2 : 8;
          a->items =
              (Value *)xrealloc(a->items, (size_t)a->capacity * sizeof(Value));
        }
        a->items[a->count++] = copy_value(k);
      }
      free_value(k);
    }
    Value fk = v_string("__fluorine_fields");
    map_set(def.as.map, fk, fields);
    free_value(fk);
    Value sk = v_string("__fluorine_struct");
    map_set(def.as.map, sk, v_bool(true));
    free_value(sk);
    def.type = V_STRUCT_DEF;
    define_var(env, n->as.struct_stmt.name, def, n->line);
    return normal_flow(v_null());
  }
  case N_FUNCTION: {
    FunctionValue *fv = (FunctionValue *)malloc(sizeof(FunctionValue));
    if (!fv)
      exit(2);
    fv->declaration = n;
    fv->closure = env;
    if (function_count == function_capacity) {
      function_capacity =
          function_capacity ? function_capacity * 2 : INITIAL_CAP;
      all_functions = (FunctionValue **)xrealloc(
          all_functions, function_capacity * sizeof(FunctionValue *));
    }
    all_functions[function_count++] = fv;
    Value v;
    v.type = V_FUNCTION;
    v.as.function = fv;
    define_var(env, n->as.function.name, v, n->line);
    return normal_flow(v_null());
  }
  case N_THROW: {
    Value v = eval(n->as.throw_stmt.value, env);
    Flow f = normal_flow(v);
    f.is_throw = true;
    has_exception = true;
    pending_exception = copy_value(v);
    return f;
  }
  case N_TRY: {
    exception_handler_depth++;
    Flow f = exec(n->as.try_stmt.body, env);
    exception_handler_depth--;
    if (f.is_throw || has_exception) {
      if (!f.is_throw) {
        f = normal_flow(copy_value(pending_exception));
        f.is_throw = true;
      }
      if (n->as.try_stmt.handler) {
        Value caught = copy_value(f.value);
        has_exception = false;
        free_value(pending_exception);
        pending_exception = v_null();
        free_value(f.value);
        Env *catch_env = new_env(env);
        if (n->as.try_stmt.name)
          define_var(catch_env, n->as.try_stmt.name, caught, n->line);
        else
          free_value(caught);
        f = exec(n->as.try_stmt.handler, catch_env);
      }
    }
    if (n->as.try_stmt.finally_body) {
      bool saved_has_exception = has_exception;
      Value saved_exception =
          saved_has_exception ? copy_value(pending_exception) : v_null();
      if (saved_has_exception) {
        has_exception = false;
        free_value(pending_exception);
        pending_exception = v_null();
      }
      Flow final_flow = exec(n->as.try_stmt.finally_body, env);
      bool final_abrupt = final_flow.is_return || final_flow.is_break ||
                          final_flow.is_continue || final_flow.is_throw ||
                          has_exception || had_error;
      if (final_abrupt) {
        free_value(f.value);
        free_value(saved_exception);
        return final_flow;
      }
      free_value(final_flow.value);
      if (saved_has_exception) {
        has_exception = true;
        free_value(pending_exception);
        pending_exception = saved_exception;
      }
    }
    return f;
  }
  case N_RETURN: {
    Value v =
        n->as.return_stmt.value ? eval(n->as.return_stmt.value, env) : v_null();
    Flow f = normal_flow(v);
    f.is_return = true;
    return f;
  }
  case N_BREAK: {
    Flow f = normal_flow(v_null());
    f.is_break = true;
    return f;
  }
  case N_CONTINUE: {
    Flow f = normal_flow(v_null());
    f.is_continue = true;
    return f;
  }
  case N_PRINT:
    for (int i = 0; i < n->as.print_stmt.count; i++) {
      Value v = eval(n->as.print_stmt.args[i], env);
      char *s = value_to_string(v);
      fputs(s, stdout);
      if (i + 1 < n->as.print_stmt.count)
        fputc(' ', stdout);
      free(s);
      free_value(v);
    }
    fputc('\n', stdout);
    fflush(stdout);
    return normal_flow(v_null());
  default: {
    Value v = eval(n, env);
    free_value(v);
    return normal_flow(v_null());
  }
  }
}

/* Keep the evaluator aligned with the existing AST representation. The former
 * experimental heap-frame evaluator referenced nonexistent Node fields and was
 * not a complete continuation-based evaluator. */
static Value eval(Node *root, Env *env) { return eval_impl(root, env); }

static const char *debug_source_line(int line) {
  static char linebuf[4096];
  const char *p = debug_source_text;
  int current_line = 1;
  size_t n = 0;
  if (!p || line < 1)
    return "<source unavailable>";
  while (*p && current_line < line) {
    if (*p++ == '\n')
      current_line++;
  }
  while (*p && *p != '\n' && *p != '\r' && n + 1 < sizeof(linebuf))
    linebuf[n++] = *p++;
  linebuf[n] = '\0';
  return linebuf;
}
static const char *debug_node_name(NodeType t) {
  switch (t) {
  case N_EXPR_STMT:
    return "expression";
  case N_LET:
    return "variable declaration";
  case N_ASSIGN:
    return "assignment";
  case N_BLOCK:
    return "block";
  case N_IF:
    return "if";
  case N_WHILE:
    return "while";
  case N_DO_WHILE:
    return "do-while";
  case N_FOR:
    return "for";
  case N_FOR_IN:
    return "for-in";
  case N_SWITCH:
    return "switch";
  case N_FUNCTION:
    return "function declaration";
  case N_CLASS:
    return "class declaration";
  case N_STRUCT:
    return "struct declaration";
  case N_TYPEDEF:
    return "typedef";
  case N_RETURN:
    return "return";
  case N_PRINT:
    return "print";
  case N_IMPORT:
    return "import";
  case N_FLUORINE:
    return "green thread";
  case N_THREAD:
    return "OS thread";
  case N_THROW:
    return "throw";
  case N_TRY:
    return "try";
  default:
    return "instruction";
  }
}
static void debug_dump_env(const char *phase, Env *env) {
  if (!debug_enabled)
    return;
  fprintf(stderr, "    %s variables:\n", phase);
  if (!env) {
    fprintf(stderr, "      <no environment>\n");
    return;
  }
  /* Show innermost bindings first and avoid repeating shadowed names. */
  for (Env *e = env; e; e = e->parent) {
    for (int i = 0; i < e->count; i++) {
      bool shadowed = false;
      for (Env *inner = env; inner != e && !shadowed; inner = inner->parent)
        for (int j = 0; j < inner->count; j++)
          if (strcmp(inner->bindings[j].name, e->bindings[i].name) == 0) {
            shadowed = true;
            break;
          }
      if (shadowed)
        continue;
      char *v = value_to_string(e->bindings[i].value);
      fprintf(stderr, "      %s = %s\n", e->bindings[i].name,
              v ? v : "<unprintable>");
      free(v);
    }
  }
}
static Flow exec(Node *n, Env *env) {
  runtime_lock();
  ActiveEnv active = {env, active_envs};
  active_envs = &active;
  exec_depth++;
  if (debug_enabled && n) {
    fprintf(stderr, "[debug] BEFORE line %d: %s\n      source: %s\n", n->line,
            debug_node_name(n->type), debug_source_line(n->line));
    debug_dump_env("before", env);
  }
  Flow f = exec_impl(n, env);
  if (debug_enabled && n) {
    fprintf(stderr, "[debug] AFTER line %d: %s%s\n      source: %s\n", n->line,
            debug_node_name(n->type), had_error ? " (error)" : "",
            debug_source_line(n->line));
    debug_dump_env("after", env);
    fflush(stderr);
  }
  if (has_exception && !f.is_throw) {
    free_value(f.value);
    f = normal_flow(copy_value(pending_exception));
    f.is_throw = true;
  }
  exec_depth--;
  active_envs = active.next;
  if (exec_depth == 0 && gc_pending && !fluorine_scheduler_running) {
    (void)gc_collect();
    gc_pending = false;
  }
  runtime_unlock();
  if (fluorine_scheduler_running && fluorine_current_task)
    fluorine_task_yield();
  return f;
}

static void fluorine_restore_scheduler_state(void) {
  active_envs = fluorine_scheduler_active_envs;
  exec_depth = fluorine_scheduler_exec_depth;
  exception_handler_depth = fluorine_scheduler_exception_handler_depth;
  has_exception = fluorine_scheduler_has_exception;
  free_value(pending_exception);
  pending_exception = copy_value(fluorine_scheduler_pending_exception);
}
static void fluorine_save_scheduler_state(void) {
  fluorine_scheduler_active_envs = active_envs;
  fluorine_scheduler_exec_depth = exec_depth;
  fluorine_scheduler_exception_handler_depth = exception_handler_depth;
  fluorine_scheduler_has_exception = has_exception;
  free_value(fluorine_scheduler_pending_exception);
  fluorine_scheduler_pending_exception = copy_value(pending_exception);
}
static void fluorine_task_yield(void) {
  FluorineTask *t = fluorine_current_task;
  if (!fluorine_scheduler_running || !t)
    return;
  t->saved_active_envs = active_envs;
  t->saved_exec_depth = exec_depth;
  t->saved_exception_handler_depth = exception_handler_depth;
  t->saved_has_exception = has_exception;
  free_value(t->saved_pending_exception);
  t->saved_pending_exception = copy_value(pending_exception);
#ifdef _WIN32
  SwitchToFiber(fluorine_scheduler_fiber);
#else
  swapcontext(&t->context, &fluorine_scheduler_context);
#endif
  active_envs = t->saved_active_envs;
  exec_depth = t->saved_exec_depth;
  exception_handler_depth = t->saved_exception_handler_depth;
  has_exception = t->saved_has_exception;
  free_value(pending_exception);
  pending_exception = copy_value(t->saved_pending_exception);
}
static void fluorine_task_finish(FluorineTask *t) {
  t->finished = true;
#ifdef _WIN32
  SwitchToFiber(fluorine_scheduler_fiber);
#else
  swapcontext(&t->context, &fluorine_scheduler_context);
#endif
}
static void fluorine_task_entry_common(FluorineTask *t) {
  Value result =
      t->native_name
          ? native_call(t->native_name, t->argc, t->args, t->line)
          : call_value(t->callee, t->argc, t->args, t->line, v_null(), false);
  free_value(result);
  free_value(t->callee);
  t->callee = v_null();
  free(t->native_name);
  t->native_name = NULL;
  for (int i = 0; i < t->argc; i++)
    free_value(t->args[i]);
  free(t->args);
  t->args = NULL;
  t->argc = 0;
  fluorine_task_finish(t);
}
#ifdef _WIN32
static VOID WINAPI fluorine_task_fiber_entry(void *opaque) {
  fluorine_task_entry_common((FluorineTask *)opaque);
}
#else
static void fluorine_task_context_entry(int lo, int hi) {
  uint64_t bits = ((uint64_t)(uint32_t)hi << 32) | (uint32_t)lo;
  fluorine_task_entry_common((FluorineTask *)(uintptr_t)bits);
}
#endif
static bool fluorine_task_initialize(FluorineTask *t) {
  if (t->initialized)
    return true;
#ifdef _WIN32
  if (!fluorine_scheduler_fiber) {
    fluorine_scheduler_fiber = ConvertThreadToFiber(NULL);
    if (!fluorine_scheduler_fiber) {
      runtime_error(t->line, "Could not initialize green-thread scheduler "
                             "(Windows fiber API failed).");
      return false;
    }
  }
  t->fiber = CreateFiber(256 * 1024, fluorine_task_fiber_entry, t);
  if (!t->fiber) {
    runtime_error(t->line, "Could not allocate green-thread stack.");
    return false;
  }
#else
  t->stack = malloc(256 * 1024);
  if (!t->stack) {
    runtime_error(t->line, "Could not allocate green-thread stack.");
    return false;
  }
  if (getcontext(&t->context) != 0) {
    runtime_error(t->line, "Could not initialize green-thread context.");
    free(t->stack);
    t->stack = NULL;
    return false;
  }
  t->context.uc_stack.ss_sp = t->stack;
  t->context.uc_stack.ss_size = 256 * 1024;
  t->context.uc_stack.ss_flags = 0;
  t->context.uc_link = &fluorine_scheduler_context;
  uint64_t bits = (uint64_t)(uintptr_t)t;
  makecontext(&t->context, (void (*)(void))fluorine_task_context_entry, 2,
              (int)(uint32_t)(bits & UINT32_MAX), (int)(uint32_t)(bits >> 32));
#endif
  t->saved_pending_exception = v_null();
  t->initialized = true;
  return true;
}
static void fluorine_task_dispose(FluorineTask *t) {
  FluorineTask **link = &fluorine_all_tasks;
  while (*link && *link != t)
    link = &(*link)->all_next;
  if (*link == t)
    *link = t->all_next;
#ifdef _WIN32
  if (t->fiber)
    DeleteFiber(t->fiber);
#else
  free(t->stack);
#endif
  free_value(t->saved_pending_exception);
  free_value(t->callee);
  free(t->native_name);
  for (int i = 0; i < t->argc; i++)
    free_value(t->args[i]);
  free(t->args);
  free(t);
}
static void run_fluorine_tasks_step(void) {
  if (fluorine_scheduler_running || !fluorine_tasks_head || had_error)
    return;
  FluorineTask *t = fluorine_tasks_head;
  if (!fluorine_task_initialize(t))
    return;
  fluorine_tasks_head = t->next;
  if (!fluorine_tasks_head)
    fluorine_tasks_tail = NULL;
  t->next = NULL;
  fluorine_current_task = t;
  fluorine_scheduler_running = true;
  fluorine_save_scheduler_state();
  active_envs = t->saved_active_envs;
  exec_depth = t->saved_exec_depth;
  exception_handler_depth = t->saved_exception_handler_depth;
  has_exception = t->saved_has_exception;
  free_value(pending_exception);
  pending_exception = copy_value(t->saved_pending_exception);
#ifdef _WIN32
  SwitchToFiber(t->fiber);
#else
  swapcontext(&fluorine_scheduler_context, &t->context);
#endif
  fluorine_restore_scheduler_state();
  fluorine_scheduler_running = false;
  fluorine_current_task = NULL;
  if (t->finished)
    fluorine_task_dispose(t);
  else {
    if (fluorine_tasks_tail)
      fluorine_tasks_tail->next = t;
    else
      fluorine_tasks_head = t;
    fluorine_tasks_tail = t;
  }
  if (!fluorine_tasks_head && gc_pending && !had_error) {
    (void)gc_collect();
    gc_pending = false;
  }
}
static void run_fluorine_tasks(void) {
  while (fluorine_tasks_head && !had_error)
    run_fluorine_tasks_step();
  if (had_error) {
    while (fluorine_tasks_head) {
      FluorineTask *t = fluorine_tasks_head;
      fluorine_tasks_head = t->next;
      fluorine_task_dispose(t);
    }
    fluorine_tasks_tail = NULL;
  }
  free_value(fluorine_scheduler_pending_exception);
  fluorine_scheduler_pending_exception = v_null();
}

static void free_node(Node *n) {
  if (!n)
    return;
  if (n->type == N_NUMBER)
    free(n->number_text);
  switch (n->type) {
  case N_STRING:
    free(n->as.string);
    break;
  case N_VAR:
    free(n->as.var.name);
    break;
  case N_ASSIGN:
    free(n->as.assign.name);
    free_node(n->as.assign.value);
    break;
  case N_ARRAY:
    for (int i = 0; i < n->as.array.count; i++)
      free_node(n->as.array.items[i]);
    free(n->as.array.items);
    break;
  case N_MAP:
    for (int i = 0; i < n->as.map.count; i++) {
      free_node(n->as.map.keys[i]);
      free_node(n->as.map.values[i]);
    }
    free(n->as.map.keys);
    free(n->as.map.values);
    break;
  case N_INDEX:
    free_node(n->as.index.object);
    free_node(n->as.index.index);
    break;
  case N_INDEX_ASSIGN:
    free_node(n->as.index_assign.object);
    free_node(n->as.index_assign.index);
    free_node(n->as.index_assign.value);
    break;
  case N_COMPOUND_INDEX_ASSIGN:
    free_node(n->as.compound_index_assign.object);
    free_node(n->as.compound_index_assign.index);
    free_node(n->as.compound_index_assign.value);
    break;
  case N_IMPORT:
    free(n->as.import_stmt.path);
    free(n->as.import_stmt.alias);
    break;
  case N_UNARY:
    free_node(n->as.unary.right);
    break;
  case N_UPDATE:
    free_node(n->as.update.target);
    break;
  case N_BINARY:
    free_node(n->as.binary.left);
    free_node(n->as.binary.right);
    break;
  case N_CALL:
    free_node(n->as.call.callee);
    for (int i = 0; i < n->as.call.count; i++)
      free_node(n->as.call.args[i]);
    free(n->as.call.args);
    break;
  case N_EXPR_STMT:
    free_node(n->as.expr_stmt.expr);
    break;
  case N_LET:
    free(n->as.let_stmt.name);
    free_node(n->as.let_stmt.init);
    break;
  case N_BLOCK:
    for (int i = 0; i < n->as.block.count; i++)
      free_node(n->as.block.items[i]);
    free(n->as.block.items);
    break;
  case N_IF:
    free_node(n->as.if_stmt.condition);
    free_node(n->as.if_stmt.then_branch);
    free_node(n->as.if_stmt.else_branch);
    break;
  case N_SWITCH:
    free_node(n->as.switch_stmt.expr);
    for (int i = 0; i < n->as.switch_stmt.count; i++) {
      free_node(n->as.switch_stmt.values[i]);
      for (int j = 0; j < n->as.switch_stmt.counts[i]; j++)
        free_node(n->as.switch_stmt.bodies[i][j]);
      free(n->as.switch_stmt.bodies[i]);
    }
    free(n->as.switch_stmt.values);
    free(n->as.switch_stmt.bodies);
    free(n->as.switch_stmt.counts);
    free(n->as.switch_stmt.defaults);
    break;
  case N_WHILE:
  case N_DO_WHILE:
    free_node(n->as.while_stmt.condition);
    free_node(n->as.while_stmt.body);
    break;
  case N_FOR:
    free_node(n->as.for_stmt.init);
    free_node(n->as.for_stmt.condition);
    free_node(n->as.for_stmt.increment);
    free_node(n->as.for_stmt.body);
    break;
  case N_FOR_IN:
    free(n->as.for_in.name);
    free_node(n->as.for_in.iterable);
    free_node(n->as.for_in.body);
    break;
  case N_CLASS:
    free(n->as.class_stmt.name);
    free(n->as.class_stmt.parent);
    free_node(n->as.class_stmt.body);
    break;
  case N_NAMESPACE:
    free(n->as.namespace_stmt.name);
    free_node(n->as.namespace_stmt.body);
    break;
  case N_STRUCT:
    free(n->as.struct_stmt.name);
    free_node(n->as.struct_stmt.body);
    break;
  case N_TYPEDEF:
    free(n->as.typedef_stmt.target);
    free(n->as.typedef_stmt.name);
    break;
  case N_ADDRESS:
  case N_DEREF:
    free_node(n->as.pointer_stmt.target);
    break;
  case N_DEREF_ASSIGN:
    free_node(n->as.deref_assign.pointer);
    free_node(n->as.deref_assign.value);
    break;
  case N_DEREF_COMPOUND_ASSIGN:
    free_node(n->as.deref_compound_assign.pointer);
    free_node(n->as.deref_compound_assign.value);
    break;
  case N_FLUORINE:
  case N_THREAD:
    free_node(n->as.fluorine_stmt.call);
    break;
  case N_FUNCTION:
    free(n->as.function.name);
    free(n->as.function.doc);
    for (int i = 0; i < n->as.function.param_count; i++)
      free(n->as.function.params[i]);
    free(n->as.function.params);
    free_node(n->as.function.body);
    break;
  case N_RETURN:
    free_node(n->as.return_stmt.value);
    break;
  case N_THROW:
    free_node(n->as.throw_stmt.value);
    break;
  case N_TRY:
    free_node(n->as.try_stmt.body);
    free(n->as.try_stmt.name);
    free_node(n->as.try_stmt.handler);
    free_node(n->as.try_stmt.finally_body);
    break;
  case N_PRINT:
    for (int i = 0; i < n->as.print_stmt.count; i++)
      free_node(n->as.print_stmt.args[i]);
    free(n->as.print_stmt.args);
    break;
  default:
    break;
  }
  free(n);
}

static char *read_file(const char *path) {
  enum { MAX_SOURCE_BYTES = 64 * 1024 * 1024 };
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "Cannot open '%s': %s\n", path, strerror(errno));
    return NULL;
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    fprintf(stderr, "Cannot seek in '%s': %s\n", path, strerror(errno));
    fclose(f);
    return NULL;
  }
  long size = ftell(f);
  if (size < 0) {
    fprintf(stderr, "Cannot determine size of '%s'.\n", path);
    fclose(f);
    return NULL;
  }
  if ((uintmax_t)size > (uintmax_t)MAX_SOURCE_BYTES) {
    fprintf(stderr, "Source file '%s' exceeds the 64 MiB limit.\n", path);
    fclose(f);
    return NULL;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Cannot rewind '%s': %s\n", path, strerror(errno));
    fclose(f);
    return NULL;
  }
  size_t expected = (size_t)size;
  char *buffer = (char *)malloc(expected + 1);
  if (!buffer) {
    fprintf(stderr, "Out of memory while reading '%s'.\n", path);
    fclose(f);
    exit(2);
  }
  size_t got = fread(buffer, 1, expected, f);
  if (got != expected) {
    if (ferror(f))
      fprintf(stderr, "Error reading '%s': %s\n", path, strerror(errno));
    else
      fprintf(stderr, "Unexpected end of file while reading '%s'.\n", path);
    free(buffer);
    fclose(f);
    return NULL;
  }
  if (fclose(f) != 0) {
    fprintf(stderr, "Error closing '%s' after reading.\n", path);
    free(buffer);
    return NULL;
  }
  buffer[got] = '\0';

  /* Source files are normalized to UTF-8 before lexing. A UTF-8 BOM is
     metadata, not source text; UTF-16/UTF-32 BOMs identify their byte order
     unambiguously and are converted using the same platform encoding layer
     as read_text(). Files without a BOM are treated as UTF-8 (the standard
     portable source encoding). Legacy single-byte encodings are ambiguous
     without a BOM and should be converted to UTF-8 before execution. */
  size_t off = 0;
  const char *encoding = "UTF-8";
  if (got >= 4 && (unsigned char)buffer[0] == 0xFF &&
      (unsigned char)buffer[1] == 0xFE && buffer[2] == 0 && buffer[3] == 0) {
    encoding = "UTF-32LE";
    off = 4;
  } else if (got >= 4 && buffer[0] == 0 && buffer[1] == 0 &&
             (unsigned char)buffer[2] == 0xFE &&
             (unsigned char)buffer[3] == 0xFF) {
    encoding = "UTF-32BE";
    off = 4;
  } else if (got >= 3 && (unsigned char)buffer[0] == 0xEF &&
             (unsigned char)buffer[1] == 0xBB &&
             (unsigned char)buffer[2] == 0xBF) {
    off = 3;
  } else if (got >= 2 && (unsigned char)buffer[0] == 0xFF &&
             (unsigned char)buffer[1] == 0xFE) {
    encoding = "UTF-16LE";
    off = 2;
  } else if (got >= 2 && (unsigned char)buffer[0] == 0xFE &&
             (unsigned char)buffer[1] == 0xFF) {
    encoding = "UTF-16BE";
    off = 2;
  }
  if (off || strcmp(encoding, "UTF-8") != 0) {
    char *utf8 = NULL;
    size_t utf8_len = 0;
    if (!convert_encoding(buffer + off, got - off, encoding_for_iconv(encoding),
                          "UTF-8", &utf8, &utf8_len) ||
        memchr(utf8, 0, utf8_len) != NULL) {
      fprintf(stderr,
              "Cannot decode source '%s': invalid %s text. Save it as UTF-8 or "
              "use a valid BOM-marked UTF-16/UTF-32 file.\n",
              path, encoding);
      free(utf8);
      free(buffer);
      return NULL;
    }
    char *terminated = (char *)realloc(utf8, utf8_len + 1);
    if (!terminated) {
      free(utf8);
      free(buffer);
      fprintf(stderr, "Out of memory while decoding '%s'.\n", path);
      exit(2);
    }
    terminated[utf8_len] = '\0';
    free(buffer);
    buffer = terminated;
  }
  return buffer;
}

static bool safe_relative_path(const char *path) {
  if (!path || !*path || path[0] == '/' || path[0] == '\\' || strchr(path, ':'))
    return false;
  const char *p = path;
  while (*p) {
    while (*p == '/' || *p == '\\')
      p++;
    const char *start = p;
    while (*p && *p != '/' && *p != '\\')
      p++;
    if ((size_t)(p - start) == 2 && start[0] == '.' && start[1] == '.')
      return false;
  }
  return true;
}

static Value run_import(const char *path, Env *env, int line, const char *alias,
                        bool quoted) {
  if (import_depth >= 32) {
    runtime_error(line, "Import nesting exceeds 32 levels.");
    return v_null();
  }
  if (!safe_relative_path(path)) {
    runtime_error(
        line,
        "Import path must be relative and must not escape its base directory.");
    return v_null();
  }
  char resolved[4096];
  char module_path[4096];
  char local_path[4096];
  const char *base = path;
  const char *slash1 = strrchr(path, '/'), *slash2 = strrchr(path, '\\');
  if (slash1 && slash1 + 1 > base)
    base = slash1 + 1;
  if (slash2 && slash2 + 1 > base)
    base = slash2 + 1;
  bool has_extension = (strrchr(base, '.') != NULL);
  const char *requested_path = path;
  bool found = false;

  /* Quoted import paths are file-oriented. Resolve them against the process
   * working directory first, adding .f for extensionless module names. */
  if (quoted) {
    int written;
    if (!has_extension)
      written = snprintf(local_path, sizeof(local_path), "%s.f", path);
    else
      written = snprintf(local_path, sizeof(local_path), "%s", path);
    if (written < 0 || (size_t)written >= sizeof(local_path)) {
      runtime_error(line, "Import path is too long.");
      return v_null();
    }
    if (file_is_readable(local_path)) {
      if (strlen(local_path) >= sizeof(resolved)) {
        runtime_error(line, "Import path is too long.");
        return v_null();
      }
      strcpy(resolved, local_path);
      found = true;
    }
  }

  /* Quoted imports search cwd first, then the standard library. Unquoted
   * imports are always stdlib-relative, including names containing slashes
   * or an explicit .f extension; they must never resolve as cwd files. */
  if (!found) {
    int written = snprintf(module_path, sizeof(module_path), "stdlib/%s%s",
                           path, has_extension ? "" : ".f");
    if (written < 0 || (size_t)written >= sizeof(module_path)) {
      runtime_error(line, quoted ? "Module path is too long."
                                 : "Module name is too long.");
      return v_null();
    }
    requested_path = module_path;
    if (!resolve_import_path(resolved, sizeof(resolved), requested_path)) {
      runtime_error(line, "Import path is too long.");
      return v_null();
    }
    if (!file_is_readable(resolved)) {
      runtime_error(line, "Cannot find import '%s'%s.", path,
                    quoted ? " in the current directory or standard library"
                           : " in the standard library");
      return v_null();
    }
  }
  if (alias) {
    for (size_t i = 0; i < isolated_module_count; i++)
      if (strcmp(isolated_module_paths[i], resolved) == 0) {
        Value cached;
        cached.type = V_MAP;
        cached.as.map = isolated_module_maps[i];
        return cached;
      }
  } else {
    for (size_t i = 0; i < imported_count; i++)
      if (strcmp(imported_paths[i], resolved) == 0)
        return v_null();
  }
  if (imported_count >= 256 || isolated_module_count >= 256) {
    runtime_error(line, "Too many imported modules.");
    return v_null();
  }
  char *source = read_file(resolved);
  if (!source) {
    had_error = 1;
    return v_null();
  }
  if (!alias)
    strcpy(imported_paths[imported_count++], resolved);
  import_depth++;
  Env *target = alias ? new_env(NULL) : env;
  Lexer lex = {0};
  lex.src = source;
  lex.line = 1;
  lex_source(&lex);
  if (!had_error) {
    Parser p = {0};
    p.tokens = lex.tokens;
    p.count = lex.count;
    Node *program = new_node(N_BLOCK, 1);
    program->as.block.items = NULL;
    program->as.block.count = 0;
    while (!check(&p, T_EOF) && !had_error) {
      Node *item = parse_declaration(&p);
      program->as.block.items = (Node **)xrealloc(
          program->as.block.items,
          (size_t)(program->as.block.count + 1) * sizeof(Node *));
      program->as.block.items[program->as.block.count++] = item;
    }
    if (!had_error) {
      for (int i = 0; i < program->as.block.count && !had_error; i++) {
        Flow f = exec(program->as.block.items[i], target);
        if (f.is_throw) {
          runtime_error(line, "Uncaught exception in imported module.");
          free_value(f.value);
          break;
        }
      }
    }
    if (imported_program_count == imported_program_capacity) {
      imported_program_capacity =
          imported_program_capacity ? imported_program_capacity * 2 : 8;
      imported_programs = (Node **)xrealloc(
          imported_programs, imported_program_capacity * sizeof(Node *));
    }
    imported_programs[imported_program_count++] = program;
  }
  for (size_t i = 0; i < lex.count; i++)
    free(lex.tokens[i].lexeme);
  free(lex.tokens);
  free(source);
  import_depth--;
  if (alias) {
    Value module = v_map();
    if (!had_error) {
      for (int i = 0; i < target->count; i++) {
        Value k = v_string(target->bindings[i].name);
        map_set(module.as.map, k, target->bindings[i].value);
        free_value(k);
      }
    }
    strcpy(isolated_module_paths[isolated_module_count], resolved);
    isolated_module_maps[isolated_module_count++] = module.as.map;
    return module;
  }
  return v_null();
}
static void cleanup_envs(void) {
  while (fluorine_tasks_head) {
    FluorineTask *task = fluorine_tasks_head;
    fluorine_tasks_head = task->next;
    fluorine_task_dispose(task);
  }
  fluorine_tasks_tail = NULL;
  global_env = NULL;
  active_envs = NULL;
  if (has_exception) {
    free_value(pending_exception);
    has_exception = false;
  }

  // Safely unlink and free environments
  runtime_lock();
  while (env_list) {
    EnvLink *link = env_list;
    env_list = link->next;
    Env *e = link->env;
    if (e) {
      for (int i = 0; i < e->count; i++) {
        free(e->bindings[i].name);
        free_value(e->bindings[i].value);
      }
      free(e->bindings);
      free(e);
    }
    free(link);
  }
  all_envs = NULL;

  // Safely unlink and free arrays
  while (all_arrays) {
    Array *a = all_arrays;
    all_arrays = a->next;
    for (int i = 0; i < a->count; i++)
      free_value(a->items[i]);
    free(a->items);
    free(a);
  }

  // Safely unlink and free maps
  while (all_maps) {
    Map *m = all_maps;
    all_maps = m->next;
    for (int i = 0; i < m->count; i++) {
      free_value(m->items[i].key);
      free_value(m->items[i].value);
    }
    free(m->items);
    free(m);
  }

  // Safely unlink and free safe pointers
  while (all_pointers) {
    SafePointer *p = all_pointers;
    all_pointers = p->next;
    free(p->name);
    free_value(p->key);
    free(p);
  }
  runtime_unlock();
}
static void usage(const char *argv0) {
  fprintf(stderr,
          "Fluorine language interpreter 0.6\nUsage: %s <script.f> "
          "[arguments...]\n       %s -c <source>\n       %s --version\n",
          argv0, argv0, argv0);
  fprintf(stderr,
          "Script globals: argv (script path plus arguments), args (arguments "
          "after the script path), argc (number of script arguments).\n");
  fprintf(stderr, "Automatic semicolon insertion (ASI) is enabled by default; "
                  "use --disable-asi to require explicit semicolons.\n");
  fprintf(stderr,
          "Options: --enable-asi, --disable-asi, --debug, --version, --help, "
          "-c <source>. Script arguments follow the script path.\n");
}

/* Install script arguments as ordinary language globals before evaluating a
 * file. */
static void set_script_arguments(int argc, char **argv) {
  if (!global_env)
    global_env = new_env(NULL);
  Value full = v_array();
  Value extras = v_array();
  for (int i = 1; i < argc; i++) {
    Value item = v_string(argv[i]);
    Array *all = full.as.array;
    if (all->count == all->capacity) {
      all->capacity = all->capacity ? all->capacity * 2 : 8;
      all->items =
          (Value *)xrealloc(all->items, (size_t)all->capacity * sizeof(Value));
    }
    all->items[all->count++] = copy_value(item);
    if (i >= 2) {
      Array *only = extras.as.array;
      if (only->count == only->capacity) {
        only->capacity = only->capacity ? only->capacity * 2 : 8;
        only->items = (Value *)xrealloc(only->items,
                                        (size_t)only->capacity * sizeof(Value));
      }
      only->items[only->count++] = copy_value(item);
    }
    free_value(item);
  }
  (void)define_var(global_env, "argv", full, 1);
  (void)define_var(global_env, "args", extras, 1);
  (void)define_var(global_env, "argc",
                   v_num(argc >= 2 ? (double)(argc - 2) : 0), 1);
  free_value(full);
  free_value(extras);
}

static int execute_source_text(const char *source) {
  debug_source_text = source;
  /* -c and REPL callers may also supply UTF-8 text with a leading BOM. */
  if (source && (unsigned char)source[0] == 0xEF &&
      (unsigned char)source[1] == 0xBB && (unsigned char)source[2] == 0xBF)
    source += 3;
  Lexer lex = {0};
  lex.src = source;
  lex.line = 1;
  lex_source(&lex);
  if (!had_error) {
    Parser p = {0};
    p.tokens = lex.tokens;
    p.count = lex.count;
    Node *program = new_node(N_BLOCK, 1);
    program->as.block.items = NULL;
    program->as.block.count = 0;
    while (!check(&p, T_EOF) && !had_error) {
      Node *n = parse_declaration(&p);
      program->as.block.items = (Node **)xrealloc(
          program->as.block.items,
          (size_t)(program->as.block.count + 1) * sizeof(Node *));
      program->as.block.items[program->as.block.count++] = n;
    }
    if (!had_error) {
      /* FunctionValue stores a pointer to its declaration Node. Keep the
         owning AST alive for the lifetime of the interpreter; this is
         essential for definitions entered on earlier REPL lines and for
         functions launched later by either thread keyword. */
      if (imported_program_count == imported_program_capacity) {
        imported_program_capacity =
            imported_program_capacity ? imported_program_capacity * 2 : 8;
        imported_programs = (Node **)xrealloc(
            imported_programs, imported_program_capacity * sizeof(Node *));
      }
      imported_programs[imported_program_count++] = program;
      Env *global = global_env;
      if (!global) {
        global = new_env(NULL);
        global_env = global;
      }
      for (int i = 0; i < program->as.block.count && !had_error; i++) {
        Flow result = exec(program->as.block.items[i], global);
        if (result.is_throw) {
          char *msg = value_to_string(result.value);
          fprintf(stderr, "Uncaught exception: %s\n", msg);
          free(msg);
          had_error = 1;
          free_value(result.value);
        } else if (result.is_return || result.is_break || result.is_continue)
          runtime_error(
              program->as.block.items[i]->line,
              "Control-flow statement used outside its valid context.");
        if (!had_error)
          run_fluorine_tasks_step();
      }
      run_fluorine_tasks();
      join_all_threads();
    } else {
      free_node(program);
    }
  }
  for (size_t i = 0; i < lex.count; i++)
    free(lex.tokens[i].lexeme);
  free(lex.tokens);
  return had_error ? 1 : 0;
}
static void repl(void) {
  char line[65536];
  puts("Fluorine 0.1 interactive interpreter. Type :quit to exit.");
  for (;;) {
    fputs(">>> ", stdout);
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin))
      break;
    size_t n = strlen(line);
    while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
      line[--n] = '\0';
    if (strcmp(line, ":quit") == 0 || strcmp(line, ":q") == 0 ||
        strcmp(line, "exit()") == 0)
      break;
    if (!*line)
      continue;
    had_error = 0;
    (void)execute_source_text(line);
    if (had_error)
      had_error = 0;
  }
}

int main(int argc, char **argv) {
  fluorine_argc = argc;
  fluorine_argv = argv;
  if (!set_executable_directory(argv[0])) {
    fprintf(stderr, "Error: could not determine the Fluorine executable's "
                    "directory; refusing cwd-relative bundle lookup.\n");
    return 2;
  }
  char **args = (char **)calloc((size_t)argc + 1, sizeof(char *));
  if (!args) {
    fprintf(stderr, "Out of memory\n");
    return 2;
  }
  int nargc = 1;
  args[0] = argv[0];
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--enable-asi") == 0) {
      asi_enabled = true;
      continue;
    }
    if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
      debug_enabled = true;
      continue;
    }
    if (strcmp(argv[i], "--disable-asi") == 0) {
      asi_enabled = false;
      continue;
    }
    args[nargc++] = argv[i];
  }
  if (nargc == 2 && strcmp(args[1], "--version") == 0) {
    puts("Fluorine 0.1");
    free(args);
    return 0;
  }
  if (nargc == 2 &&
      (strcmp(args[1], "--help") == 0 || strcmp(args[1], "-h") == 0)) {
    usage(args[0]);
    free(args);
    return 0;
  }
  if (nargc == 1) {
    repl();
  } else if (nargc >= 3 && strcmp(args[1], "-c") == 0) {
    set_script_arguments(nargc - 1, args + 1);
    (void)execute_source_text(args[2]);
  } else if (nargc >= 2) {
    char *source = read_file(args[1]);
    if (!source) {
      fprintf(stderr, "Error: could not read script '%s'.\n", args[1]);
      free(args);
      return 2;
    }
    set_script_arguments(nargc, args);
    (void)execute_source_text(source);
    free(source);
  } else {
    usage(args[0]);
    free(args);
    return 2;
  }
  free(args);
  join_all_threads();
  for (size_t i = 0; i < imported_program_count; i++)
    free_node(imported_programs[i]);
  free(imported_programs);
  for (size_t i = 0; i < function_count; i++)
    free(all_functions[i]);
  free(all_functions);
  while (all_threads) {
    FluorineThread *t = all_threads;
    all_threads = t->next;
    if (t->args) {
      for (int i = 0; i < t->argc; i++)
        free_value(t->args[i]);
      free(t->args);
    }
    free_value(t->callee);
    free_value(t->result);
    free(t);
  }
  cleanup_envs();
  return had_error ? 1 : 0;
}
