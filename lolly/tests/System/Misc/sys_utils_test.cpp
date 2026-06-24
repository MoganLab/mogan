#include "a_lolly_test.hpp"
#include "sys_utils.hpp"

TEST_MEMORY_LEAK_INIT

TEST_CASE ("get_process_id") { CHECK (get_process_id () >= 1); }

TEST_CASE ("get_env/set_env") {
#if defined(OS_MINGW) || defined(OS_WIN)
  CHECK_EQ (get_env ("ProgramFiles") == "C:\\Program Files", true);
  set_env ("LOLLY_PATH", "C:\\lolly");
  CHECK_EQ (get_env ("LOLLY_PATH") == "C:\\lolly", true);
#endif

  set_env ("SHELL", "/bin/zsh");
  CHECK_EQ (get_env ("SHELL") == "/bin/zsh", true);
}

TEST_CASE ("get_user_name") {
  CHECK (N (get_user_name ()) >= 0);
  // cout << get_user_name () << LF;
}

TEST_CASE ("get_stacktrace") {
  CHECK (N (lolly::get_stacktrace (10)) >= 0);
  cout << lolly::get_stacktrace (10) << LF;
}

TEST_MEMORY_LEAK_ALL
