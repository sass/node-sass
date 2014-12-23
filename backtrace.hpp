#define SASS_BACKTRACE

#include <sstream>

#ifndef SASS_POSITION
#include "position.hpp"
#endif

#ifndef SASS_FILE
#include "file.hpp"
#endif

namespace Sass {

  using namespace std;

  struct Backtrace {

    Backtrace* parent;
    string     path;
    Position   position;
    string     caller;

    Backtrace(Backtrace* prn, string pth, Position position, string c)
    : parent(prn),
      path(pth),
      position(position),
      caller(c)
    { }

    string to_string(bool warning = false)
    {
      size_t i = -1;
      stringstream ss;
      string cwd(Sass::File::get_cwd());
      Backtrace* this_point = this;

      if (!warning) ss << endl << "Backtrace:";
      // the first tracepoint (which is parent-less) is an empty placeholder
      while (this_point->parent) {

        // make path relative to the current directory
        string rel_path(Sass::File::resolve_relative_path(this_point->path, cwd, cwd));

        if (warning) {
          ss << endl
             << "\t"
             << (++i == 0 ? "on" : "from")
             << " line "
             << this_point->position.line
             << " of "
             << rel_path;
        } else {
          ss << endl
             << "\t"
             << rel_path
             << ":"
             << this_point->position.line
             << this_point->parent->caller;
        }

        this_point = this_point->parent;
      }

      return ss.str();
    }

    size_t depth()
    {
      size_t d = 0;
      Backtrace* p = parent;
      while (p) {
        ++d;
        p = p->parent;
      }
      return d-1;
    }

  };

}
