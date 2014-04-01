#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include "rvm_core.h"

using namespace std;

std::map<const char*, char, cmpStr> *instructionList = NULL;

char GetInstructionByName(const char *instr)
{
  if(instructionList->find(instr) != instructionList->end()) return instructionList->at(instr);
  return 0;
}

int ExpandBytes(char **ptr, int currentLength)
{
  char *newData = new char[currentLength*2];
  memcpy(newData, *ptr, currentLength);
  delete[] (*ptr);
  (*ptr) = newData;
  return currentLength*2;
}

void ExpandIfNeeded(char **ptr, int *size, int currentLength, int toAdd)
{
  if(currentLength + toAdd >= *size)
  {
    (*size) = ExpandBytes(ptr, *size);
  }
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

void VM::ExpandStack(int sz)
{
  int offset = currentFrame - stackFrame;
  ExpandIfNeeded(&stackFrame, &stackFrameSize, currentFrame - stackFrame, currentFrameSize + sz);
  currentFrame = stackFrame + offset;
}

void VM::execute(char *bytecode, int size)
{
  beforeJmpPtr = NULL;
  instPtr = bytecode; //place at beginning

  while(instPtr >= bytecode && instPtr < bytecode + size)
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
      case INST_PUSHA: //on stack
      {
        int addr = (int)*((unsigned char*)(instPtr+1)); //actually a unsigned char
        int value = BYTES2INT(currentFrame+sizeof(FrameHeader)+addr);
        instPtr += 2;
        push(value);
        break;
      }
      case INST_POPA: //on stack
      {
        int addr = (int)*((unsigned char*)(instPtr+1)); //actually a unsigned char
        int value = pop();
        instPtr += 2;
        INT2BYTES(value, (&currentFrame[sizeof(FrameHeader)+addr]));
        break;
      }
      case INST_PUSHC:
      {
        int addr = BYTES2INT(instPtr+1);
        push(addr);
        instPtr += 5;
        break;
      }
      case INST_JMP:
      {
        int addr = BYTES2INT(instPtr + 1);
        beforeJmpPtr = instPtr + 5;
        instPtr = &bytecode[addr];
        break;
      }
      case INST_ADDS:
      {
        push(pop() + pop());
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
      case INST_PUSHFRAME:
      {
        ExpandStack((int)sizeof(FrameHeader));
        FrameHeader newFrame;
        newFrame.savedPtr = beforeJmpPtr;
        newFrame.savedSize = currentFrameSize;
        newFrame.prevFrame = currentFrame;
        char *newLoc = currentFrame + currentFrameSize;
        memcpy(newLoc, &newFrame, sizeof(FrameHeader));
        currentFrame = newLoc;
        currentFrameSize = (int)sizeof(FrameHeader);
        instPtr++;
        break;
      }
      case INST_POPFRAME:
      {
        FrameHeader *header = (FrameHeader*)currentFrame;
        instPtr = header->savedPtr;
        currentFrame = header->prevFrame;
        currentFrameSize = header->savedSize;
        if(currentFrame == stackFrame)
        {
          //end execution
          printf("\nExecution completed\n");
          return;
        }

        break;
      }
      case INST_PUSHVAR:
      {
        ExpandStack(4);
        *(int*)(&currentFrame[currentFrameSize]) = 0; //zeros out variables to be nice
        currentFrameSize += 4;
        instPtr++;
        break;
      }
      default:
      {
        throw runtime_error("Invalid Instruction");
        break;
      }
    }

  }
}

