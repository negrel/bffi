// XSTD bundle from https://github.com/negrel/xstd.h

#ifdef __clang__
#define typeof __typeof__
#else
#ifndef typeof
#define typeof assert(0 && "typeof macro is not defined")
#endif
#endif

#define type_assert_eq(X, Y)                                                   \
  _Generic((Y), typeof(X): _Generic((X), typeof(Y): (void)NULL))
// Single file header allocator interface / vtable
// from xstd (https://github.com/negrel/xstd.h).

#ifndef XSTD_ALLOC_H_INCLUDE
#define XSTD_ALLOC_H_INCLUDE

#include <stddef.h>

#ifdef XSTD_IMPLEMENTATION
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#endif

struct xstd_allocator_vtable {
  void *(*malloc)(void *allocator, size_t size);
  void (*free)(void *allocator, void *ptr);
  void *(*calloc)(void *allocator, size_t nmemb, size_t size);
  void *(*realloc)(void *allocator, void *ptr, size_t newsize);
};

// Allocator interface.
typedef struct {
  struct xstd_allocator_vtable *vtable_;
  size_t offset_;
} Allocator;

// alloc_malloc() allocates size bytes and returns a pointer to the allocated
// memory. The memory is not initialized.
void *alloc_malloc(Allocator *, size_t);

#ifdef XSTD_IMPLEMENTATION
void *alloc_malloc(Allocator *allocator, size_t size) {
  void *ptr = allocator->vtable_->malloc(
      (void *)((uintptr_t)allocator + allocator->offset_), size);
  assert(ptr != NULL);
  return ptr;
}
#endif

// alloc_free() frees the memory space pointed to by ptr, which must have been
// returned by a previous call to malloc() or related functions.
void alloc_free(Allocator *allocator, void *ptr);

#ifdef XSTD_IMPLEMENTATION
void alloc_free(Allocator *allocator, void *ptr) {
  allocator->vtable_->free((void *)((uintptr_t)allocator + allocator->offset_),
                           ptr);
}
#endif

// The calloc() function allocates memory for an array of nmemb elements of size
// bytes each and returns a pointer to the allocated memory. The memory is set
// to zero.
void *alloc_calloc(Allocator *allocator, size_t nmemb, size_t size);

#ifdef XSTD_IMPLEMENTATION
void *alloc_calloc(Allocator *allocator, size_t nmemb, size_t size) {
  void *ptr = allocator->vtable_->calloc(
      (void *)((uintptr_t)allocator + allocator->offset_), nmemb, size);
  assert(ptr != NULL);
  return ptr;
}
#endif

// The realloc() function changes the size of the memory block pointed to by ptr
// to newsize bytes. The contents of the memory will be unchanged in the range
// from the start of the region up to the minimum of the old and new sizes. If
// the new size is larger than the old size, the added memory will not be
// initialized.
void *alloc_realloc(Allocator *allocator, void *ptr, size_t newsize);

#ifdef XSTD_IMPLEMENTATION
void *alloc_realloc(Allocator *allocator, void *ptr, size_t newsize) {
  ptr = allocator->vtable_->realloc(
      (void *)((uintptr_t)allocator + allocator->offset_), ptr, newsize);
  assert(ptr != NULL);
  return ptr;
}
#endif

Allocator *g_libc_allocator;

#ifdef XSTD_IMPLEMENTATION
static void *libc_malloc(void *alloc, size_t size) {
  (void)alloc;
  return malloc(size);
}

static void libc_free(void *alloc, void *ptr) {
  (void)alloc;
  free(ptr);
}

static void *libc_calloc(void *alloc, size_t nmemb, size_t size) {
  (void)alloc;
  void *ptr = calloc(nmemb, size);
  return ptr;
}

static void *libc_realloc(void *alloc, void *ptr, size_t size) {
  (void)alloc;
  return realloc(ptr, size);
}

static struct xstd_allocator_vtable libc_allocator_vtable = {
    .malloc = &libc_malloc,
    .free = &libc_free,
    .calloc = &libc_calloc,
    .realloc = &libc_realloc,
};

static Allocator libc_allocator = {
    .vtable_ = &libc_allocator_vtable,
    .offset_ = 0,
};

Allocator *g_libc_allocator = &libc_allocator;
#endif

#endif
#ifndef XSTD_IO_CLOSER_H_INCLUDE
#define XSTD_IO_CLOSER_H_INCLUDE

#include <stdbool.h>
#include <stdio.h>

#ifdef XSTD_IMPLEMENTATION
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#endif


struct xstd_closer_vtable {
  void (*close)(void *closer, int *error);
};

// Closer interface / virtual table. Closer is the interface that wraps basic
// Close method. The behavior of Close after the first call is undefined.
// Specific implementations may document their own behavior.
typedef struct {
  struct xstd_closer_vtable *vtable_;
  size_t offset_;
} Closer;

// closer_close calls the close method of the closer.
void closer_close(Closer *closer, int *error);

#ifdef XSTD_IMPLEMENTATION
void closer_close(Closer *closer, int *error) {
  closer->vtable_->close((void *)((uintptr_t)closer + closer->offset_), error);
}
#endif

// NopCloser is a no-op Closer implementation.
typedef struct {
  Closer closer;
} NopCloser;

void nop_closer_close(void *_, int *error);

#ifdef XSTD_IMPLEMENTATION
void nop_closer_close(void *_, int *error) {
  (void)_;
  (void)error;
}
#endif

struct xstd_closer_vtable nop_closer_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_closer_vtable nop_closer_vtable = {.close = &nop_closer_close};
#endif

NopCloser nop_closer(void);

#ifdef XSTD_IMPLEMENTATION
NopCloser nop_closer(void) {
  NopCloser nc = {0};
  nc.closer.vtable_ = &nop_closer_vtable;
  nc.closer.offset_ = 0;
  return nc;
}
#endif

// FileCloser wraps FILE and implements the Closer interface.
typedef struct {
  Closer closer;
  FILE *f_;
} FileCloser;

#ifdef XSTD_IMPLEMENTATION
static void file_closer_close(void *file, int *error) {
  FILE *f = *(void **)file;
  if (!fclose(f) && error != NULL)
    *error = errno;
}
#endif

struct xstd_closer_vtable file_closer_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_closer_vtable file_closer_vtable = {.close = &file_closer_close};
#endif

// file_closer_init initializes the given FileCloser.
void file_closer_init(FileCloser *fcloser, FILE *f);

#ifdef XSTD_IMPLEMENTATION
void file_closer_init(FileCloser *fcloser, FILE *f) {
  *fcloser = (FileCloser){0};
  fcloser->closer.vtable_ = &file_closer_vtable;
  fcloser->closer.offset_ = offsetof(FileCloser, f_);
  fcloser->f_ = f;
}
#endif

