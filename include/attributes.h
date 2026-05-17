#ifndef MAZECHAIN_ATTRIBUTES_H
#define MAZECHAIN_ATTRIBUTES_H

#if defined(__GNUC__) || defined(__clang__)
  #define NODISCARD [[nodiscard]]
  #define NO_RETURN [[noreturn]]
  #define UNUSED [[maybe_unused]]
#else
  #define NODISCARD
  #define NO_RETURN
  #define UNUSED
#endif

#endif // MAZECHAIN_ATTRIBUTES_H
