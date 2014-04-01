#include "rvm_tokenmap.h"

ExactMap<const char*, TokenType> AnyWhereTokens(cmp_token_exists);
ExactMap<const char*, TokenType> KeywordTokens(cmp_token_exists_space_or_token_follows);
ExactMap<TokenType, const char*> AnyWhereTokensReverse(token_compare);
ExactMap<TokenType, const char*> KeywordTokensReverse(token_compare);

bool cmp_token_exists(const char *a, const char *b)
{
  int lenA = strlen(a);
  int lenB = strlen(b);
  int min = (lenA < lenB ? lenA : lenB);
  for(int i1 = 0; i1 < min; i1++)
  {
    if(a[i1] != b[i1]) return false;
  }
  return true;
}

bool cmp_token_exists_space_or_token_follows(const char *a, const char *b)
{
  int lenA = strlen(a);
  int lenB = strlen(b);
  if(lenA == lenB) return false; //no space could follow
  int min = (lenA < lenB ? lenA : lenB);
  for(int i1 = 0; i1 < min; i1++)
  {
    if(a[i1] != b[i1]) return false;
  }
  if(lenA == min)
  {
    if(isspace(b[min]) || AnyWhereTokens.contains(&b[min])) return true;
  }
  else if(lenB == min)
  {
    if(isspace(a[min]) || AnyWhereTokens.contains(&a[min])) return true;
  }
  return false;
}

bool token_compare(TokenType a, TokenType b)
{
  return a == b;
}

bool populated = false;

#define DEFINEANYWHERE(tok,enu) AnyWhereTokens.set(tok,enu);AnyWhereTokensReverse.set(enu,tok);
#define DEFINEKEYWORD(tok,enu) KeywordTokens.set(tok,enu);KeywordTokensReverse.set(enu,tok);

void PopulateTokenMap()
{
  if(populated) return;

  DEFINEANYWHERE(";", TOKEN_ENDSTATEMENT);
  DEFINEANYWHERE("(", TOKEN_LEFTPAREN);
  DEFINEANYWHERE(")", TOKEN_RIGHTPAREN);
  DEFINEANYWHERE("\"", TOKEN_QUOTE);
  DEFINEANYWHERE("*", TOKEN_PTRMULT);
  DEFINEANYWHERE("{", TOKEN_LEFTBRACKET);
  DEFINEANYWHERE("}", TOKEN_RIGHTBRACKET);
  DEFINEANYWHERE(",", TOKEN_COMMA);
  DEFINEANYWHERE("=", TOKEN_ASSIGNMENT);
  DEFINEANYWHERE("+", TOKEN_PLUS);
  DEFINEANYWHERE("-", TOKEN_MINUS);
  DEFINEANYWHERE("/", TOKEN_DIV);

  DEFINEKEYWORD("int", TOKEN_INT);
  DEFINEKEYWORD("float", TOKEN_FLOAT);
  DEFINEKEYWORD("string", TOKEN_STRING);
  DEFINEKEYWORD("void", TOKEN_VOID);
  DEFINEKEYWORD("#include", TOKEN_INCLUDE);
  DEFINEKEYWORD("asm", TOKEN_ASM);

  populated = true;
}


bool TokenIsMathOp(TokenType type)
{
  if(type == TOKEN_PLUS ||
     type == TOKEN_MINUS ||
     type == TOKEN_PTRMULT ||
     type == TOKEN_DIV
    )
  {
    return true;
  }
  return false;
}

bool TokenIsDataType(TokenType type)
{
  if(type == TOKEN_INT ||
     type == TOKEN_FLOAT ||
     type == TOKEN_STRING ||
     type == TOKEN_VOID
    )
  {
    return true;
  }
  return false;
}
