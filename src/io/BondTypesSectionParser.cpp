/*
 * Copyright (c) 2005 The University of Notre Dame. All Rights Reserved.
 *
 * The University of Notre Dame grants you ("Licensee") a
 * non-exclusive, royalty free, license to use, modify and
 * redistribute this software in source and binary code form, provided
 * that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
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
 *
 * SUPPORT OPEN SCIENCE!  If you use OpenMD or its source code in your
 * research, please cite the appropriate papers when you publish your
 * work.  Good starting points are:
 *                                                                      
 * [1]  Meineke, et al., J. Comp. Chem. 26, 252-271 (2005).             
 * [2]  Fennell & Gezelter, J. Chem. Phys. 124, 234104 (2006).          
 * [3]  Sun, Lin & Gezelter, J. Chem. Phys. 128, 234107 (2008).          
 * [4]  Kuang & Gezelter,  J. Chem. Phys. 133, 164101 (2010).
 * [5]  Vardeman, Stocker & Gezelter, J. Chem. Theory Comput. 7, 834 (2011).
 */
 
#include "io/BondTypesSectionParser.hpp"
#include "types/BondTypeParser.hpp"
#include "brains/ForceField.hpp"
#include "utils/simError.h"

namespace OpenMD {

  BondTypesSectionParser::BondTypesSectionParser(ForceFieldOptions& options) : options_(options){
    setSectionName("BondTypes");
  }

  void BondTypesSectionParser::parseLine(ForceField& ff,
                                         const std::string& line,
                                         int lineNo) {
    StringTokenizer tokenizer(line);
    BondTypeParser btParser;        
    BondType* bondType = NULL;
    int nTokens = tokenizer.countTokens();
    
    if (nTokens < 4) {
      sprintf(painCave.errMsg,
              "BondTypesSectionParser Error: Not enough tokens at line %d\n",
	      lineNo);
      painCave.isFatal = 1;
      simError();
    }
    
    std::string at1 = tokenizer.nextToken();
    std::string at2 = tokenizer.nextToken(); 
    std::string remainder = tokenizer.getRemainingString();
   
    try {
      bondType = btParser.parseLine(remainder);
    }
    catch( OpenMDException e ) {
      
      sprintf(painCave.errMsg, "BondTypesSectionParser Error: %s "
              "at line %d\n",
              e.what(), lineNo);
      painCave.isFatal = 1;
      simError();
    }

    if (bondType != NULL) {
      ff.addBondType(at1, at2, bondType);
    }

  }
}

