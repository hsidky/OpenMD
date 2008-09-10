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
 
#include <functional>
#include <iterator>
#include <utility>

#include "brains/PairList.hpp"

namespace oopse {
  
  int *PairList::getPairList() {

    if (modified_) {
      pairList_.clear();
      
      for (std::set<std::pair<int,int> >::iterator i = pairSet_.begin();
           i != pairSet_.end(); ++i) {
	pairList_.push_back(i->first + 1);
	pairList_.push_back(i->second + 1);            
      }
      modified_ = false;
    } 
    
    return pairList_.size() > 0 ? &(pairList_[0]) : NULL;    
  }
  
  void PairList::addPair(int i, int j) {
    
    if (i == j) {
      return;
    } else if (i > j) {
      std::swap(i, j);
    }
    
    std::set<std::pair<int, int> >::iterator iter = pairSet_.find(std::make_pair(i, j));
    
    if (iter == pairSet_.end()) {
      pairSet_.insert(std::make_pair(i, j));
      modified_ = true;
    }
  }
  
  void PairList::addPairs(std::set<int>& set1, std::set<int>& set2) {
    for (std::set<int>::iterator iter1 = set1.begin(); 
         iter1 !=  set1.end(); ++ iter1) {
      for(std::set<int>::iterator iter2 = set2.begin(); 
          iter2 != set2.end(); ++ iter2) {
        this->addPair(*iter1, *iter2);
      }
    }    
  }

  template<typename IterType1, typename IterType2>
  void PairList::addPairs(IterType1 iter1_first, IterType1 iter1_last, IterType2 iter2_first, IterType2 iter2_last) {
    for (IterType1 iter1 = iter1_first; iter1 != iter1_last; ++ iter1) {
      for(IterType2 iter2 = iter2_first; iter2 != iter2_last; ++ iter2) {
        this->addPair(*iter1, * iter2);
      }
    }
  }

  void PairList::removePair(int i, int j) {    
    if (i == j) {
      return;
    } else if (i > j) {
      std::swap(i, j);
    }


    std::set<std::pair<int, int> >::iterator iter = pairSet_.find(std::make_pair(i, j));
    
    if (iter != pairSet_.end()) {
      pairSet_.erase(iter);
      modified_ = true;
    }
  }

  void PairList::removePairs(std::set<int>& set1, std::set<int>& set2) {
    for (std::set<int>::iterator iter1 = set1.begin(); 
         iter1 !=  set1.end(); ++ iter1) {
      for(std::set<int>::iterator iter2 = set2.begin(); 
          iter2 != set2.end(); ++ iter2) {
        this->removePair(*iter1, * iter2);
      }
    }    
  }

  template<typename IterType1, typename IterType2>
  void PairList::removePairs(IterType1 iter1_first, IterType1 iter1_last, IterType2 iter2_first, IterType2 iter2_last) {
    for (IterType1 iter1 = iter1_first; iter1 != iter1_last; ++ iter1) {
      for(IterType2 iter2 = iter2_first; iter2 != iter2_last; ++ iter2) {
        this->removePair(*iter1, * iter2);
      }
    }
  }

  bool PairList::hasPair(int i, int j) {
    
    if (i == j) {
      return false;
    } else if (i > j) {
      std::swap(i, j);
    }
    
    std::set<std::pair<int, int> >::iterator  iter = pairSet_.find(std::make_pair(i, j));
    return iter == pairSet_.end() ? false : true; 
  }
  
  int PairList::getSize() {
    return pairSet_.size();
  }

  std::ostream& operator <<(std::ostream& o, PairList& e) {
    std::set<std::pair<int, int> >::iterator i;
    
    int index;
    
    index = 0;
    
    for(i = e.pairSet_.begin(); i != e.pairSet_.end(); ++i) {
      o << "pairList[" << index << "] i, j: " << (*i).first << " - "
	<< (*i).second << "\n";
      index++;
    }
    
    return o;
  }
  
}

  
