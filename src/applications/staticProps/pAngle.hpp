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
 * [4]  Vardeman, Stocker & Gezelter, J. Chem. Theory Comput. 7, 834 (2011).                     
 *
 */
#ifndef APPLICATIONS_STATICPROPS_PANGLE_HPP
#define APPLICATIONS_STATICPROPS_PANGLE_HPP

#include <string>
#include <vector>
#include "selection/SelectionEvaluator.hpp"
#include "selection/SelectionManager.hpp"
#include "utils/NumericConstant.hpp"
#include "applications/staticProps/StaticAnalyser.hpp"

using namespace std;
namespace OpenMD {
  
  class pAngle : public StaticAnalyser {
    
  public:
    pAngle(SimInfo* info, const string& filename, 
           const string& sele1, int nzbins);
    pAngle(SimInfo* info, const string& filename, 
           const string& sele1, const string& sele2, int nzbins);
    pAngle(SimInfo* info, const string& filename, 
           const string& sele, const int seleOffset, int nzbins);
    pAngle(SimInfo* info, const string& filename, 
           const string& sele, const int seleOffset, const int seleOffset2, 
           int nzbins);
    
    int getNThetaBins() {
      return nThetaBins_; 
    }
    
    virtual void process();
    
  private:
    
    void processHistogram();
    void writeProbs();
        
    Snapshot* currentSnapshot_;

    bool doVect_;
    bool doOffset_;
    bool doOffset2_;
    string selectionScript1_;
    string selectionScript2_;
    SelectionManager seleMan1_;
    SelectionManager seleMan2_;       
    SelectionEvaluator evaluator1_;
    SelectionEvaluator evaluator2_;
    int seleOffset_;
    int seleOffset2_;

    int nProcessed_;    
    int nThetaBins_;     
    vector<int> count_;
    vector<RealType> histogram_;
  };
  
}
#endif



