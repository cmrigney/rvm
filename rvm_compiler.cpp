#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdexcept>
#include <vector>
#include "rvm_core.h"
#include "rvm_tokenmap.h"

using namespace std;

struct _Token
{
  TokenType type;
  const char *str;
  int length;
  int padding;
  bool operator==(const _Token& a) const
  {
    return (type == a.type && str == a.str && length == a.length && padding == a.padding);
  }
};

typedef struct _Token Token;

struct _FunctionSig
{
  Token returnToken;
  Token symbolToken;
  vector<Token> argTokens;
  vector<Token> argTypeTokens;
  _FunctionSig(Token rToken, Token sToken, vector<Token> aTypeTokens, vector<Token> aTokens) : returnToken(rToken), symbolToken(sToken), argTypeTokens(aTypeTokens), argTokens(aTokens)
  {
  }
  bool operator==(const _FunctionSig& a) const
  {
    return (symbolToken == a.symbolToken); //sufficient
  }
};

typedef struct _FunctionSig FunctionSig;

typedef struct _VariableInfo
{
  TokenType type;
  char *name;
  int codeIndex;
  ~_VariableInfo() { delete[] name; }
} VariableInfo;

bool lastCompileWasError = false;

void CompileCodeInternal(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, vector<VariableInfo> localSymbols);
void CompileCodeInternal(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength);
void CompileExpression(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, int *consumedTokens, vector<VariableInfo> *localSymbols);
void CompileExpression(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, int *consumedTokens, vector<VariableInfo> *localSymbols, bool stopAfterOne);

bool compareStringsMap(char *a, char *b)
{
  return strcmp(a, b) == 0;
}

bool compareIntsMap(int a, int b)
{
  return a == b;
}

ExactMap<char*, int> symbolLocation(compareStringsMap);
ExactMap<int, char*> jmpToFill(compareIntsMap);
vector<FunctionSig> symbolDefines;

ExactMap<int, char*> stringsToFill(compareIntsMap);

void SyntaxError(const char* error)
{
  printf("Syntax Error: %s\n", error);
  lastCompileWasError = true;
  throw runtime_error("Syntax Error");
}

vector<char> PreProcessCode(const char *code)
{
  int codeLen = strlen(code);
  vector<char> newCode;
  bool inComment = false;
  for(int i1 = 0; i1 < codeLen; i1++)
  {
    if(inComment && code[i1] == '\n')
    {
      inComment = false;
      if(i1 > 0 && code[i1-1] == '\r') newCode.push_back('\r');
    }
    else if(!inComment)
    {
      if(code[i1] == '/' && (i1 + 1 < codeLen && code[i1+1] == '/')) inComment = true;
    }

    if(inComment)
    {
      continue;
    }

    newCode.push_back(code[i1]);
  }
  return newCode;
}

Token CreateToken(const char *ptr)
{
  Token result;
  result.padding = 0;
  while(isspace(*ptr))
  {
    ptr++;
    result.padding++;
  }
  if(AnyWhereTokens.contains(ptr))
  {
    if(AnyWhereTokens[ptr] == TOKEN_QUOTE)
    {
      if(*(ptr - 1) == '\\') SyntaxError("Unrecongnized sequence: \\\"");
      const char *tptr = ptr + 1;
      while((*tptr != '\"' && *tptr != '\0') || ((*tptr == '\"' && (*(tptr - 1) == '\\')))) tptr++;
      if(*tptr == '\0') SyntaxError("No ending quote");
      result.type = TOKEN_STRING;
      result.str = ptr;
      result.length = tptr - ptr + 1; //include end quote
      return result;
    }

    result.type = AnyWhereTokens[ptr];
    result.length = strlen(AnyWhereTokensReverse[result.type]);
    result.str = ptr;
    return result;
  }
  else if(KeywordTokens.contains(ptr))
  {
    result.type = KeywordTokens[ptr];
    result.length = strlen(KeywordTokensReverse[result.type]);
    result.str = ptr;
    return result;
  }
  else if(isdigit(*ptr) || (*ptr == '.' && isdigit(*(ptr+1))))
  {
    result.type = TOKEN_NUMBER;
    const char *tptr = ptr + 1;
    result.str = ptr;
    while(!isspace(*tptr) && (*tptr != '\0')  && !AnyWhereTokens.contains(tptr)) tptr++;
    result.length = tptr - ptr;
    return result;
  }
  else //must be a symbol whether var or func
  {
    result.type = TOKEN_SYMBOL;
    const char *tptr = ptr + 1;
    result.str = ptr;
    while(!isspace(*tptr) && (*tptr != '\0')  && !AnyWhereTokens.contains(tptr)) tptr++;
    result.length = tptr - ptr;
    return result;
  }
}

