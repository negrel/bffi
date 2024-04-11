#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define XSTD_IMPLEMENTATION
#include "xstd.h"

typedef enum {
  TK_INC = '+',
  TK_DEC = '-',
  TK_LEFT = '<',
  TK_RIGHT = '>',
  TK_OUTPUT = '.',
  TK_INPUT = ',',
  TK_JUMP_IF_ZERO = '[',
  TK_JUMP_IF_NONZERO = ']',
  TK_EOF = '\0',
} BffiToken;

typedef struct {
  BffiToken token;
  size_t operand;
} BffiOp;

typedef struct {
  Reader *reader;
  char current;
  size_t cursor;
} BffiLexer;

void bffi_lexer_next_char(BffiLexer *lexer);
void bffi_lexer_next_char(BffiLexer *lexer) {
  size_t read = 0;
  int error = 0;
  reader_read(lexer->reader, (uint8_t *)&lexer->current, sizeof(char), &read,
              &error);

  if (error != 0) {
    fprintf(stderr, "failed to read: %s\n", strerror(error));
    exit(1);
  }

  if (read == 0) {
    lexer->current = '\0';
  }
}

BffiToken bffi_lexer_lex(BffiLexer *lexer);
BffiToken bffi_lexer_lex(BffiLexer *lexer) {
  bffi_lexer_next_char(lexer);

  // Skip comment line.
  if (lexer->current == '#')
    do {
      bffi_lexer_next_char(lexer);
    } while (lexer->current != '\n' || lexer->current != '\0');

  if (strchr("+-<>,.[]", lexer->current) != NULL) {
    return (BffiToken)lexer->current;
  }

  return TK_EOF;
}

void bffi_lex(Reader *r, BytesBuffer *buffer);
void bffi_lex(Reader *r, BytesBuffer *buffer) {
  bytes_buffer_reset(buffer);

  BffiLexer lexer = {
      .reader = r,
  };

  size_t *addrs = vec_new(g_libc_allocator, 1, sizeof(size_t));

  BffiOp op = {0};
  BffiToken tk = TK_EOF;
  size_t depth = 0;

  while ((tk = bffi_lexer_lex(&lexer)) != TK_EOF) {
    if (tk == op.token) {
      op.operand++;
      continue;
    } else if (op.token != TK_EOF) {
      bytes_buffer_append(buffer, &op);
    }

    switch (tk) {
    case TK_LEFT:
    case TK_RIGHT:
    case TK_OUTPUT:
    case TK_INPUT:
    case TK_DEC:
    case TK_INC:
      op.token = tk;
      op.operand = 1;
      break;

    case TK_JUMP_IF_ZERO: {
      depth++;
      op.token = tk;
      op.operand = 0;
      size_t addr = bytes_buffer_length(buffer) / sizeof(BffiOp);
      *vec_push(&addrs) = addr;
      break;
    }

    case TK_JUMP_IF_NONZERO: {
      if (depth == 0) {
        fprintf(stderr, "unbalanced []\n");
        exit(1);
      }

      depth--;
      size_t addr = 0;
      vec_pop(addrs, &addr);

      op.token = tk;
      op.operand = addr + 1;

      bytes_buffer_get_ptr(buffer, addr, BffiOp)->operand =
          bytes_buffer_length(buffer) / sizeof(BffiOp) + 1;

      break;
    }

    case TK_EOF:
      assert(0);
      break;
    }
  }
  bytes_buffer_append(buffer, &op);

  vec_free(addrs);

  if (depth > 0) {
    fprintf(stderr, "unbalanced [] %lu\n", depth);
    exit(1);
  }
}

void bffi_interpret(BytesBuffer *buffer);
void bffi_interpret(BytesBuffer *buffer) {
  size_t ops = bytes_buffer_length(buffer) / sizeof(BffiOp);

  BytesBuffer memory = bytes_buffer(g_libc_allocator);
  bytes_buffer_resize(&memory, 4096);
  bytes_buffer_fill_available(&memory, '\0');

  size_t head = 0;
  size_t ip = 0;
  while (ip < ops) {
    BffiOp op = bytes_buffer_get(buffer, ip, BffiOp);

    switch (op.token) {
    case TK_INC: {
      uint8_t *ptr = bytes_buffer_get_ptr(&memory, head, uint8_t);
      (*ptr) += op.operand;
      ip++;
      break;
    }

    case TK_DEC: {
      uint8_t *ptr = bytes_buffer_get_ptr(&memory, head, uint8_t);
      (*ptr) -= op.operand;
      ip++;
      break;
    }

    case TK_LEFT: {
      head -= op.operand;
      ip++;
      break;
    }

    case TK_RIGHT: {
      head += op.operand;
      ip++;
      break;
    }

    case TK_OUTPUT: {
      for (size_t i = 0; i < op.operand; i++) {
        fwrite(bytes_buffer_get_ptr(&memory, head, uint8_t), sizeof(uint8_t), 1,
               stdout);
      }
      ip++;
      break;
    }
    case TK_INPUT: {
      for (size_t i = 0; i < op.operand; i++) {
        size_t read = fread(bytes_buffer_get_ptr(&memory, head, uint8_t),
                            sizeof(uint8_t), 1, stdin);
        (void)read;
      }
      ip++;
      break;
    }

    case TK_JUMP_IF_ZERO: {
      if (bytes_buffer_get(&memory, head, uint8_t) == 0) {
        ip = op.operand;
      } else {
        ip++;
      }
      break;
    }

    case TK_JUMP_IF_NONZERO: {
      if (bytes_buffer_get(&memory, head, uint8_t) != 0) {
        ip = op.operand;
      } else {
        ip++;
      }
      break;
    }

    default:
      assert(0);
    }
  }

  bytes_buffer_deinit(&memory);
}

int main(int argc, char **argv) {
  // Skip program name.
  argc--;
  argv++;

  while (argc > 0) {
    const char *fpath = argv[0];

    // Shift args.
    argc--;
    argv++;

    FILE *f = fopen(fpath, "r");
    if (f == NULL) {
      fprintf(stderr, "failed to open file '%s'\n", fpath);
      return 1;
    }

    FileReader freader = file_reader(f);

    BytesBuffer buffer = bytes_buffer(g_libc_allocator);
    bffi_lex(&freader.reader, &buffer);
    bffi_interpret(&buffer);
    bytes_buffer_deinit(&buffer);
  }

  return 0;
}
