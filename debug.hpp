#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 1

#ifdef DEBUG
#define DEBUG_PRINT(x) do { std::cerr << x; } while (0);
#define DEBUG_PRINTLN(x) do { std::cerr << x << std::endl; } while (0);
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#endif
