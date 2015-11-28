set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable(trivial-elim
  trivial-elim.cpp
)

target_link_libraries(trivial-elim
  clangTooling
  clangBasic
  clangAST
)