vector<Token> Tokenize(const char *code)
{
  vector<Token> result;
  int codeLen = strlen(code);
  for(int i1 = 0; i1 < codeLen; )
  {
    Token token = CreateToken(&code[i1]);
    result.push_back(token);
    i1 += token.length + token.padding;
  }
  return result;
}


void PrintToken(Token token, const char *offset)
{
  printf("------------------\n");
  printf("Token Type: %d\nToken Offset: %d\nToken Length: %d\nToken Start String: %s\n", token.type, token.str - offset, token.length, token.str);
}

static inline void PrepareForWrite(char **bytecode, int *bytecodeLength, int *workingOffset, int sizeToBeWritten)
{
  while(*workingOffset + sizeToBeWritten > *bytecodeLength)
  {
    (*bytecodeLength) = ExpandBytes(bytecode, *bytecodeLength);
  }
}

vector<char> CompileStatement(vector<Token> statement)
{
  if(statement.size() == 0) return vector<char>();
  if(statement[statement.size()-1].type != TOKEN_ENDSTATEMENT)
    SyntaxError("Did not end statement");

}

static inline bool VariableDeclared(vector<VariableInfo> &lst, VariableInfo info)
{
  for(int i1 = 0; i1 < lst.size(); i1++)
  {
    if(strcmp(lst[i1].name, info.name) == 0) return true;
  }
 return false;
}

static inline VariableInfo *LookupVariable(vector<VariableInfo> &lst, char *name)
{
  for(int i1 = 0; i1 < lst.size(); i1++)
  {
    if(strcmp(lst[i1].name, name) == 0) return &lst[i1];
  }
  return NULL;
}

static inline int LookupVariableIndex(vector<VariableInfo> &lst, char *name)
{
  for(int i1 = 0; i1 < lst.size(); i1++)
  {
    if(strcmp(lst[i1].name, name) == 0) return i1;
  }
  return -1;
}

static inline bool IsFunctionDeclaration(Token *tokens, int tokenLength)
{
  if(tokenLength  >= 5)
  {
    if(TokenIsDataType(tokens[0].type) &&
       tokens[1].type == TOKEN_SYMBOL &&
       tokens[2].type == TOKEN_LEFTPAREN
      )
    {
      return true;
    }
  }
  return false;
}

void AddSymbol(FunctionSig sig)
{
  for(int i1 = 0; i1 < symbolDefines.size(); i1++)
  {
    if(symbolDefines[i1] == sig)
    {
      char temp[128];
      char temp2[36];
      snprintf(temp2, (sig.symbolToken.length > 32 ? 32 : sig.symbolToken.length), "%s", sig.symbolToken.str);
      snprintf(temp, 128, "Multiple definitions of %s", temp2);
      SyntaxError((const char*) temp);
    }
  }
  symbolDefines.push_back(sig);
}

