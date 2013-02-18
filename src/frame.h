#pragma once
/*
 * TODO - change tokens
 * TODO - on implementation side, use rtti field to call proper template copy
 *        functions.
 */

/*
 * NOTES
 * -----
 * - preliminary implementation doesn't uses neither "translate" nor
 *   serialization. So they're basically untested.  It's also not clear
 *   how useful they are.
 */

/*
 * Messages are meant to describe formated data passing through a queue that
 * mediates communication between two threads.
 *
 * ## USAGE ##
 *
 * The assumed usage pattern goes something like this:
 *
 *  0. Memory is managed by the queue.  It's assumed that the queue provides an
 *     allocated storage space for the entire Message.
 *
 *     Importantly, this means **data storage is allocated outside this object.**
 *
 *     However, the formating of the buffer is up to the Message object.
 *     Typically data will be organized as:
 *
 *     buffer:
 *     [ message object | data .......................... ]
 *     ^                ^
 *     0                sizeof( message object )
 *
 *     The message object acts as a header with the appropriate formatting
 *     data.
 *
 *
 *  Producer side
 *  -------------
 *  The producer knows how to format the data.  It's job is to compute the
 *  sizes of things, allocate the queue and fill it up.
 *
 *  1. Compute the required size for the queue buffer, and alloc the queue.
 *      a. Compute format parameters (e.g. width, height, etc...)
 *      b. Create a "dummy" instance (i.e. an instance with correct format
 *         metadata but with this->data == NULL)
 *      c. Call size_bytes()
 *  2. Initialize the data buffer.
 *      a. Cast the buffer pointer to the appropriate Message type.
 *      b. Call format( ... ) with the appropriate arguments.
 *         This will fill in the parameter fields and set the data pointer.
 *  3. Fill in the data.
 *
 *  Consumer side
 *  -------------
 *  The consumer knows what format it wants the data in.  It's job is to
 *  take a formated buffer off a queue, translate/cast it to the appropriate
 *  Message subclass, and use the data.
 *
 *  1. Cast the raw data pointer to a Message pointer.
 *  2. Call translate() using the appropriate format id.
 *  3. Cast the returned pointer to a pointer for the desired Message subclass.
 *  4. ...
 *  5. Profit
 *
 * ## SERIALIZING ##
 *
 *  Any instance of a class deriving from Message can be written to file.  The
 *  file assumed to consist of a series of messages separated by a few bytes
 *  describing the size of each chunk.
 *
 *  Reading from a file/byte stream
 *  -------------------------------
 *  There are a couple different methods.  Both revolve around how memory is
 *  allocated for the receiving buffer.
 *
 *  A. Determine required buffer size, allocate and read.
 *     0. Get the required size with: sz = Message::from_file(file,NULL,0);
 *     1. Allocate the buffer.
 *     2. Call from_file a second time.  This time it should return 0.
 *        Message::from_file( file, buffer, sz );
 *
 *  B. Determine max buffer size on first pass through file and push onto a
 *     queue.
 *     0. With maxsize = 0
 *        while not end of file
 *           sz = Message::from_file(file,NULL,0); // get size of this message
 *           fseek(file, sz, SEEK_CUR);            // jump to next message
 *           maxsize = max( maxsize, sz );         // update the max size
 *     1. Rewind the file
 *     2. Allocate a queue with buffers of size "maxsize"
 *     3. Allocate a token buffer for the queue
 *     4. Call Message::from_file(file, buffer, buffersize ).
 *        It should return zero this time.
 *     5. Push token buffer onto the queue.
 *     6. Repeat 4-5 till end of file.
 *
 *
 *  Now it can be shipped off (pushed on a queue, for example) and translated
 *  by a consumer.
 *
 *  Writing to a file/byte stream
 *  -----------------------------
 *  With a Message*, m, and a FILE*, fp, call:
 *
 *    m->to_file(fp);
 *
 *  Compatibility Issues 
 *  --------------------
 *
 *  I think most of these issues could be accounted for with an appropriate
 *  file-level header that specifies endianness, whether the system is 32 or
 *  64 bits, and an overall version identifier.
 *
 *  - endianness is not accounted for.
 *
 *  - Pointers are written to disk.
 *
 *    On 64 bit systems these pointers will have a different size than on 32
 *    bit systems.  The Message should get read in entirely, but translation
 *    will likely fail since, effectively, the format will be inaccurate.
 *
 *    However, with a mechanism to detect 32 vs. 64 bit compatibility, a
 *    translation mechanism could probably be put into place.
 *
 *  - If a format's identifier changes, messages written beforehand will be
 *    incompatible.
 *
 *  Translate
 *  ---------
 *  
 *  Compatible messages are transformed into the calling Message sub-type.
 *  Minimally, this is a copy operation.
 *
 *  Returns the size required of the destination message buffer.
 *
 *  If the pointer to the destination buffer is NULL, the function will 
 *  restrict itself to just computing the size.
 */

