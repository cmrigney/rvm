#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include "rvm_core.h"

using namespace std;

int ExpandBytes(char **ptr, int currentLength)
{
  char *newData = new char[currentLength*2];
  memcpy(newData, *ptr, currentLength);
  delete[] (*ptr);
  (*ptr) = newData;
  return currentLength*2;
}

void VM::push(int value)
{
  if(stackSize >= MAX_STACK) throw runtime_error("Stack Overflow Exception");
  stack[stackSize++] = value;
}

int VM::pop()
{
  if(stackSize <= 0) throw runtime_error("Stack Underflow Exception");
  return stack[--stackSize];
}

void VM::execute(char *bytecode, int size)
{
  lastInstPtr = NULL;
  instPtr = bytecode; //place at beginning

  while(instPtr > bytecode && instPtr < bytecode + size)
  {
    char instruction = *instPtr;
    switch(instruction)
    {
      case INST_PUSH:
      {
        int value = BYTES2INT(instPtr + 1);
        instPtr +=5;
        push(value);
        break;
      }
      case INST_POP:
      {
        pop();
        instPtr++;
        break;
      }
      case INST_PUSHA:
      {
        int addr = (int)*((unsigned char*)(instPtr+1)); //actually a unsigned char
        int value = BYTES2INT(currentFrame+addr);
        instPtr += 2;
        push(value);
        break;
      }
      case INST_POPA:
      {
        int addr = (int)*((unsigned char*)(instPtr+1)); //actually a unsigned char
        int value = pop();
        instPtr += 2;
        INT2BYTES(value, (&currentFrame[addr]));
        break;
      }
      case INST_JMP:
      {
        int addr = BYTES2INT(instPtr + 1);
        instPtr = &bytecode[addr];
        break;
      }
      case INST_ADDS:
      {
        push(pop()+ pop());
        instPtr++;
        break;
      }
      case INST_PRINT:
      {
        int ptr = pop();
        const char *data = &bytecode[ptr];
        printf("%s", data);
        instPtr++;
        break;
      }
      default:
      {
        throw runtime_error("Invalid Instruction");
        break;
      }
    }

    lastInstPtr = instPtr;
  }
}

