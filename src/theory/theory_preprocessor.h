/*********************                                                        */
/*! \file theory_preprocessor.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Dejan Jovanovic, Morgan Deters, Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The theory preprocessor.
 **/

#include "cvc4_private.h"

#ifndef CVC4__THEORY__THEORY_PREPROCESSOR_H
#define CVC4__THEORY__THEORY_PREPROCESSOR_H

#include <unordered_map>

#include "expr/lazy_proof.h"
#include "expr/node.h"
#include "expr/term_conversion_proof_generator.h"
#include "theory/trust_node.h"

namespace CVC4 {

class LogicInfo;
class TheoryEngine;
class RemoveTermFormulas;

namespace theory {

/**
 * The preprocessor used in TheoryEngine.
 */
class TheoryPreprocessor
{
  typedef std::unordered_map<Node, Node, NodeHashFunction> NodeMap;

 public:
  /** Constructs a theory preprocessor */
  TheoryPreprocessor(TheoryEngine& engine,
                     RemoveTermFormulas& tfr,
                     ProofNodeManager* pnm = nullptr);
  /** Destroys a theory preprocessor */
  ~TheoryPreprocessor();
  /** Clear the cache of this class */
  void clearCache();
  /**
   * Preprocesses node and stores it along with lemmas generated by
   * preprocessing into the assertion pipeline lemmas. The (optional) argument
   * lcp is the proof that stores a proof of all top-level formulas in lemmas,
   * assuming that lcp initially contains a proof of node. The flag
   * doTheoryPreprocess is whether we should run theory-specific preprocessing.
   */
  TrustNode preprocess(TNode node,
                       std::vector<TrustNode>& newLemmas,
                       std::vector<Node>& newSkolems,
                       bool doTheoryPreprocess);
  /**
   * Runs theory specific preprocessing on the non-Boolean parts of
   * the formula.  This is only called on input assertions, after ITEs
   * have been removed.
   */
  TrustNode theoryPreprocess(TNode node);

 private:
  /** Reference to owning theory engine */
  TheoryEngine& d_engine;
  /** Logic info of theory engine */
  const LogicInfo& d_logicInfo;
  /** Cache for theory-preprocessing of assertions */
  NodeMap d_ppCache;
  /** The term formula remover */
  RemoveTermFormulas& d_tfr;
  /** The context for the proof generator below */
  context::Context d_pfContext;
  /** A term conversion proof generator */
  std::unique_ptr<TConvProofGenerator> d_tpg;
  /** A lazy proof, for additional lemmas. */
  std::unique_ptr<LazyCDProof> d_lp;
  /** Helper for theoryPreprocess */
  Node ppTheoryRewrite(TNode term);
  /** rewrite with proof, store REWRITE step in d_tpg. */
  Node rewriteWithProof(Node term);
  /** preprocess with proof */
  Node preprocessWithProof(Node term);
  /** Proofs enabled */
  bool isProofEnabled() const;
};

}  // namespace theory
}  // namespace CVC4

#endif /* CVC4__THEORY__THEORY_PREPROCESSOR_H */