#include "types.h"

namespace fetch{
  
  
  enum MessageFormatID
  { FORMAT_INVALID           = 0,
    FRAME_INTERLEAVED_PLANES = 1,
    FRAME_INTERLEAVED_LINES,
    FRAME_INTERLEAVED_PIXELS,
  };
  
  
  class Message
  { public:
      MessageFormatID id;
      size_t          self_size;
      void           *data;
      //Message        *next;
  
      Message(void) : id(FORMAT_INVALID), self_size(sizeof(Message)), data(NULL) {}
      Message(MessageFormatID id, size_t self_size) : id(id), self_size(self_size), data(NULL) {}
  
             void      to_file    ( FILE *fp );
             void      to_file    ( HANDLE hfile );
      static size_t    from_file  ( FILE *fp,     Message* workspace, size_t size_workspace);
      static size_t    from_file  ( HANDLE hfile, Message* workspace, size_t size_workspace);
  
      static size_t    copy_data  ( Message *dst, Message *src ); // returns #bytes copied
  
      // Override these in implementing classes.
      virtual size_t   size_bytes ( void ) = 0;
      virtual void     format     ( Message *unformatted ) = 0;           // This should simply copy the format metadata from "this" to "unformatted."
      static  size_t   translate  ( Message *dst, Message *src );         // The calling class determines the destination type
    
                void   cast(void);  // Update the virtual table according to the type id.  
    private:
      template<class MT> void __cast(void) { MT eg; memcpy(this,&eg,sizeof(void*));} // overwrites the virtual table pointer with the example's pointer
  };
  
  class FrmFmt : public Message
  { public:
      u32           width;
      u32           height;
      u8            nchan;
      u8            Bpp;
      Basic_Type_ID rtti;
  
      FrmFmt(void);
      FrmFmt(u32 width, u32 height, u8 nchan, Basic_Type_ID type);
      FrmFmt(u32 width, u32 height, u8 nchan, Basic_Type_ID type, MessageFormatID id, size_t self_size);
  
      unsigned int   is_equivalent( FrmFmt *ref );
      void           dump( const char *filename,... );                    ///< performs printf-style string formating on filename.  Thread-safe.
  
              size_t size_bytes      ( void );
              void   format          ( Message *unformatted );
  };
  
  class Frame : public FrmFmt
  { public:
      Frame(void)                                                                                      {}
      Frame(u32 width, u32 height, u8 nchan, Basic_Type_ID type)                                       : FrmFmt(width,height,nchan,type) {}
      Frame(u32 width, u32 height, u8 nchan, Basic_Type_ID type, MessageFormatID id, size_t self_size) : FrmFmt(width,height,nchan,type,id,self_size) {}
  
  
      virtual void   copy_channel    ( void *dst, size_t rowpitch, size_t ichan ) = 0;
      virtual void   compute_pitches ( size_t pitch[4] )                          = 0;
      virtual void   get_shape       ( size_t n[3] )                              = 0;
      
      void           totif           ( const char *filename, ... );       ///< performs printf-style string formating on filename.  Thread-safe.
      // Children also need to impliment (left over from Message):
      //             translate()
      //             format()    - but only if formatting data is added
  };
  
  class Frame_With_Interleaved_Pixels : public Frame
  { public:
      Frame_With_Interleaved_Pixels(void) {}
      Frame_With_Interleaved_Pixels(u32 width, u32 height, u8 nchan, Basic_Type_ID type);
  
                void   copy_channel ( void *dst, size_t rowpitch, size_t ichan );
      static  size_t   translate    ( Message *dst, Message *src );
      
      virtual void   compute_pitches ( size_t pitch[4] );
      virtual void   get_shape       ( size_t n[3] );
  };
  
  class Frame_With_Interleaved_Lines : public Frame
  { public:
      Frame_With_Interleaved_Lines(void) {}
      Frame_With_Interleaved_Lines(u32 width, u32 height, u8 nchan, Basic_Type_ID type);
  
                void   copy_channel ( void *dst, size_t rowpitch, size_t ichan );
      static  size_t   translate    ( Message *dst, Message *src );
      
      virtual void   compute_pitches ( size_t pitch[4] );
      virtual void   get_shape       ( size_t n[3] );
  };
  
  class Frame_With_Interleaved_Planes : public Frame
  { public:
      Frame_With_Interleaved_Planes(void) {}
      Frame_With_Interleaved_Planes(u32 width, u32 height, u8 nchan, Basic_Type_ID type);
  
                void   copy_channel ( void *dst, size_t rowpitch, size_t ichan );
      static  size_t   translate    ( Message *dst, Message *src );
      
      virtual void   compute_pitches ( size_t pitch[4] );
      virtual void   get_shape       ( size_t n[3] );
  };

} // end namespace fetch