bool HandleFunctionDeclaration(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, int *consumedTokens)
{
  if(!IsFunctionDeclaration(tokens, tokenLength)) return false;
  vector<Token> args, argTypes;
  Token *ret, *sym;
  ret = &tokens[0];
  sym = &tokens[1];
  bool endFound = false;
  int i1 = 3;
  for(; i1 < tokenLength - 2; ) //because 2 is left paren
  {
    if(tokens[i1].type == TOKEN_RIGHTPAREN && tokens[i1+1].type == TOKEN_LEFTBRACKET) { endFound = true; break; }
    //look for first symbol
    if(!TokenIsDataType(tokens[i1].type) || tokens[i1+1].type != TOKEN_SYMBOL)
    {
      i1++;
      continue;
    }
    args.push_back(tokens[i1+1]);
    argTypes.push_back(tokens[i1]);
    i1 += 2;
  }
  if(!endFound) SyntaxError("No end parenthesis found for function");

  AddSymbol(FunctionSig(*ret, *sym, argTypes, args));

  i1++; //to get into the function
  int startOffset = i1;
  int inBracket = 0;
  int outBracket = 0;
  int totalTokens = 0;
  for(; i1 < tokenLength; i1++)
  {
    if(tokens[i1].type == TOKEN_LEFTBRACKET) inBracket++;
    else if(tokens[i1].type == TOKEN_RIGHTBRACKET) outBracket++;

    totalTokens++;

    if(inBracket == outBracket) break;
  }
  if(inBracket != outBracket) SyntaxError("No end bracket");

  char *str = new char[sym->length+1];
  strncpy(str, sym->str, sym->length);
  str[sym->length] = '\0';
  symbolLocation.set(str, *workingOffset); //set symbol location here

  PrepareForWrite(bytecode, bytecodeLength, workingOffset, 4);
  (*bytecode)[(*workingOffset)++] = INST_PUSHFRAME;

  //POP ARGS OFF STACK FOR DEFINING LOCAL SYMBOLS, MAYBE ADD PARAMETER FOR COMPILECODEINTERNAL

  CompileCodeInternal(bytecode, bytecodeLength, workingOffset, tokens + startOffset, totalTokens, vector<VariableInfo>()); //define a vector for variables

  PrepareForWrite(bytecode, bytecodeLength, workingOffset, 4);
  (*bytecode)[(*workingOffset)++] = INST_POPFRAME;

  (*consumedTokens) += totalTokens + startOffset; //startOffset has arg tokens and stuff

  return true;
}

static inline bool IsVariableAssignment(Token *tokens, int tokenLength)
{
  if(tokenLength  >= 2)
  {
    if(tokens[0].type == TOKEN_SYMBOL &&
       tokens[1].type == TOKEN_ASSIGNMENT)
    {
      return true;
    }
  }
  return false;
}

bool HandleVariableAssignment(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, int *consumedTokens, vector<VariableInfo> *localSymbols)
{
  if(!IsVariableAssignment(tokens, tokenLength)) return false;
  char *name = new char[tokens[0].length+1];
  strncpy(name, tokens[0].str, tokens[0].length);
  name[tokens[0].length] = '\0';
  VariableInfo *info;
  if(!(info = LookupVariable(*localSymbols, name))) SyntaxError("Variable used but not declared");

  (*consumedTokens) += 2;

  int totalTokens = 0;
  bool foundEnd = false;
  for(int i1 = 0; i1 < tokenLength; i1++)
  {
    totalTokens++;
    if(tokens[i1].type == TOKEN_ENDSTATEMENT)
    {
      foundEnd = true;
      break;
    }
  }
  if(!foundEnd) SyntaxError("No end to assignment");

  CompileExpression(bytecode, bytecodeLength, workingOffset, tokens, totalTokens, consumedTokens, localSymbols);

  unsigned char idx = (unsigned char)LookupVariableIndex(*localSymbols, name);

  PrepareForWrite(bytecode, bytecodeLength, workingOffset, 5);
  (*bytecode)[(*workingOffset)++] = INST_POPA;
  (*bytecode)[(*workingOffset)++] = *((char*)&idx);

  return true;
}

