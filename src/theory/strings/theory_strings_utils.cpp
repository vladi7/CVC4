/*********************                                                        */
/*! \file theory_strings_utils.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Util functions for theory strings.
 **
 ** Util functions for theory strings.
 **/

#include "theory/strings/theory_strings_utils.h"

#include "options/strings_options.h"
#include "theory/rewriter.h"
#include "theory/strings/word.h"

using namespace CVC4::kind;

namespace CVC4 {
namespace theory {
namespace strings {
namespace utils {

uint32_t getAlphabetCardinality()
{
  if (options::stdPrintASCII())
  {
    Assert(128 <= String::num_codes());
    return 128;
  }
  Assert(256 <= String::num_codes());
  return 256;
}

Node mkAnd(const std::vector<Node>& a)
{
  std::vector<Node> au;
  for (const Node& ai : a)
  {
    if (std::find(au.begin(), au.end(), ai) == au.end())
    {
      au.push_back(ai);
    }
  }
  if (au.empty())
  {
    return NodeManager::currentNM()->mkConst(true);
  }
  else if (au.size() == 1)
  {
    return au[0];
  }
  return NodeManager::currentNM()->mkNode(AND, au);
}

void flattenOp(Kind k, Node n, std::vector<Node>& conj)
{
  if (n.getKind() != k)
  {
    // easy case, just add to conj if non-duplicate
    if (std::find(conj.begin(), conj.end(), n) == conj.end())
    {
      conj.push_back(n);
    }
    return;
  }
  // otherwise, traverse
  std::unordered_set<TNode, TNodeHashFunction> visited;
  std::unordered_set<TNode, TNodeHashFunction>::iterator it;
  std::vector<TNode> visit;
  TNode cur;
  visit.push_back(n);
  do
  {
    cur = visit.back();
    visit.pop_back();
    it = visited.find(cur);

    if (it == visited.end())
    {
      visited.insert(cur);
      if (cur.getKind() == k)
      {
        for (const Node& cn : cur)
        {
          visit.push_back(cn);
        }
      }
      else if (std::find(conj.begin(), conj.end(), cur) == conj.end())
      {
        conj.push_back(cur);
      }
    }
  } while (!visit.empty());
}

void getConcat(Node n, std::vector<Node>& c)
{
  Kind k = n.getKind();
  if (k == STRING_CONCAT || k == REGEXP_CONCAT)
  {
    for (const Node& nc : n)
    {
      c.push_back(nc);
    }
  }
  else
  {
    c.push_back(n);
  }
}

Node mkConcat(const std::vector<Node>& c, TypeNode tn)
{
  Assert(tn.isStringLike() || tn.isRegExp());
  if (c.empty())
  {
    Assert(tn.isStringLike());
    return Word::mkEmptyWord(tn);
  }
  else if (c.size() == 1)
  {
    return c[0];
  }
  Kind k = tn.isStringLike() ? STRING_CONCAT : REGEXP_CONCAT;
  return NodeManager::currentNM()->mkNode(k, c);
}

Node mkNConcat(Node n1, Node n2)
{
  return Rewriter::rewrite(
      NodeManager::currentNM()->mkNode(STRING_CONCAT, n1, n2));
}

Node mkNConcat(Node n1, Node n2, Node n3)
{
  return Rewriter::rewrite(
      NodeManager::currentNM()->mkNode(STRING_CONCAT, n1, n2, n3));
}

Node mkNConcat(const std::vector<Node>& c, TypeNode tn)
{
  return Rewriter::rewrite(mkConcat(c, tn));
}

Node mkNLength(Node t)
{
  return Rewriter::rewrite(NodeManager::currentNM()->mkNode(STRING_LENGTH, t));
}

Node getConstantComponent(Node t)
{
  if (t.getKind() == STRING_TO_REGEXP)
  {
    return t[0].isConst() ? t[0] : Node::null();
  }
  return t.isConst() ? t : Node::null();
}

Node getConstantEndpoint(Node e, bool isSuf)
{
  Kind ek = e.getKind();
  if (ek == STRING_IN_REGEXP)
  {
    e = e[1];
    ek = e.getKind();
  }
  if (ek == STRING_CONCAT || ek == REGEXP_CONCAT)
  {
    return getConstantComponent(e[isSuf ? e.getNumChildren() - 1 : 0]);
  }
  return getConstantComponent(e);
}

bool isUnboundedWildcard(const std::vector<Node>& rs, size_t start)
{
  size_t i = start;
  while (i < rs.size() && rs[i].getKind() == REGEXP_SIGMA)
  {
    i++;
  }

  if (i >= rs.size())
  {
    return false;
  }

  return rs[i].getKind() == REGEXP_STAR && rs[i][0].getKind() == REGEXP_SIGMA;
}

bool isSimpleRegExp(Node r)
{
  Assert(r.getType().isRegExp());

  std::vector<Node> v;
  utils::getConcat(r, v);
  for (const Node& n : v)
  {
    if (n.getKind() == STRING_TO_REGEXP)
    {
      if (!n[0].isConst())
      {
        return false;
      }
    }
    else if (n.getKind() != REGEXP_SIGMA
             && (n.getKind() != REGEXP_STAR || n[0].getKind() != REGEXP_SIGMA))
    {
      return false;
    }
  }
  return true;
}

void getRegexpComponents(Node r, std::vector<Node>& result)
{
  Assert(r.getType().isRegExp());

  NodeManager* nm = NodeManager::currentNM();
  if (r.getKind() == REGEXP_CONCAT)
  {
    for (const Node& n : r)
    {
      getRegexpComponents(n, result);
    }
  }
  else if (r.getKind() == STRING_TO_REGEXP && r[0].isConst())
  {
    size_t rlen = Word::getLength(r[0]);
    for (size_t i = 0; i < rlen; i++)
    {
      result.push_back(nm->mkNode(STRING_TO_REGEXP, Word::substr(r[0], i, 1)));
    }
  }
  else
  {
    result.push_back(r);
  }
}

void printConcat(std::ostream& out, std::vector<Node>& n)
{
  for (unsigned i = 0, nsize = n.size(); i < nsize; i++)
  {
    if (i > 0)
    {
      out << " ++ ";
    }
    out << n[i];
  }
}

void printConcatTrace(std::vector<Node>& n, const char* c)
{
  std::stringstream ss;
  printConcat(ss, n);
  Trace(c) << ss.str();
}

bool isStringKind(Kind k)
{
  return k == STRING_STOI || k == STRING_ITOS || k == STRING_TOLOWER
         || k == STRING_TOUPPER || k == STRING_LEQ || k == STRING_LT
         || k == STRING_FROM_CODE || k == STRING_TO_CODE;
}

TypeNode getOwnerStringType(Node n)
{
  TypeNode tn;
  Kind k = n.getKind();
  if (k == STRING_STRIDOF || k == STRING_LENGTH || k == STRING_STRCTN
      || k == STRING_PREFIX || k == STRING_SUFFIX)
  {
    // owning string type is the type of first argument
    tn = n[0].getType();
  }
  else if (isStringKind(k))
  {
    tn = NodeManager::currentNM()->stringType();
  }
  else
  {
    tn = n.getType();
  }
  AlwaysAssert(tn.isStringLike())
      << "Unexpected term in getOwnerStringType : " << n << ", type " << tn;
  return tn;
}

}  // namespace utils
}  // namespace strings
}  // namespace theory
}  // namespace CVC4
