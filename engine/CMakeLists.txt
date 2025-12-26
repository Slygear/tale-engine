add_library(tale_engine STATIC
  src/diagnostics.cpp
  src/dsl/lexer.cpp
  src/dsl/parser.cpp
  src/runtime/state.cpp
  src/runtime/interpreter.cpp
 "include/tale_engine/dsl/token.h" "include/tale_engine/dsl/lexer.h" "include/tale_engine/dsl/ast.h" "include/tale_engine/dsl/parser.h" "src/dsl/lexer.cpp" "src/dsl/parser.cpp")

target_include_directories(tale_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MSVC)
  target_compile_options(tale_engine PRIVATE /W4 /permissive-)
else()
  target_compile_options(tale_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