static inline bool IsFunctionCall(Token *tokens, int tokenLength)
{
  if(tokenLength  >= 3)
  {
    if(tokens[0].type == TOKEN_SYMBOL &&
      tokens[1].type == TOKEN_LEFTPAREN)
    {
      return true;
    }
  }
  return false;
}

bool HandleFunctionCall(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, int *consumedTokens, vector<VariableInfo> *localSymbols)
{
  if(!IsFunctionCall(tokens, tokenLength)) return false;

  (*consumedTokens) += 2;

  int runningTokens = 0;
  for(int i1 = 2; i1 < tokenLength; i1++)
  {
    if(tokens[i1].type == TOKEN_RIGHTPAREN)
    {
      (*consumedTokens)++;
      break;
    }
    else if(tokens[i1].type == TOKEN_COMMA)
    {
      (*consumedTokens)++;
      if(runningTokens == 0) SyntaxError("No argument specified");
      CompileExpression(bytecode, bytecodeLength, workingOffset, &tokens[i1 - runningTokens], runningTokens, consumedTokens, localSymbols); //will push on stack
      runningTokens = 0;
    }
    runningTokens++;
  }

  PrepareForWrite(bytecode, bytecodeLength, workingOffset, 5);
  (*bytecode)[(*workingOffset)++] = INST_JMP;
  char *name = new char[tokens[0].length + 1];
  strncpy(name, tokens[0].str, tokens[0].length);
  name[tokens[0].length] = '\0';
  jmpToFill.set((*workingOffset), name);
  (*workingOffset) += 4;

  return true;
}

static inline bool IsVariableDeclaration(Token *tokens, int tokenLength)
{
  if(tokenLength  >= 3)
  {
    if(TokenIsDataType(tokens[0].type) &&
       tokens[1].type == TOKEN_SYMBOL &&
      (tokens[2].type == TOKEN_ENDSTATEMENT || tokens[2].type == TOKEN_ASSIGNMENT))
    {
      return true;
    }
  }
  return false;
}


bool HandleVariableDeclaration(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, int *consumedTokens, vector<VariableInfo> *localSymbols)
{
  if(!IsVariableDeclaration(tokens, tokenLength)) return false;

  VariableInfo var;
  var.type = tokens[0].type;
  char *name = new char[tokens[0].length+1];
  strncpy(name, tokens[0].str, tokens[0].length);
  name[tokens[0].length] = '\0';
  var.name = name;
  PrepareForWrite(bytecode, bytecodeLength, workingOffset, 4);
  var.codeIndex = *workingOffset;
  if(VariableDeclared(*localSymbols, var)) SyntaxError("Variable declared more than once");
/*
  (*workingOffset) += 4; //for 4 byte variable lengths //don't put this in code
*/
  localSymbols->push_back(var);
  (*bytecode)[(*workingOffset)++] = INST_PUSHVAR;

  (*consumedTokens) += 2;

  if(tokens[2].type == TOKEN_ENDSTATEMENT) return true;

  (*consumedTokens)--; //workaround for the following
  if(!HandleVariableAssignment(bytecode, bytecodeLength, workingOffset, tokens + 1, tokenLength - 1, consumedTokens, localSymbols)) SyntaxError("Unknown variable operation");

  return true;
}


void CompileExpression(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, int *consumedTokens, vector<VariableInfo> *localSymbols)
{
  CompileExpression(bytecode, bytecodeLength, workingOffset, tokens, tokenLength, consumedTokens, localSymbols, false);
}

