#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include "parser.hpp"

char *file_content;
int ptr;

// #define debug main
#define start main

int debug(int argc, char **argv) {

  std::cout << "[DEBUG]:" << std::endl;
  return 0;
}

int start(int argc, char **argv) {

  if (argc < 2) {
    std::cerr << "Invalid usage" << std::endl;
    return 1;
  } 

  char *filename = argv[1];
  int fd = open(filename, O_RDONLY);

  if (fd == -1) {
    std::cerr << "No file found" << std::endl;
    return 1;
  }

  file_content = ::new char[2 << 10];
  int char_len = read(fd, file_content, 2 << 10);
  ptr = 0;

  // install the binary operation precedence
  parser::binary_op_precedence['<'] = 10;
  parser::binary_op_precedence['-'] = 20;
  parser::binary_op_precedence['+'] = 30;
  parser::binary_op_precedence['*'] = 40;

  ast::initialise_module();

  parser::get_next_token();
  while (ptr < char_len) {
    switch(parser::curr_token) {
      case tok_eof:
        break;
      case ';':
        parser::get_next_token();
        break;
      case '\n':
        parser::get_next_token();
        break;
      case tok_def:
        parser::handle_definition();
        break;
      case tok_extern:
        parser::handle_extern();
        break;
      default: 
        parser::handle_top_level_expr();
        break;
    }
  }  

  return 0;
}


