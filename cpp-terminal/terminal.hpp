#pragma once

#include "cpp-terminal/options.hpp"
#include "cpp-terminal/terminfo.hpp"

#include <cstdio>
#include <string>

namespace Term
{

/* Note: the code that uses Terminal must be inside try/catch block, otherwise
 * the destructors will not be called when an exception happens and the
 * terminal will not be left in a good state. Terminal uses exceptions when
 * something goes wrong.
 */

class Terminal
{
public:
  Terminal(const std::vector<Term::Options::Option>& options = {});
  ~Terminal();

private:
  FILE*          m_stdin{nullptr};
  FILE*          m_stdout{nullptr};
  FILE*          m_stderr{nullptr};
  void           store_and_restore();
  void           setRawMode();
  void           attachConsole();
  void           detachConsole();
  bool           has_allocated_console{false};
  Term::Terminfo m_terminfo;
  Term::Options  m_options;
};

// change the title of the terminal, only supported by a few terminals
std::string terminal_title(const std::string& title);
// clear the screen and the scroll-back buffer
std::string clear_buffer();

}  // namespace Term