void CompileExpression(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, int *consumedTokens, vector<VariableInfo> *localSymbols, bool stopAfterOne)
{
  if(tokenLength == 0) SyntaxError("Invalid expression.  Cannot be empty");

  for(int i1 = 0; i1 < tokenLength; i1++)
  {
    if(stopAfterOne && i1 >= 1) return;

    (*consumedTokens)++;

    if(tokens[i1].type == TOKEN_ENDSTATEMENT) return;
    else if(tokens[i1].type == TOKEN_LEFTPAREN)
    {
      int leftParen = 0;
      int rightParen = 0;
      int totalToks = 0;
      int i2 = i1;
      for(; i2 < tokenLength; i2++)
      {
        if(tokens[i2].type == TOKEN_LEFTPAREN) leftParen++;
        else if(tokens[i2].type == TOKEN_RIGHTPAREN) rightParen++;
        totalToks++;

        if(rightParen == leftParen) break;
      }
      if(i2 == tokenLength) SyntaxError("No end parenthesis in expression");
      totalToks -= 2; //since we don't want end parens
      CompileExpression(bytecode, bytecodeLength, workingOffset, tokens + i1 + 1, totalToks, consumedTokens, localSymbols);
      (*consumedTokens)++; //for end paren
    }
    else if(tokens[i1].type == TOKEN_NUMBER) //someday change this to a const instead of always pushing //ALSO THIS ALWAYS ASSUMES INT FOR NOW
    {
      PrepareForWrite(bytecode, bytecodeLength, workingOffset, 5);
      (*bytecode)[(*workingOffset)++] = INST_PUSH;
      char temp[32];
      strncpy(temp, tokens[i1].str, tokens[i1].length);
      temp[tokens[i1].length] = '\0';
      int number = atoi(temp);
      INT2BYTES(number, &((*bytecode)[*workingOffset]));
      (*workingOffset) += 4;
    }
    else if(TokenIsMathOp(tokens[i1].type)) //NEED TO CONSIDER ORDER OF OPERATIONS
    {
      //get expression following this one
      CompileExpression(bytecode, bytecodeLength, workingOffset, tokens + i1 + 1, tokenLength, consumedTokens, localSymbols, true); //stop after one
      INST mathOp;
      switch(tokens[i1].type) //RIGHT NOW THIS ONLY DOES SIGNED INTS
      {
        case TOKEN_PLUS:
          mathOp = INST_ADDS;
          break;
        case TOKEN_MINUS:
          mathOp = INST_SUBS;
          break;
        case TOKEN_PTRMULT:
          mathOp = INST_MULTS;
          break;
        case TOKEN_DIV:
          mathOp = INST_DIVS;
          break;
        default:
          SyntaxError("Unknown math op");
      }

      PrepareForWrite(bytecode, bytecodeLength, workingOffset, 2);
      (*bytecode)[(*workingOffset)++] = mathOp;
    }
    else if(tokens[i1].type == TOKEN_STRING)
    {
      char *str = new char[tokens[i1].length+1];
      strncpy(str, tokens[i1].str, tokens[i1].length);
      str[tokens[i1].length] = '\0';
      PrepareForWrite(bytecode, bytecodeLength, workingOffset, 5);
      (*bytecode)[(*workingOffset)++] = INST_PUSHC;
      stringsToFill.set(*workingOffset, str);
      (*workingOffset) += 4;
    }
    else if(tokens[i1].type == TOKEN_SYMBOL)
    {
      char temp[32];
      strncpy(temp, tokens[i1].str, tokens[i1].length);
      temp[tokens[i1].length] = '\0';
      VariableInfo *info = LookupVariable(*localSymbols, temp);
      if(info == NULL) SyntaxError("Undefined symbol in expression");
      unsigned char idx = (unsigned char)LookupVariableIndex(*localSymbols, temp);
      PrepareForWrite(bytecode, bytecodeLength, workingOffset, 5);
      (*bytecode)[(*workingOffset)++] = INST_PUSHA;
      (*bytecode)[(*workingOffset)++] = *(char*)&idx;
    }
    else
    {
      SyntaxError("Unrecognized op in expression");
    }
  }
}

