/*
 * Copyright (c) 2005 The University of Notre Dame. All Rights Reserved.
 *
 * The University of Notre Dame grants you ("Licensee") a
 * non-exclusive, royalty free, license to use, modify and
 * redistribute this software in source and binary code form, provided
 * that the following conditions are met:
 *
 * 1. Acknowledgement of the program authors must be made in any
 *    publication of scientific results based in part on use of the
 *    program.  An acceptable form of acknowledgement is citation of
 *    the article in which the program was described (Matthew
 *    A. Meineke, Charles F. Vardeman II, Teng Lin, Christopher
 *    J. Fennell and J. Daniel Gezelter, "OOPSE: An Object-Oriented
 *    Parallel Simulation Engine for Molecular Dynamics,"
 *    J. Comput. Chem. 26, pp. 252-271 (2005))
 *
 * 2. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 3. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 * This software is provided "AS IS," without a warranty of any
 * kind. All express or implied conditions, representations and
 * warranties, including any implied warranty of merchantability,
 * fitness for a particular purpose or non-infringement, are hereby
 * excluded.  The University of Notre Dame and its licensors shall not
 * be liable for any damages suffered by licensee as a result of
 * using, modifying or distributing the software or its
 * derivatives. In no event will the University of Notre Dame or its
 * licensors be liable for any lost revenue, profit or data, or for
 * direct, indirect, special, consequential, incidental or punitive
 * damages, however caused and regardless of the theory of liability,
 * arising out of the use of or inability to use software, even if the
 * University of Notre Dame has been advised of the possibility of
 * such damages.
 */

#ifndef MATH_RANDNUMGEN_HPP
#define MATH_RANDNUMGEN_HPP

#include <vector>
#include "MersenneTwister.hpp"
#include "utils/simError.h"


namespace oopse {



  /**
   * @class RandNumGen a random number generator class
   */
  class RandNumGen{
  public:
    typedef unsigned long uint32; 
        
    virtual ~RandNumGen() {
      delete mtRand_;
    }
        
    /** Returns a real number in [0,1] */
    double rand() {
      return mtRand_->rand();
    }

    /** Returns a real number in [0, n] */
    double rand( const double& n ) {
      return mtRand_->rand(n);
    }

    /** Returns a real number in [0, 1) */
    double randExc() {
      return mtRand_->randExc();
    }

    /** Returns a real number in [0, n) */        
    double randExc( const double& n ) {
      return mtRand_->randExc(n);
    }

    /** Returns a real number in (0, 1) */                
    double randDblExc() {
      return mtRand_->randDblExc();
    }

    /** Returns a real number in (0, n) */                        
    double randDblExc( const double& n ) {
      return mtRand_->randDblExc(n);
    }

    /** Returns aninteger in [0,2^32-1]  */            
    uint32 randInt() {
      return mtRand_->randInt();
    }

    /** Returns aninteger in [0, n]  for n < 2^32 */     
    uint32 randInt( const uint32& n ) {
      return mtRand_->randInt(n);
    }
	
    /** Returns a 53-bitreal number in [0,1) (capacity of IEEE double precision) */
    double rand53() {
      return mtRand_->rand53();
    }
	
    /** Access to nonuniform random number distributions */
    double randNorm( const double& mean, const double& variance) {
      return mtRand_->randNorm(mean, variance);
    }
	
    // Re-seeding functions with same behavior as initializers
    virtual void seed( const uint32 oneSeed ) = 0;
	
    virtual void seed()= 0;

  protected:
        
    MTRand* mtRand_;


  };

}

#endif 

