#ifndef _RVM_TOKENMAP
#define _RVM_TOKENMAP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <ctype.h>
#include <vector>
#include <utility>

using namespace std;

enum TokenType
{
  TOKEN_INT = 1,
  TOKEN_FLOAT,
  TOKEN_STRING,
  TOKEN_VOID,
  TOKEN_INCLUDE,
  TOKEN_ASM,

  TOKEN_SYMBOL,

  TOKEN_LEFTPAREN,
  TOKEN_RIGHTPAREN,
  TOKEN_LEFTBRACKET,
  TOKEN_RIGHTBRACKET,
  TOKEN_ENDSTATEMENT,
  TOKEN_QUOTE,
  TOKEN_PTRMULT,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_DIV,
  TOKEN_COMMA,
  TOKEN_ASSIGNMENT,

  TOKEN_CONSTSTRING,
  TOKEN_NUMBER,

  TOKEN_INVALID
};

template <class K, class V>
class ExactMap
{
public:
  ExactMap(bool (*cmp)(K, K)) : comparer(cmp)
  {
  }

  V& operator [](const K key)
  {
    for(int i1 = 0; i1 < values.size(); i1++)
    {
      if(comparer(values[i1].first, key)) return values[i1].second;
    }
    throw new runtime_error("Key not found");
  }

  void set(const K key, const V value)
  {
    for(int i1 = 0; i1 < values.size(); i1++)
    {
      if(comparer(values[i1].first, key))
      {
        values[i1].second = value;
        return;
      }
    }
    values.push_back(make_pair(key, value));
  }

  pair<K, V>& getAtIndex(int idx)
  {
    if(idx >= values.size() || idx < 0) throw runtime_error("Out of bounds");
    return values[idx];
  }

  size_t size() { return values.size(); }

  bool contains(const K key)
  {
    for(int i1 = 0; i1 < values.size(); i1++)
    {
      if(comparer(values[i1].first, key)) return true;
    }
    return false;
  }

  void clear()
  {
    values.clear();
  }

private:
  bool (*comparer)(K, K);
  vector< pair <K, V> > values;
};

extern bool cmp_token_exists(const char *a, const char *b);
extern bool cmp_token_exists_space_or_token_follows(const char *a, const char *b);
extern bool token_compare(TokenType a, TokenType b);

extern ExactMap<const char*, TokenType> AnyWhereTokens;
extern ExactMap<const char*, TokenType> KeywordTokens;
extern ExactMap<TokenType, const char*> AnyWhereTokensReverse;
extern ExactMap<TokenType, const char*> KeywordTokensReverse;

extern void PopulateTokenMap();

extern bool TokenIsDataType(TokenType type);
extern bool TokenIsMathOp(TokenType type);

#endif
