#ifndef _RVM_CORE
#define _RVM_CORE

typedef struct _Symbol
{
  char name[32];
  unsigned int address;
} Symbol;


enum INST
{
  INST_ADDS       = 0x05,
  INST_SUBS       = 0x06,
  INST_MULTS      = 0x07,
  INST_DIVS       = 0x08,
  INST_ADDSF      = 0x09,
  INST_SUBSF      = 0x0A,
  INST_MULTSF     = 0x0B,
  INST_DIVSF      = 0x0C,
  INST_PRINT      = 0x10,
  INST_JMP        = 0x11,
  INST_PUSH       = 0x12, //push raw value
  INST_POP        = 0x13,
  INST_PUSHFRAME  = 0x14, //push stack frame
  INST_POPFRAME   = 0x15,
  INST_PUSHA      = 0x16, //push from an address
  INST_POPA       = 0x17, //pop into an address
  INST_PUSHC      = 0x18, //global constants
  INST_PUSHVAR    = 0x19  //puts a variable on the stack frame
};

extern int ExpandBytes(char **ptr, int currentLength);

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

class VM
{
public:
#define INITIAL_FRAME_SIZE 128

  VM() : stackSize(0)
  {
    stackFrame = new char[INITIAL_FRAME_SIZE];
    stackFrameSize = 0;
    currentFrame = stackFrame;
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

  char *instPtr;
  char *lastInstPtr;
};


#endif