// file_closer_new allocates, initiliazes and returns a new FileCloser.
FileCloser *file_closer_new(Allocator *allocator, FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileCloser *file_closer_new(Allocator *allocator, FILE *f) {
  FileCloser *fcloser = alloc_malloc(allocator, sizeof(FileCloser));
  file_closer_init(fcloser, f);
  return fcloser;
}
#endif

// file_closer initiliazes and returns a FileCloser that wraps the given file.
FileCloser file_closer(FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileCloser file_closer(FILE *f) {
  FileCloser fcloser = {0};
  file_closer_init(&fcloser, f);
  return fcloser;
}
#endif

#endif
#ifndef XSTD_IO_READER_H_INCLUDE
#define XSTD_IO_READER_H_INCLUDE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef XSTD_IMPLEMENTATION
#include <errno.h>
#endif


struct xstd_reader_vtable {
  void (*read)(void *reader, uint8_t *p, size_t p_len, size_t *n, int *err);
};

// Reader is the interface that wraps the basic read method.
//
// read reads up to len(p) bytes into p. It returns the number of bytes read
// (0 <= n <= p_len) and any error encountered. Even if read returns n < p_len,
// it may use all of p as scratch space during the call. If some data is
// available but not p_len bytes, read conventionally returns what is available
// instead of waiting for more.
//
// When read encounters an error or end-of-file condition after successfully
// reading n > 0 bytes, it returns the number of bytes read. It may return the
// (non-null) error from the same call or return the error (and n == 0) from a
// subsequent call. An instance of this general case is that a Reader returning
// a non-zero number of bytes at the end of the input stream may return either
// err == EOF or err == nil. The next read should return 0, EOF.
//
// Callers should always process the n > 0 bytes returned before considering the
// error err. Doing so correctly handles I/O errors that happen after reading
// some bytes and also both of the allowed EOF behaviors.
//
// If p_len == 0, read should always return n == 0. It may return a non-null
// error if some error condition is known, such as EOF.
//
// Implementations of read are discouraged from returning a zero byte count with
// a nil error, except when len(p) == 0. Callers should treat a return of 0 and
// nil as indicating that nothing happened; in particular it does not indicate
// EOF.
//
// Implementations must not retain p.
typedef struct xstd_reader {
  struct xstd_reader_vtable *vtable_;
  size_t offset_;
} Reader;

// reader_read calls the read method of the reader.
void reader_read(Reader *reader, uint8_t *p, size_t p_len, size_t *n, int *err);

#ifdef XSTD_IMPLEMENTATION
void reader_read(Reader *reader, uint8_t *p, size_t p_len, size_t *n,
                 int *err) {
  reader->vtable_->read((void *)((uintptr_t)reader + reader->offset_), p, p_len,
                        n, err);
}
#endif

// FileReader wraps FILE and implements Reader.
typedef struct {
  Reader reader;
  FILE *f_;
} FileReader;

// file_reader_read implements Reader.
#ifdef XSTD_IMPLEMENTATION
static void file_reader_read(void *file, uint8_t *p, size_t p_len, size_t *n,
                             int *error) {
  FILE *f = *(void **)file;
  size_t read = fread(p, sizeof(*p), p_len, f);
  if (n != NULL)
    *n = read;

  if (error != NULL) {
    *error = errno;
    if (read == 0 && feof(f)) {
      *error = EOF;
    }
    clearerr(f);
  }
}
#endif

// FileReader virtual table for Reader interface.
struct xstd_reader_vtable file_reader_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_reader_vtable file_reader_vtable = {.read = &file_reader_read};
#endif

// file_reader_init initializes the given FileReader.
void file_reader_init(FileReader *freader, FILE *f);

#ifdef XSTD_IMPLEMENTATION
void file_reader_init(FileReader *freader, FILE *f) {
  *freader = (FileReader){0};
  freader->reader.vtable_ = &file_reader_vtable;
  freader->reader.offset_ = offsetof(FileReader, f_);
  freader->f_ = f;
}
#endif

// file_reader_new allocates, initializes and returns a new FileReader.
FileReader *file_reader_new(Allocator *allocator, FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileReader *file_reader_new(Allocator *allocator, FILE *f) {
  FileReader *freader = alloc_malloc(allocator, sizeof(FileReader));
  file_reader_init(freader, f);
  return freader;
}
#endif

// file_reader initializes and returns a FileReader that wraps the given file.
FileReader file_reader(FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileReader file_reader(FILE *f) {
  FileReader freader = {0};
  file_reader_init(&freader, f);
  return freader;
}
#endif

#endif
#ifndef XSTD_IO_WRITER_H_INCLUDE
#define XSTD_IO_WRITER_H_INCLUDE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef XSTD_IMPLEMENTATION
#include <errno.h>
#endif


struct xstd_writer_vtable {
  void (*write)(void *writer, uint8_t *p, size_t p_len, size_t *n, int *err);
};

// Writer is the interface that wraps the basic Write method.
//
// Write writes len(p) bytes from p to the underlying data stream. It returns
// the number of bytes written from p (0 <= n <= len(p)) and any error
// encountered that caused the write to stop early. Write must return a non-nil
// error if it returns n < len(p). Write must not modify the slice data, even
// temporarily.
typedef struct {
  struct xstd_writer_vtable *vtable_;
  size_t offset_;
} Writer;

void writer_write(Writer *writer, uint8_t *p, size_t p_len, size_t *n,
                  int *error);

#ifdef XSTD_IMPLEMENTATION
void writer_write(Writer *writer, uint8_t *p, size_t p_len, size_t *n,
                  int *error) {
  writer->vtable_->write((void *)((uintptr_t)writer + writer->offset_), p,
                         p_len, n, error);
}
#endif

// FileWriter wraps FILE and implements Writer.
typedef struct {
  Writer writer;
  FILE *f_;
} FileWriter;

#ifdef XSTD_IMPLEMENTATION
static void file_writer_write(void *file, uint8_t *p, size_t p_len, size_t *n,
                              int *error) {
  FILE *f = *(void **)file;
  size_t write = fwrite(p, sizeof(*p), p_len, f);
  if (n != NULL)
    *n = write;

  if (error != NULL) {
    *error = errno;
    if (write == 0 && feof(f))
      *error = EOF;

    clearerr(f);
  }
}

#endif

// FileWriter virtual table for Writer interface.
struct xstd_writer_vtable file_writer_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_writer_vtable file_writer_vtable = {.write = &file_writer_write};
#endif

// file_writer_init initializes the given FileWriter.
void file_writer_init(FileWriter *fw, FILE *f);

#ifdef XSTD_IMPLEMENTATION
void file_writer_init(FileWriter *fw, FILE *f) {
  *fw = (FileWriter){0};
  fw->writer.vtable_ = &file_writer_vtable;
  fw->writer.offset_ = offsetof(FileWriter, f_);
  fw->f_ = f;
}
#endif

// file_writer_new allocates, initializes and returns a new FileWriter.
FileWriter *file_writer_new(Allocator *allocator, FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileWriter *file_writer_new(Allocator *allocator, FILE *f) {
  FileWriter *fw = alloc_malloc(allocator, sizeof(FileWriter));
  file_writer_init(fw, f);
  return fw;
}
#endif

// file_writer initializes and returns a FileWriter that wraps the given file.
FileWriter file_writer(FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileWriter file_writer(FILE *f) {
  FileWriter fw = {0};
  file_writer_init(&fw, f);
  return fw;
}
#endif

#endif
#ifndef XSTD_IO_READ_CLOSER_H_INCLUDE
#define XSTD_IO_READ_CLOSER_H_INCLUDE

#ifdef XSTD_IMPLEMENTATION
#include <stddef.h>
#endif


// ReadCloser is the interface that groups the basic Read and Close methods.
typedef struct {
  Reader reader;
  Closer closer;
} ReadCloser;

// FileReadCloser wraps FILE and implements ReadCloser.
typedef struct {
  ReadCloser read_closer;
  FILE *f_;
} FileReadCloser;

// file_read_closer_init initializes the given FileReadCloser.
void file_read_closer_init(FileReadCloser *frw, FILE *f);

#ifdef XSTD_IMPLEMENTATION
void file_read_closer_init(FileReadCloser *frw, FILE *f) {
  *frw = (FileReadCloser){0};
  frw->read_closer.reader.vtable_ = &file_reader_vtable;
  frw->read_closer.reader.offset_ = offsetof(FileReadCloser, f_);

  frw->read_closer.closer.vtable_ = &file_closer_vtable;
  frw->read_closer.closer.offset_ =
      offsetof(FileReadCloser, f_) -
      offsetof(FileReadCloser, read_closer.closer);

  frw->f_ = f;
}
#endif

// file_read_closer allocates and initializes and returns a FileReadCloser that
// wraps the given file.
FileReadCloser *file_read_closer_new(Allocator *allocator, FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileReadCloser *file_read_closer_new(Allocator *allocator, FILE *f) {
  FileReadCloser *frw = alloc_malloc(allocator, sizeof(FileReadCloser));
  file_read_closer_init(frw, f);
  return frw;
}
#endif

// file_read_closer initializes and returns a FileReadCloser that wraps the
// given file.
FileReadCloser file_read_closer(FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileReadCloser file_read_closer(FILE *f) {
  FileReadCloser frw = {0};
  file_read_closer_init(&frw, f);
  return frw;
}
#endif

#endif
#ifndef XSTD_IO_READ_WRITER_H_INCLUDE
#define XSTD_IO_READ_WRITER_H_INCLUDE

#ifdef XSTD_IMPLEMENTATION
#include <stddef.h>
#endif


// ReadWriter is the interface that groups the basic Read and Write methods.
typedef struct {
  Reader reader;
  Writer writer;
} ReadWriter;

// FileReadWriter wraps FILE and implements ReadWriter.
typedef struct {
  ReadWriter read_writer;
  FILE *f_;
} FileReadWriter;

// file_read_writer_init initializes the given FileReadWriter.
void file_read_writer_init(FileReadWriter *frw, FILE *f);

#ifdef XSTD_IMPLEMENTATION
void file_read_writer_init(FileReadWriter *frw, FILE *f) {
  *frw = (FileReadWriter){0};
  frw->read_writer.reader.vtable_ = &file_reader_vtable;
  frw->read_writer.reader.offset_ = offsetof(FileReadWriter, f_);

  frw->read_writer.writer.vtable_ = &file_writer_vtable;
  frw->read_writer.writer.offset_ =
      offsetof(FileReadWriter, f_) -
      offsetof(FileReadWriter, read_writer.writer);

  frw->f_ = f;
}
#endif

// file_read_writer allocates and initializes and returns a FileReadWriter that
// wraps the given file.
FileReadWriter *file_read_writer_new(Allocator *allocator, FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileReadWriter *file_read_writer_new(Allocator *allocator, FILE *f) {
  FileReadWriter *frw = alloc_malloc(allocator, sizeof(FileReadWriter));
  file_read_writer_init(frw, f);
  return frw;
}
#endif

// file_read_writer initializes and returns a FileReadWriter that wraps the
// given file.
FileReadWriter file_read_writer(FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileReadWriter file_read_writer(FILE *f) {
  FileReadWriter frw = {0};
  file_read_writer_init(&frw, f);
  return frw;
}
#endif

#endif
#ifndef XSTD_IO_WRITE_CLOSER_H_INCLUDE
#define XSTD_IO_WRITE_CLOSER_H_INCLUDE

#ifdef XSTD_IMPLEMENTATION
#include <stddef.h>
#endif


// WriteCloser is the interface that groups the basic Write and Close methods.
typedef struct {
  Writer writer;
  Closer closer;
} WriteCloser;

// FileWriteCloser wraps FILE and implements WriteCloser.
typedef struct {
  WriteCloser write_closer;
  FILE *f_;
} FileWriteCloser;

// file_write_closer_init initializes the given FileWriteCloser.
void file_write_closer_init(FileWriteCloser *frw, FILE *f);

#ifdef XSTD_IMPLEMENTATION
void file_write_closer_init(FileWriteCloser *frw, FILE *f) {
  *frw = (FileWriteCloser){0};
  frw->write_closer.writer.vtable_ = &file_writer_vtable;
  frw->write_closer.writer.offset_ = offsetof(FileWriteCloser, f_);

  frw->write_closer.closer.vtable_ = &file_closer_vtable;
  frw->write_closer.closer.offset_ =
      offsetof(FileWriteCloser, f_) -
      offsetof(FileWriteCloser, write_closer.closer);

  frw->f_ = f;
}
#endif

// file_write_closer allocates and initializes and returns a FileWriteCloser
// that wraps the given file.
FileWriteCloser *file_write_closer_new(Allocator *allocator, FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileWriteCloser *file_write_closer_new(Allocator *allocator, FILE *f) {
  FileWriteCloser *frw = alloc_malloc(allocator, sizeof(FileWriteCloser));
  file_write_closer_init(frw, f);
  return frw;
}
#endif

// file_write_closer initializes and returns a FileWriteCloser that wraps the
// given file.
FileWriteCloser file_write_closer(FILE *f);

#ifdef XSTD_IMPLEMENTATION
FileWriteCloser file_write_closer(FILE *f) {
  FileWriteCloser frw = {0};
  file_write_closer_init(&frw, f);
  return frw;
}
#endif

#endif
#ifndef XSTD_BUF_READER_H_INCLUDE
#define XSTD_BUF_READER_H_INCLUDE

#include <stddef.h>
#include <stdint.h>

#ifdef XSTD_IMPLEMENTATION
#include <string.h>
#endif


struct xstd_buf_reader_body {
  Reader *reader;
  uint8_t *buf;
  size_t buf_len;
  int read_err;
  size_t read;
  size_t write;
};

// BufReader wraps a Reader
typedef struct {
  Reader reader;
  struct xstd_buf_reader_body body_;
} BufReader;

size_t buf_reader_buffered(BufReader *buf_reader);

#ifdef XSTD_IMPLEMENTATION
size_t buf_reader_buffered(BufReader *buf_reader) {
  return buf_reader->body_.read - buf_reader->body_.write;
}
#endif

#ifdef XSTD_IMPLEMENTATION
static void buf_reader_read(void *body, uint8_t *p, size_t p_len, size_t *n,
                            int *err) {
  // p is empty.
  if (p_len == 0)
    return;

  struct xstd_buf_reader_body *b = (struct xstd_buf_reader_body *)body;

  size_t buffered = b->read - b->write;

  // No buffered data.
  if (buffered <= 0) {
    if (b->read_err != 0) {
      if (n != NULL)
        *n = 0;
      if (err != NULL)
        *err = b->read_err;
      return;
    }

    // p is larger than internal buffer,
    // read directly into p to avoid copy.
    if (p_len > b->buf_len) {
      reader_read(b->reader, p, p_len, n, err);
      return;
    }

    // Reset cursors.
    b->read = 0;
    b->write = 0;
    // Read into internal buffer.
    reader_read(b->reader, b->buf, b->buf_len, &b->read, &b->read_err);
    if (b->read == 0) {
      if (err != NULL)
        *err = b->read_err;
      return;
    }
    buffered = b->read;
  }

  size_t copied = buffered < p_len ? buffered : p_len;
  memcpy(p, &b->buf[b->write], copied);
  b->write += copied;

  if (n != NULL)
    *n = copied;
}
#endif

struct xstd_reader_vtable buf_reader_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_reader_vtable buf_reader_vtable = {.read = &buf_reader_read};
#endif

void buf_reader_init(BufReader *buf_reader, Reader *r, uint8_t *buf,
                     size_t buf_len);

#ifdef XSTD_IMPLEMENTATION
void buf_reader_init(BufReader *buf_reader, Reader *r, uint8_t *buf,
                     size_t buf_len) {
  *buf_reader = (BufReader){0};
  buf_reader->reader.vtable_ = &buf_reader_vtable;
  buf_reader->reader.offset_ = offsetof(BufReader, body_);

  buf_reader->body_.reader = r;
  buf_reader->body_.buf = buf;
  buf_reader->body_.buf_len = buf_len;
}
#endif

BufReader *buf_reader_new(Allocator *allocator, Reader *r, uint8_t *buf,
                          size_t buf_len);

#ifdef XSTD_IMPLEMENTATION
BufReader *buf_reader_new(Allocator *allocator, Reader *r, uint8_t *buf,
                          size_t buf_len) {
  BufReader *buf_reader = alloc_malloc(allocator, sizeof(BufReader));
  buf_reader_init(buf_reader, r, buf, buf_len);
  return buf_reader;
}
#endif

BufReader buf_reader(Reader *r, uint8_t *buf, size_t buf_len);

#ifdef XSTD_IMPLEMENTATION
BufReader buf_reader(Reader *r, uint8_t *buf, size_t buf_len) {
  BufReader buf_reader = {0};
  buf_reader_init(&buf_reader, r, buf, buf_len);
  return buf_reader;
}
#endif

#endif
#ifndef XSTD_SLICE_H_INCLUDE
#define XSTD_SLICE_H_INCLUDE

#include <stddef.h>
#include <stdint.h>

#ifdef XSTD_IMPLEMENTATION
#include <assert.h>
#endif

// Slice define a slice/portion of a buffer. Slice doesn't own memory it
// contains.
typedef struct {
  uint8_t *const data;
  const size_t len;
} Slice;

size_t slice_len(Slice *slice);

#ifdef XSTD_IMPLEMENTATION
size_t slice_len(Slice *slice) { return slice->len; }
#endif

Slice slice(uint8_t *data, size_t length);

#ifdef XSTD_IMPLEMENTATION
Slice slice(uint8_t *data, size_t length) {
  Slice s = {.data = data, .len = length};
  return s;
}
#endif

#endif
#ifndef XSTD_BYTES_BUFFER_H_INCLUDE
#define XSTD_BYTES_BUFFER_H_INCLUDE

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define XSTD_IMPLEMENTATION
#ifdef XSTD_IMPLEMENTATION
#include <assert.h>
#endif


// BytesBuffer define a resizable bytes array that owns its memory.
typedef struct {
  uint8_t *bytes_;
  Allocator *allocator_;
  size_t len_;
  size_t cap_;
} BytesBuffer;

size_t bytes_buffer_capacity(const BytesBuffer *buffer);

#ifdef XSTD_IMPLEMENTATION
size_t bytes_buffer_capacity(const BytesBuffer *buffer) { return buffer->cap_; }
#endif

size_t bytes_buffer_length(const BytesBuffer *buffer);

#ifdef XSTD_IMPLEMENTATION
size_t bytes_buffer_length(const BytesBuffer *buffer) { return buffer->len_; }
#endif

size_t bytes_buffer_available(const BytesBuffer *buffer);

#ifdef XSTD_IMPLEMENTATION
size_t bytes_buffer_available(const BytesBuffer *buffer) {
  return buffer->cap_ - buffer->len_;
}
#endif

void bytes_buffer_reset(BytesBuffer *buffer);

#ifdef XSTD_IMPLEMENTATION
void bytes_buffer_reset(BytesBuffer *buffer) { buffer->len_ = 0; }
#endif

Slice bytes_buffer_slice(const BytesBuffer *buffer, size_t start, size_t end);

#ifdef XSTD_IMPLEMENTATION
Slice bytes_buffer_slice(const BytesBuffer *buffer, size_t start, size_t end) {
  assert(buffer->len_ >= start);
  assert(buffer->len_ < end);
  assert(end >= start);

  return slice(&buffer->bytes_[start], end - start);
}
#endif

Slice bytes_buffer_bytes(const BytesBuffer *buffer);

#ifdef XSTD_IMPLEMENTATION
Slice bytes_buffer_bytes(const BytesBuffer *buffer) {
  return slice(buffer->bytes_, buffer->len_);
}
#endif

bool bytes_buffer_resize(BytesBuffer *buffer, size_t new_capacity);

#ifdef XSTD_IMPLEMENTATION
bool bytes_buffer_resize(BytesBuffer *buffer, size_t new_capacity) {
  uint8_t *bytes =
      alloc_realloc(buffer->allocator_, buffer->bytes_, new_capacity);
  if (bytes == NULL)
    return false;

  buffer->bytes_ = bytes;
  buffer->cap_ = new_capacity;

  if (buffer->cap_ < buffer->len_)
    buffer->len_ = buffer->cap_;
  return true;
}
#endif

#define bytes_buffer_append(buffer, data)                                      \
  bytes_buffer_append_bytes(buffer, data, sizeof(typeof(*data)))

#define bytes_buffer_append_char(buffer, data)                                 \
  bytes_buffer_append_bytes(buffer, data, sizeof(char))

size_t bytes_buffer_append_bytes(BytesBuffer *buffer, void *data, size_t size);

#ifdef XSTD_IMPLEMENTATION
size_t bytes_buffer_append_bytes(BytesBuffer *buffer, void *data, size_t size) {
  // Resize if needed.
  bool need_resize = bytes_buffer_available(buffer) < size;
  while (bytes_buffer_available(buffer) < size) {
    if (buffer->cap_ == 0)
      buffer->cap_ = 1;
    else
      buffer->cap_ *= 2;
  }
  if (need_resize)
    if (!bytes_buffer_resize(buffer, buffer->cap_))
      return 0;

  memcpy(&buffer->bytes_[buffer->len_], data, size);
  buffer->len_ += size;

  return size;
}
#endif

void bytes_buffer_fill(BytesBuffer *buffer, int c);

#ifdef XSTD_IMPLEMENTATION
void bytes_buffer_fill(BytesBuffer *buffer, int c) {
  memset(buffer->bytes_, c, buffer->cap_);
  buffer->len_ = buffer->cap_;
}
#endif

void bytes_buffer_fill_available(BytesBuffer *buffer, int c);

#ifdef XSTD_IMPLEMENTATION
void bytes_buffer_fill_available(BytesBuffer *buffer, int c) {
  memset(&buffer->bytes_[buffer->len_], c, bytes_buffer_available(buffer));
  buffer->len_ = buffer->cap_;
}
#endif

#define bytes_buffer_get(buffer, index, type)                                  \
  *(type *)(bytes_buffer_get_(buffer, index, sizeof(type)))

#define bytes_buffer_get_ptr(buffer, index, type)                              \
  ((type *)(bytes_buffer_get_(buffer, index, sizeof(type))))

void *bytes_buffer_get_(const BytesBuffer *buffer, size_t index,
                        size_t elem_size);

#ifdef XSTD_IMPLEMENTATION
void *bytes_buffer_get_(const BytesBuffer *buffer, size_t index,
                        size_t elem_size) {
  assert(index * elem_size < buffer->len_);
  return (void *)&buffer->bytes_[index * elem_size];
}
#endif

void bytes_buffer_init(BytesBuffer *buffer, Allocator *allocator);

#ifdef XSTD_IMPLEMENTATION
void bytes_buffer_init(BytesBuffer *buffer, Allocator *allocator) {
  *buffer = (BytesBuffer){0};
  buffer->allocator_ = allocator;
}
#endif

BytesBuffer *bytes_buffer_new(Allocator *allocator);

#ifdef XSTD_IMPLEMENTATION
BytesBuffer *bytes_buffer_new(Allocator *allocator) {
  BytesBuffer *buf = alloc_malloc(allocator, sizeof(BytesBuffer));
  bytes_buffer_init(buf, allocator);
  return buf;
}
#endif

BytesBuffer bytes_buffer(Allocator *allocator);

#ifdef XSTD_IMPLEMENTATION
BytesBuffer bytes_buffer(Allocator *allocator) {
  BytesBuffer buf = {0};
  bytes_buffer_init(&buf, allocator);
  return buf;
}
#endif

void bytes_buffer_deinit(BytesBuffer *buf);

#ifdef XSTD_IMPLEMENTATION
void bytes_buffer_deinit(BytesBuffer *buf) {
  alloc_free(buf->allocator_, buf->bytes_);
}
#endif

void bytes_buffer_free(BytesBuffer *buf);

#ifdef XSTD_IMPLEMENTATION
void bytes_buffer_free(BytesBuffer *buf) {
  if (buf == NULL)
    return;

  bytes_buffer_deinit(buf);
  alloc_free(buf->allocator_, buf);
}
#endif

#endif
#ifndef XSTD_BYTES_READER_H_INCLUDE
#define XSTD_BYTES_READER_H_INCLUDE

#ifdef XSTD_IMPLEMENTATION
#include <string.h>
#endif


typedef struct {
  Reader reader;
  BytesBuffer *buffer_;
} BytesBufferReader;

#ifdef XSTD_IMPLEMENTATION
static void bytes_buffer_reader_read(void *reader, uint8_t *p, size_t p_len,
                                     size_t *n, int *err) {
  (void)err;

  BytesBuffer *buffer = (BytesBuffer *)reader;
  size_t copied = p_len > buffer->len_ ? buffer->len_ : p_len;
  memcpy(p, buffer->bytes_, copied);
  if (n != NULL)
    *n = copied;
}
#endif

struct xstd_reader_vtable bytes_buffer_reader_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_reader_vtable bytes_buffer_reader_vtable = {
    .read = &bytes_buffer_reader_read,
};
#endif

BytesBufferReader bytes_buffer_reader(BytesBuffer *buffer);

#ifdef XSTD_IMPLEMENTATION
BytesBufferReader bytes_buffer_reader(BytesBuffer *buffer) {
  BytesBufferReader bbw = {0};
  bbw.reader.vtable_ = &bytes_buffer_reader_vtable;
  bbw.reader.offset_ = offsetof(BytesBufferReader, buffer_);
  bbw.buffer_ = buffer;
  return bbw;
}
#endif

#endif
#ifndef XSTD_BYTES_WRITER_H_INCLUDE
#define XSTD_BYTES_WRITER_H_INCLUDE

#ifdef XSTD_IMPLEMENTATION
#include <errno.h>
#include <stddef.h>
#endif


typedef struct {
  Writer writer;
  BytesBuffer *buffer_;
} BytesBufferWriter;

#ifdef XSTD_IMPLEMENTATION
static void bytes_buffer_writer_write(void *writer, uint8_t *p, size_t p_len,
                                      size_t *n, int *err) {

  BytesBuffer *buffer = (BytesBuffer *)writer;
  size_t appended = bytes_buffer_append_bytes(buffer, p, p_len);
  if (n != NULL)
    *n = appended;

  if (err != NULL && appended != p_len)
    *err = ENOMEM;
}
#endif

struct xstd_writer_vtable bytes_buffer_writer_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_writer_vtable bytes_buffer_writer_vtable = {
    .write = &bytes_buffer_writer_write,
};
#endif

BytesBufferWriter bytes_buffer_writer(BytesBuffer *buffer);

#ifdef XSTD_IMPLEMENTATION
BytesBufferWriter bytes_buffer_writer(BytesBuffer *buffer) {
  BytesBufferWriter bbw = {0};
  bbw.writer.vtable_ = &bytes_buffer_writer_vtable;
  bbw.writer.offset_ = offsetof(BytesBufferWriter, buffer_);
  bbw.buffer_ = buffer;
  return bbw;
}
#endif

#endif
// Single file header iterator interface / vtable
// from xstd (https://github.com/negrel/xstd.h).

#ifndef XSTD_ITER_H_INCLUDE
#define XSTD_ITER_H_INCLUDE

#include <stddef.h>
#include <stdint.h>
#include <string.h>


struct xstd_iterator_vtable {
  void *(*next)(void *);
};

// Iterator interface.
typedef struct xstd_iterator {
  struct xstd_iterator_vtable *vtable_;
  size_t offset_;
} Iterator;

#define iter_foreach(iter, type, iterator)                                     \
  for (                                                                        \
      struct {                                                                 \
        size_t index;                                                          \
        type value;                                                            \
      } iterator =                                                             \
          {                                                                    \
              .index = 0,                                                      \
             .value = iter_next(iter),                                         \
          };                                                                   \
      iterator.value != NULL;                                                  \
      iterator.index++, iterator.value = iter_next(iter))

void *iter_next(Iterator *);

#ifdef XSTD_IMPLEMENTATION
void *iter_next(Iterator *iter) {
  return iter->vtable_->next((void *)((uintptr_t)iter + iter->offset_));
}
#endif

#define range_foreach(iterator, from, to, step)                                \
  for (struct {                                                                \
         short i;                                                              \
         RangeIterator iter;                                                   \
       } tmp = {.i = 0, .iter = range_iter(from, to, step)};                   \
       tmp.i == 0; tmp.i = 1)                                                  \
  iter_foreach((Iterator *)&tmp.iter, size_t *, iterator)

#define range_to_foreach(i, to) range_foreach(i, 0, to, 1)
#define range_from_to_foreach(i, from, to) range_foreach(i, from, to, 1)

struct xstd_range_iter_body {
  int64_t value, end, step;
};

typedef struct xstd_range_iter {
  Iterator iterator;
  struct xstd_range_iter_body body_;
} RangeIterator;

#ifdef XSTD_IMPLEMENTATION
static void *range_iterator_next(void *body) {
  struct xstd_range_iter_body *b = (struct xstd_range_iter_body *)body;

  b->value += b->step;
  if (b->value >= b->end) {
    b->value -= b->step;
    return NULL;
  }

  return &b->value;
}
#endif

struct xstd_iterator_vtable xstd_range_iterator_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_iterator_vtable xstd_range_iterator_vtable = {
    .next = &range_iterator_next,
};
#endif

void range_iterator_init(RangeIterator *range_iterator, int64_t start,
                         int64_t end, int64_t step);

#ifdef XSTD_IMPLEMENTATION
void range_iterator_init(RangeIterator *range_iterator, int64_t start,
                         int64_t end, int64_t step) {
  *range_iterator = (RangeIterator){0};
  range_iterator->iterator.vtable_ = &xstd_range_iterator_vtable;
  range_iterator->iterator.offset_ = offsetof(RangeIterator, body_);
  range_iterator->body_.end = end;
  range_iterator->body_.step = step;
  range_iterator->body_.value = start - step;
}
#endif

RangeIterator *range_iterator_new(Allocator *allocator, int64_t start,
                                  int64_t end, int64_t step);

#ifdef XSTD_IMPLEMENTATION
RangeIterator *range_iterator_new(Allocator *allocator, int64_t start,
                                  int64_t end, int64_t step) {
  RangeIterator *ri = alloc_malloc(allocator, sizeof(RangeIterator));
  range_iterator_init(ri, start, end, step);
  return ri;
}
#endif

RangeIterator range_iter(int64_t start, int64_t end, int64_t step);

#ifdef XSTD_IMPLEMENTATION
RangeIterator range_iter(int64_t start, int64_t end, int64_t step) {
  RangeIterator ri = {0};
  range_iterator_init(&ri, start, end, step);
  return ri;
}
#endif

#endif
#ifndef XSTD_ARENA_H_INCLUDE
#define XSTD_ARENA_H_INCLUDE

#include <stddef.h>

#ifdef XSTD_IMPLEMENTATION
#include <assert.h>
#include <stdint.h>
#endif


struct xstd_arena_allocator_body {
  Allocator *parent_allocator_;
  size_t arena_size_;
  struct xstd_arena *arena_list_;
  size_t cursor_;
};

// ArenaAllocator is an arena allocator.
typedef struct xstd_arena_allocator {
  Allocator allocator;
  struct xstd_arena_allocator_body body_;
} ArenaAllocator;

struct xstd_arena {
  // Header
  struct xstd_arena *next;

  // Body
  // ...
};

void arena_allocator_init(ArenaAllocator *arena, Allocator *parent,
                          size_t arena_size);

#ifdef XSTD_IMPLEMENTATION
static void *arena_calloc(void *b, size_t nmemb, size_t size) {
  struct xstd_arena_allocator_body *arena_body =
      (struct xstd_arena_allocator_body *)b;

  // Detect overflow.
  size_t alloc_size = nmemb * size;
  if (alloc_size / size != nmemb ||
      alloc_size + sizeof(struct xstd_arena) < alloc_size) {
    return NULL;
  }

  if (alloc_size > arena_body->arena_size_) {
    struct xstd_arena *new_arena =
        alloc_calloc(arena_body->parent_allocator_, 1,
                     alloc_size + sizeof(struct xstd_arena));
    if (new_arena == NULL)
      return NULL;

    new_arena->next = arena_body->arena_list_;
    arena_body->arena_list_ = new_arena;
    arena_body->cursor_ = arena_body->arena_size_; // Allocated arena as full.
    return (void *)((uintptr_t)new_arena + sizeof(struct xstd_arena));
  }

  // Allocate at least size of a pointer to ensure all pointer are aligned.
  alloc_size = alloc_size < sizeof(void *) ? sizeof(void *) : alloc_size;

  // No current arena.
  if (arena_body->arena_list_ == NULL) {
    struct xstd_arena *new_arena =
        alloc_calloc(arena_body->parent_allocator_, 1,
                     arena_body->arena_size_ + sizeof(struct xstd_arena));
    if (new_arena == NULL)
      return NULL;

    new_arena->next = arena_body->arena_list_;
    arena_body->arena_list_ = new_arena;
    arena_body->cursor_ = 0;
  } else if (arena_body->arena_size_ - arena_body->cursor_ < alloc_size) {
    struct xstd_arena *new_arena =
        alloc_calloc(arena_body->parent_allocator_, 1,
                     arena_body->arena_size_ + sizeof(struct xstd_arena));
    if (new_arena == NULL)
      return NULL;

    new_arena->next = arena_body->arena_list_;
    arena_body->arena_list_ = new_arena;
    arena_body->cursor_ = 0;
  }

  void *ptr = (void *)((uintptr_t)arena_body->arena_list_ +
                       sizeof(struct xstd_arena) + arena_body->cursor_);
  arena_body->cursor_ += alloc_size;
  return ptr;
}

static void *arena_malloc(void *allocator, size_t size) {
  return arena_calloc(allocator, 1, size);
}

static void *arena_realloc(void *allocator, void *ptr, size_t size) {
  (void)ptr;
  // TODO: if last alloc, return same pointer.
  // TODO: if new size is smaller, return same pointer.

  return arena_malloc(allocator, size);
}

static void arena_free(void *allocator, void *ptr) {
  (void)allocator;
  (void)ptr;
}

static struct xstd_allocator_vtable arena_allocator_vtable = {
    .malloc = &arena_malloc,
    .free = &arena_free,
    .calloc = &arena_calloc,
    .realloc = &arena_realloc,
};

void arena_allocator_init(ArenaAllocator *arena, Allocator *parent,
                          size_t arena_size) {
  assert(arena_size > sizeof(void *) &&
         "arena size must be greater than size of a pointer");

  arena->body_.parent_allocator_ = parent;
  arena->body_.arena_size_ = arena_size;
  arena->body_.arena_list_ = NULL;
  arena->allocator = (Allocator){0};
  arena->allocator.vtable_ = &arena_allocator_vtable;
  arena->allocator.offset_ = offsetof(ArenaAllocator, body_);
}
#endif

ArenaAllocator arena_allocator(Allocator *parent, size_t arena_size);

#ifdef XSTD_IMPLEMENTATION
ArenaAllocator arena_allocator(Allocator *parent, size_t arena_size) {
  ArenaAllocator alloc = {0};
  arena_allocator_init(&alloc, parent, arena_size);
  return alloc;
}
#endif

void arena_alloc_reset(ArenaAllocator *arena);

#ifdef XSTD_IMPLEMENTATION
void arena_alloc_reset(ArenaAllocator *arena_alloc) {
  struct xstd_arena **arena = &arena_alloc->body_.arena_list_;
  while (*arena != NULL) {
    struct xstd_arena *a = *arena;
    *arena = a->next;
    alloc_free(arena_alloc->body_.parent_allocator_, a);
  }

  arena_allocator_init(arena_alloc, arena_alloc->body_.parent_allocator_,
                       arena_alloc->body_.arena_size_);
}
#endif

#endif
// Single file header vector (growable array)
// from xstd (https://github.com/negrel/xstd.h).

#ifndef XSTD_VEC_H_INCLUDE
#define XSTD_VEC_H_INCLUDE

#include <stdbool.h>
#include <stddef.h>

#ifdef XSTD_IMPLEMENTATION
#include <assert.h>
#include <stdint.h>
#include <string.h>
#endif


typedef void *Vec;

struct xstd_vector {
  // Header
  Allocator *allocator_;
  size_t cap_;
  size_t len_;
  size_t elem_size_;

  // Body
  // ...
};

// vec_foreach is a foreach macro for the vector type.
// The first parameter is the vector on which to iterate. The
// second is the name of the iterator. The index is accessible
// with `iterator.index` and the value with `iterator.value`.
#define vec_foreach(vec, iterator)                                             \
  for (struct {                                                                \
         size_t index;                                                         \
         typeof(*vec) value;                                                   \
       } iterator = {.value = vec[0]};                                         \
       iterator.index < vec_len(vec); iterator.value = vec[++iterator.index])

// vec_foreach_ptr does the same as vec_foreach
// but the iterator contains a ptr instead of a copy.
#define vec_foreach_ptr(vec, iterator)                                         \
  for (struct {                                                                \
         size_t index;                                                         \
         typeof(vec) value;                                                    \
       } iterator = {.value = &vec[0]};                                        \
       iterator.index < vec_len(vec); iterator.value = &vec[++iterator.index])

// vec_push returns a pointer to an element at the end of the vector.
// This function takes a double pointer to a vector to reallocate in case it's
// full.
#define vec_push(vec) ((typeof(*vec))vec_push_((Vec *)vec))

// vec_unshift adds an element at the beginning of the vector and shift
// other elements.
#define vec_unshift(vec) ((typeof(*vec))vec_unshift_((void **)vec))

#define bodyof_vec(vecptr) ((uintptr_t)(vecptr) + sizeof(struct xstd_vector))
#define headerof_vec(vecptr)                                                   \
  ((struct xstd_vector *)((uintptr_t)vecptr - sizeof(struct xstd_vector)))

#ifdef XSTD_IMPLEMENTATION
static size_t sizeof_vector(struct xstd_vector *v) {
  return sizeof(struct xstd_vector) + v->cap_ * v->elem_size_;
}
#endif

// vec_new returns a new zeroed vector with the given capacity to store
// element of the given size. It the returns a pointer to the first element of
// the vector.
Vec vec_new(Allocator *allocator, size_t cap, size_t elem_size);

#ifdef XSTD_IMPLEMENTATION
Vec vec_new(Allocator *allocator, size_t cap, size_t elem_size) {
  size_t header = sizeof(struct xstd_vector);
  size_t body = cap * elem_size;

  struct xstd_vector *vec = alloc_calloc(allocator, header + body, 1);
  if (vec == NULL)
    return NULL;

  vec->allocator_ = allocator;
  vec->len_ = 0;
  vec->cap_ = cap;
  vec->elem_size_ = elem_size;

  return (void *)bodyof_vec(vec);
}
#endif

// vec_clone creates a clone of the given vector and returns it.
Vec vec_clone(const Vec);

#ifdef XSTD_IMPLEMENTATION
Vec vec_clone(const Vec vec) {
  assert(vec != NULL);

  struct xstd_vector *v = headerof_vec(vec);
  size_t size_v = sizeof_vector(v);

  void *clone = alloc_malloc(v->allocator_, size_v);
  if (clone == NULL)
    return NULL;

  memcpy(clone, v, size_v);

  return (void *)bodyof_vec(clone);
}
#endif

// vec_len returns the current len of the vector.
size_t vec_len(const Vec);

#ifdef XSTD_IMPLEMENTATION
size_t vec_len(const Vec vec) {
  assert(vec != NULL);

  return headerof_vec(vec)->len_;
}
#endif

// vec_cap returns the current capacity of the vector.
size_t vec_cap(const Vec);

#ifdef XSTD_IMPLEMENTATION
size_t vec_cap(const Vec vec) {
  assert(vec != NULL);

  return headerof_vec(vec)->cap_;
}
#endif

// vec_isfull returns true if the vector is full.
bool vec_isfull(const Vec);

#ifdef XSTD_IMPLEMENTATION
bool vec_isfull(const Vec vec) {
  assert(vec != NULL);

  return vec_len(vec) == vec_cap(vec);
}
#endif

// vec_isempty returns true if the vector size is 0.
bool vec_isempty(const Vec);

#ifdef XSTD_IMPLEMENTATION
bool vec_isempty(const Vec vec) {
  assert(vec != NULL);

  return vec_len(vec) == 0;
}
#endif

// vec_push_ returns a pointer to an element at the end of the vector.
// This function takes a pointer to a vector to reallocate it if it's full.
// An handy vec_push macro exists so you don't have to cast arguments and
// return type.
void *vec_push_(Vec *);

#ifdef XSTD_IMPLEMENTATION
void *vec_push_(void **vec) {
  assert(vec != NULL);
  assert(*vec != NULL);

  if (vec_isfull(*vec)) {
    // Let's double the capacity of our vector
    struct xstd_vector *v = headerof_vec(*vec);
    v->cap_ *= 2;
    *vec =
        (void *)bodyof_vec(alloc_realloc(v->allocator_, v, sizeof_vector(v)));
  }

  struct xstd_vector *v = headerof_vec(*vec);
  size_t offset = v->elem_size_ * v->len_;

  v->len_++;

  return (void *)(bodyof_vec(v) + offset);
}
#endif

// vec_pop removes the last element of the vector and store it in popped
// if not null.
void vec_pop(Vec, void *);

#ifdef XSTD_IMPLEMENTATION

void vec_pop(Vec vec, void *popped) {
  assert(vec != NULL);

  if (vec_isempty(vec))
    return;

  struct xstd_vector *v = headerof_vec(vec);
  v->len_--;

  if (popped != NULL)
    memcpy(popped, (void *)((uintptr_t)vec + v->len_ * v->elem_size_),
           v->elem_size_);
}
#endif

// vec_shift removes the first element of the vector and store it shifted
// if not null. Remaining elements are copied to their index - 1.
void vec_shift(Vec, void *);

#ifdef XSTD_IMPLEMENTATION
void vec_shift(Vec vec, void *shifted) {
  assert(vec != NULL);

  if (vec_isempty(vec))
    return;

  struct xstd_vector *v = headerof_vec(vec);
  if (shifted != NULL)
    memcpy(shifted, vec, v->elem_size_);

  v->len_--;

  memmove((void *)bodyof_vec(v),
          (void *)((uintptr_t)bodyof_vec(v) + v->elem_size_),
          v->len_ * v->elem_size_);
}
#endif

// vec_unshift_ adds an element at the beginning of the vector and shift
// other elements.
// An handy vec_unshift macro exists so you don't have to cast arguments and
// return type.
void *vec_unshift_(Vec *);

#ifdef XSTD_IMPLEMENTATION

void *vec_unshift_(void **vec) {
  assert(vec != NULL);
  assert(*vec != NULL);

  if (vec_isfull(*vec)) {
    // Let's double the capacity of our vector
    struct xstd_vector *v = headerof_vec(*vec);
    v->cap_ *= 2;
    *vec =
        (void *)bodyof_vec(alloc_realloc(v->allocator_, v, sizeof_vector(v)));
  }

  struct xstd_vector *v = headerof_vec(*vec);
  // Move all elements by 1
  memmove((void *)((uintptr_t)*vec + v->elem_size_), *vec,
          v->len_ * v->elem_size_);
  v->len_++;

  return *vec;
}
#endif

// vec_free free the given vector.
void vec_free(const Vec);

#ifdef XSTD_IMPLEMENTATION
void vec_free(Vec vec) {
  assert(vec != NULL);

  struct xstd_vector *v = headerof_vec(vec);
  alloc_free(v->allocator_, v);
}
#endif

// vec_reset resets vector length to 0.
void vec_reset(Vec);

#ifdef XSTD_IMPLEMENTATION
void vec_reset(Vec v) { headerof_vec(v)->len_ = 0; }
#endif

#define vec_resize(vec, new_size) vec_resize_((void **)vec, new_size)

// vec_resize_ resizes vector to the given capacity.
// An handy vec_resize macro exists so you don't have to cast arguments.
void vec_resize_(Vec *, size_t);

#ifdef XSTD_IMPLEMENTATION
void vec_resize_(void **vec, size_t cap) {
  struct xstd_vector *v = headerof_vec(*vec);
  v->cap_ = cap;
  *vec = (void *)bodyof_vec(alloc_realloc(v->allocator_, v, sizeof_vector(v)));
}
#endif

#define vec_iter_foreach(vec, iterator)                                        \
  for (struct {                                                                \
         short i;                                                              \
         VecIterator iter;                                                     \
       } tmp = {.i = 0, .iter = vec_iter(vec)};                                \
       tmp.i == 0; tmp.i = 1)                                                  \
  iter_foreach((Iterator *)&tmp.iter, typeof(vec), iterator)

struct xstd_vec_iterator_body {
  size_t cursor_;
  Vec vec_;
};

// VecIterator is a wrapper around Vec that implements the Iterator interface.
// VecIterator pointer can safely be casted to Iterator *.
typedef struct {
  Iterator iterator;
  struct xstd_vec_iterator_body body_;
} VecIterator;

#ifdef XSTD_IMPLEMENTATION
static void *vec_iter_next(void *body) {
  if (body == NULL)
    return NULL;

  struct xstd_vec_iterator_body *b = (struct xstd_vec_iterator_body *)body;
  void *result = NULL;

  struct xstd_vector *vec = headerof_vec(b->vec_);
  if (b->cursor_ < vec->len_) {
    result = (void *)((uintptr_t)(b->vec_) + b->cursor_ * vec->elem_size_);
    b->cursor_++;
  }

  return result;
}
#endif

struct xstd_iterator_vtable vec_iterator_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_iterator_vtable vec_iterator_vtable = {.next = &vec_iter_next};
#endif

// vec_iter creates a VecIterator that wraps the given vector.
VecIterator vec_iter(const Vec vec);

#ifdef XSTD_IMPLEMENTATION
VecIterator vec_iter(const Vec vec) {
  VecIterator iterator = {0};
  iterator.iterator.vtable_ = &vec_iterator_vtable;
  iterator.iterator.offset_ = offsetof(VecIterator, body_),
  iterator.body_.cursor_ = 0;
  iterator.body_.vec_ = vec;

  return iterator;
}
#endif

#endif
#ifndef XSTD_LIST_H_INCLUDE
#define XSTD_LIST_H_INCLUDE

#ifdef XSTD_IMPLEMENTATION
#include <stddef.h>
#include <stdint.h>
#endif


// typedef a List structure with the given name and of the given type.
#define typedef_list(type, name)                                               \
  typedef struct _xstd_##name {                                                \
    struct name *next_;                                                        \
    type value;                                                                \
  } name

#define list_prepend(list, element)                                            \
  do {                                                                         \
    type_assert_eq(*(list), element);                                          \
    list_prepend_((void **)list, (void *)element);                             \
  } while (0)

// list_prepend_ adds element to beginning of list, replacing its current head.
// An handy list_prepend macro exists so you don't have to cast arguments.
void list_prepend_(void **list, void *element);

#ifdef XSTD_IMPLEMENTATION
void list_prepend_(void **list, void *element) {
  void **element_next = (void **)element;
  *element_next = *list;
  *list = element;
}
#endif

#define list_next(list) ((typeof(list))list_next_(list))

void *list_next_(void *list);

#ifdef XSTD_IMPLEMENTATION
void *list_next_(void *list) {
  if (list == NULL)
    return NULL;
  return *((void **)list);
}
#endif

#define list_remove(list, element)                                             \
  do {                                                                         \
    type_assert_eq(list, element);                                             \
    list_remove_((void *)list, (void *)element);                               \
  } while (0)

// list_remove_ removes element from list but leave element untouched.
// An handy list_remove macro exists so you don't have to cast arguments.
void list_remove_(void *list, void *element);

#ifdef XSTD_IMPLEMENTATION
void list_remove_(void *list, void *element) {
  if (element == NULL)
    return;

  void **list_next = (void **)list;

  while (*list_next != NULL && *list_next != element) {
    list_next = (void **)*list_next;
  }

  *list_next = list_next_(element);
}
#endif

#define list_remove_next(list) (typeof(list))list_remove_next_(list)

// list_remove_next_ removes next element of the given list without altering the
// next element.
void *list_remove_next_(void *list);

#ifdef XSTD_IMPLEMENTATION
void *list_remove_next_(void *list) {
  void **list_next = (void **)list;
  void *next = *list_next;
  *list_next = list_next_(next);

  return next;
}
#endif

void *list_iter_next(void *list);

#ifdef XSTD_IMPLEMENTATION
void *list_iter_next(void *list) {
  // 2nd field of list iterator (the list itself).
  void **iterator_list_field_ptr = (void **)list;

  void *next = *iterator_list_field_ptr;
  *iterator_list_field_ptr = list_next_(*iterator_list_field_ptr);

  return next;
}
#endif

struct xstd_iterator_vtable list_iterator_vtable;

#ifdef XSTD_IMPLEMENTATION
struct xstd_iterator_vtable list_iterator_vtable = {
    .next = &list_iter_next,
};
#endif

#define typedef_list_iterator(list_type, type_name)                            \
  typedef struct {                                                             \
    Iterator iterator;                                                         \
    list_type *list_;                                                          \
  } type_name

#define fndef_list_iterator_init(list_iter_type, list_type, fn_name)           \
  list_iter_type fn_name(list_type *list);                                     \
  list_iter_type fn_name(list_type *list) {                                    \
    list_iter_type iter = {0};                                                 \
    iter.iterator.vtable_ = &list_iterator_vtable;                             \
    iter.iterator.offset_ = offsetof(list_iter_type, list_);                   \
    iter.list_ = list;                                                         \
    return iter;                                                               \
  }

#endif
