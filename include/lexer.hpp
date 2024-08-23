// token holder enumeration
enum Token {
  tok_eof = -1,
  tok_def = -2,
  tok_extern = -3,
  tok_identifier = -4,
  tok_number = -5,
  tok_if = -6,
  tok_then = -7,
  tok_else = -8
};

namespace lexer {

// stores the variable name if tok_identifier
static std::string identifier_str; 

// stores the number value if tok_number
static double num_value; 

// returns the next token from the start of the pointer
static int get_token(char *file_content, int& ptr) {

  // process white space
  while (file_content[ptr] == ' ' || file_content[ptr] == '\n') {
    ++ptr; // consume all white and new line characters
  }

  // comment
  if (file_content[ptr] == '#') {
    ++ptr;
    while (file_content[ptr] != '\n' && 
        file_content[ptr] != '\0' && 
        file_content[ptr] != '\r') {

      ++ptr;

    }
    if (file_content[ptr] != '\0') {
      ++ptr;
      return get_token(file_content, ptr);
    }
  }

  // identifier
  if (isalpha(file_content[ptr])) {
    identifier_str = "" ;
    identifier_str += file_content[ptr];
    ++ptr;
    while (isalnum(file_content[ptr])) {
      identifier_str += file_content[ptr++];
    }

    if (identifier_str == "def") {
      return tok_def;
    }else if (identifier_str == "extern") {
      return tok_extern;
    }else if (identifier_str == "if") {
      return tok_if;
    }else if (identifier_str == "then") {
      return tok_then;
    }else if (identifier_str == "else") {
      return tok_else;
    }

    return tok_identifier;
  }

  // number
  if (isdigit(file_content[ptr]) || file_content[ptr] == '.') {
    std::string num_str = "";
    num_str += file_content[ptr];
    ++ptr;
    while (isdigit(file_content[ptr]) || file_content[ptr] == '.') {
      num_str += file_content[ptr++];
    }
    num_value = std::strtod(num_str.c_str(), 0);
    return tok_number;
  }
  
  // EOF
  if (file_content[ptr] == '\0') {  
    return tok_eof;
  }

  return file_content[ptr++];
}

};