void CompileCodeInternal(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength, vector<VariableInfo> localSymbols)
{
  //index of local symbol is address
  for(int i1 = 0; i1 < tokenLength; /*i1++*/)
  {
    if(tokens[i1].type == TOKEN_ENDSTATEMENT) continue;

    int consumedTokens = 0;
    bool handled = false;
    handled = HandleFunctionDeclaration(bytecode, bytecodeLength, workingOffset, tokens + i1, tokenLength - i1, &consumedTokens);
    if(!handled) handled = HandleVariableDeclaration(bytecode, bytecodeLength, workingOffset, tokens + i1, tokenLength - i1, &consumedTokens, &localSymbols);
    if(!handled) handled = HandleVariableAssignment(bytecode, bytecodeLength, workingOffset, tokens + i1, tokenLength - i1, &consumedTokens, &localSymbols);


    if(consumedTokens == 0) break;
    i1 += consumedTokens;
  }
}

void CompileCodeInternal(char **bytecode, int *bytecodeLength, int *workingOffset, Token *tokens, int tokenLength)
{
  CompileCodeInternal(bytecode, bytecodeLength, workingOffset, tokens, tokenLength, vector<VariableInfo>());
}

char *CompileToBytecode(vector<Token> &tokens, int *outputLength)
{
#define INITIALCODESIZE 128
  char *bytecode = new char[INITIALCODESIZE];
  int bytecodeLength = INITIALCODESIZE;
  int workingOffset = 0;
  symbolLocation.clear();
  jmpToFill.clear();
  symbolDefines.clear();
  stringsToFill.clear();

  bytecode[workingOffset++] = INST_JMP;
  char *mainstr = new char[5];
  strcpy(mainstr, "main");
  jmpToFill.set(workingOffset, mainstr); //set where main will need to be filled
  workingOffset += 4;

  CompileCodeInternal(&bytecode, &bytecodeLength, &workingOffset, &tokens[0], tokens.size());

  //PUT IN HALT

  //FILL IN WHERE FUNCTIONS ARE CALLED, ETC, WITH FILLED IN TABLES, ALSO DELLOCATE TABLES. ALSO THIS WILL TELL IF WE HAVE A MAIN.
  for(int i1 = 0; i1 < jmpToFill.size(); i1++)
  {
    pair<int, char*> p = jmpToFill.getAtIndex(i1);
    INT2BYTES(symbolLocation[p.second], &bytecode[p.first]);
    delete[] p.second;
  }
  for(int i1 = 0; i1 < symbolLocation.size(); i1++) delete[] symbolLocation.getAtIndex(i1).first;

  for(int i1 = 0; i1 < stringsToFill.size(); i1++)
  {
    pair<int, char*> p = stringsToFill.getAtIndex(i1);
    PrepareForWrite(&bytecode, &bytecodeLength, &workingOffset, strlen(p.second) + 2);
    strncpy(&bytecode[workingOffset], p.second, strlen(p.second) + 1);
    INT2BYTES(workingOffset, &bytecode[p.first]);
    workingOffset += strlen(p.second) + 1;
    delete[] p.second;
  }

  *outputLength = bytecodeLength;
  return bytecode;
}

int main()
{
  PopulateTokenMap();
  char code[256];
  printf("Enter text to compile: ");
  fgets(code, 256, stdin);
  if((strlen(code)>0) && (code[strlen(code) - 1] == '\n'))
    code[strlen(code) - 1] = '\0';
  vector<Token> tokens = Tokenize(code);
  for(int i1 = 0; i1 < tokens.size(); i1++)
  {
    PrintToken(tokens[i1], code);
  }
  printf("Compiling to bytecode...\n");
  int length;
  char *bytecode = CompileToBytecode(tokens, &length);
  printf("0x");
  for(int i1 = 0; i1 < length; i1++)
  {
    printf("%02x", bytecode[i1]);
  }
  return 0;
}



