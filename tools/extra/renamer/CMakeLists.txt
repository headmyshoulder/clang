set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable( renamer
  Renamer.cpp
  )
target_link_libraries( renamer
  clangTooling
  clangBasic
  clangASTMatchers
  )