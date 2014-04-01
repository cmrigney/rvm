#ifndef _RVM_CORE
#define _RVM_CORE

#include <stdarg.h>
#include <stdio.h>
#include <map>

typedef struct _Symbol
{
  char name[32];
  unsigned int address;
} Symbol;

struct cmpStr
{
  bool operator()(const char *a, const char *b) const
  {
    return strcmp(a, b) < 0;
  }
};

extern std::map<const char*, char, cmpStr> *instructionList;

class InstructionAdder
{
public:
  InstructionAdder(const char *name, char value)
  {
    if(instructionList == NULL) instructionList = new std::map<const char*, char, cmpStr>();
    (*instructionList)[name] = value;
  }
  ~InstructionAdder()
  {
    if(instructionList != NULL)
    {
      delete instructionList;
      instructionList = NULL;
    }
  }
};

#define INSTRUCTION(name,value) const char name = value; const InstructionAdder name##_lister(#name, value);

//BEGIN INSTRUCTIONS

INSTRUCTION(INST_NOP        , 0x04)
INSTRUCTION(INST_ADDS       , 0x05)
INSTRUCTION(INST_SUBS       , 0x06)
INSTRUCTION(INST_MULTS      , 0x07)
INSTRUCTION(INST_DIVS       , 0x08)
INSTRUCTION(INST_ADDSF      , 0x09)
INSTRUCTION(INST_SUBSF      , 0x0A)
INSTRUCTION(INST_MULTSF     , 0x0B)
INSTRUCTION(INST_DIVSF      , 0x0C)
INSTRUCTION(INST_PRINT      , 0x10)
INSTRUCTION(INST_JMP        , 0x11)
INSTRUCTION(INST_PUSH       , 0x12) //push raw value
INSTRUCTION(INST_POP        , 0x13)
INSTRUCTION(INST_PUSHFRAME  , 0x14) //push stack frame
INSTRUCTION(INST_POPFRAME   , 0x15)
INSTRUCTION(INST_PUSHA      , 0x16) //push from an address
INSTRUCTION(INST_POPA       , 0x17) //pop into an address
INSTRUCTION(INST_PUSHC      , 0x18) //global constants
INSTRUCTION(INST_PUSHVAR    , 0x19) //puts a variable on the stack frame

//END INST

//enum INST
//{
//  INST_ADDS       = 0x05,
//  INST_SUBS       = 0x06,
//  INST_MULTS      = 0x07,
//  INST_DIVS       = 0x08,
//  INST_ADDSF      = 0x09,
//  INST_SUBSF      = 0x0A,
//  INST_MULTSF     = 0x0B,
//  INST_DIVSF      = 0x0C,
//  INST_PRINT      = 0x10,
//  INST_JMP        = 0x11,
//  INST_PUSH       = 0x12, //push raw value
//  INST_POP        = 0x13,
//  INST_PUSHFRAME  = 0x14, //push stack frame
//  INST_POPFRAME   = 0x15,
//  INST_PUSHA      = 0x16, //push from an address
//  INST_POPA       = 0x17, //pop into an address
//  INST_PUSHC      = 0x18, //global constants
//  INST_PUSHVAR    = 0x19 //puts a variable on the stack frame
//};

extern int ExpandBytes(char **ptr, int currentLength);
extern char GetInstructionByName(const char *inst);

static inline int BYTES2INT(char *c)
{
  int a = 0;
  a = (a << 8) + c[0];
  a = (a << 8) + c[1];
  a = (a << 8) + c[2];
  a = (a << 8) + c[3];
  return a;
}

static inline void INT2BYTES(int i, char *c)
{
  c[0] = (i >> 24) & 0xff;
  c[1] = (i >> 16) & 0xff;
  c[2] = (i >> 8) & 0xff;
  c[3] = i & 0xff;
}

typedef struct _FrameHeader
{
  char *savedPtr;
  int savedSize;
  char *prevFrame;
} FrameHeader;

class VM
{
public:
#define INITIAL_FRAME_SIZE 128

  VM() : stackSize(0)
  {
    stackFrame = new char[INITIAL_FRAME_SIZE];
    stackFrameSize = INITIAL_FRAME_SIZE;
    currentFrame = stackFrame;
    currentFrameSize = 0;
  }

  ~VM()
  {
    delete[] stackFrame;
  }

  void push(int value);
  int pop();

  void execute(char *bytecode, int size);

private:
  static const int MAX_STACK = 128;

  int stackSize;
  int stack[MAX_STACK];

  char *stackFrame;
  int stackFrameSize;
  char *currentFrame;
  int currentFrameSize;

  char *instPtr;
  char *beforeJmpPtr;

  void ExpandStack(int sz);
};


#ifdef _MSC_VER

#define snprintf c99_snprintf

inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

inline int c99_snprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}



#endif // _MSC_VER

#endif
