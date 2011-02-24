#pragma once


namespace w32file {
  i64 getpos( HANDLE hf );                        // returns -1 if past end of file, otherwise returns current offset from beginning
  i64 setpos( HANDLE hf, i64 pos, DWORD method ); // returns -1 if move puts offset past end of file, otherwise returns offset from beginning after move
  int eof   ( HANDLE hf );                        // returns 1 if current position is past end of file, otherwise 0.
}
