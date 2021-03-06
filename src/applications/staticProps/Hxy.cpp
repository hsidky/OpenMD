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
 * [4] Kuang & Gezelter,  J. Chem. Phys. 133, 164101 (2010).
 * [4] , Stocker & Gezelter, J. Chem. Theory Comput. 7, 834 (2011). *
 *
 *  Created by Xiuquan Sun on 05/09/06.
 *  @author  Xiuquan Sun 
 *  @version $Id$
 *
 */

/* Calculates the undulation spectrum of the lipid membrance. */

#include <algorithm>
#include <fstream>
#include "applications/staticProps/Hxy.hpp"
#include "utils/simError.h"
#include "utils/PhysicalConstants.hpp"
#include "utils/Utility.hpp"
#include "io/DumpReader.hpp"
#include "primitives/Molecule.hpp"
#include "types/LennardJonesAdapter.hpp"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>

namespace OpenMD {
  
  Hxy::Hxy(SimInfo* info, const std::string& filename,
           const std::string& sele, int nbins_x, int nbins_y, int nbins_z,
           int nrbins)
    : StaticAnalyser(info, filename, nrbins), selectionScript_(sele),
      evaluator_(info), seleMan_(info), nBinsX_(nbins_x), nBinsY_(nbins_y),
      nBinsZ_(nbins_z) {

    evaluator_.loadScriptString(sele);
    if (!evaluator_.isDynamic()) {
      seleMan_.setSelectionSet(evaluator_.evaluate());
    }

    // dens_ stores the local density, rho(x,y,z) on a 3-D grid
    dens_.resize(nBinsX_);
    // bin stores the upper and lower surface cutoff locations (z) for
    // a column through grid location x,y
    minHeight_.resize(nBinsX_);
    maxHeight_.resize(nBinsX_);

    for (unsigned int i = 0; i < nBinsX_; i++) {
      dens_[i].resize(nBinsY_);
      minHeight_[i].resize(nBinsY_);
      maxHeight_[i].resize(nBinsY_);            
      for (unsigned int j = 0; j < nBinsY_; j++) {
        dens_[i][j].resize(nBinsZ_);
      }
    }
    
    mag1.resize(nBinsX_*nBinsY_);
    newmag1.resize(nBinsX_*nBinsY_);
    mag2.resize(nBinsX_*nBinsY_);
    newmag2.resize(nBinsX_*nBinsY_);
    
    // Pre-load an OutputData     
    freq_= new OutputData;
    freq_->units =  "Angstroms^-1";
    freq_->title =  "Spatial Frequency";
    freq_->dataType = odtReal;
    freq_->dataHandling = odhAverage;
    freq_->accumulator.reserve(nBins_);
    for (int i = 0; i < nBins_; i++) 
      freq_->accumulator.push_back( new Accumulator() );
    data_.push_back(freq_);

    top_= new OutputData;
    top_->units =  "Angstroms";
    top_->title =  "Hxy (Upper surface)";
    top_->dataType = odtReal;
    top_->dataHandling = odhAverage;
    top_->accumulator.reserve(nBins_);
    for (int i = 0; i < nBins_; i++) 
      top_->accumulator.push_back( new Accumulator() );
    data_.push_back(top_);
    
    bottom_= new OutputData;
    bottom_->units =  "Angstroms";
    bottom_->title =  "Hxy (Lower surface)";
    bottom_->dataType = odtReal;
    bottom_->dataHandling = odhAverage;
    bottom_->accumulator.reserve(nBins_);
    for (int i = 0; i < nBins_; i++) 
      bottom_->accumulator.push_back( new Accumulator() );
    data_.push_back(bottom_);

    setOutputName(getPrefix(filename) + ".Hxy");
  }

  Hxy::~Hxy(){
  }

  void Hxy::process() {
#if defined(HAVE_FFTW_H) || defined(HAVE_DFFTW_H) || defined(HAVE_FFTW3_H)
    Molecule* mol;
    RigidBody* rb;
    StuntDouble* sd;
    SimInfo::MoleculeIterator mi;
    Molecule::RigidBodyIterator rbIter;
    int ii;
    bool usePeriodicBoundaryConditions_ = info_->getSimParams()->getUsePeriodicBoundaryConditions();

    DumpReader reader(info_, dumpFilename_);    
    int nFrames = reader.getNFrames();
    nProcessed_ = nFrames/step_;

    for (int istep = 0; istep < nFrames; istep += step_) {

      for (unsigned int i = 0; i < nBinsX_; i++) {
        std::fill(minHeight_[i].begin(), minHeight_[i].end(), 0.0);
        std::fill(maxHeight_[i].begin(), maxHeight_[i].end(), 0.0);
        for (unsigned int j = 0; j < nBinsY_; j++) {
          std::fill(dens_[i][j].begin(), dens_[i][j].end(), 0.0);
        }
      }
                 
      reader.readFrame(istep);
      currentSnapshot_ = info_->getSnapshotManager()->getCurrentSnapshot();
      if (evaluator_.isDynamic()) {
        seleMan_.setSelectionSet(evaluator_.evaluate());
      }
      
      // update the positions of atoms which belong to the rigidbodies
      
      for (mol = info_->beginMolecule(mi); mol != NULL; 
           mol = info_->nextMolecule(mi)) {
        for (rb = mol->beginRigidBody(rbIter); rb != NULL; 
             rb = mol->nextRigidBody(rbIter)) {
          rb->updateAtoms();
        }        
      }                       
      
#ifdef HAVE_FFTW3_H
      fftw_plan p1, p2;
#else
      fftwnd_plan p1, p2;
#endif
      fftw_complex *in1, *in2, *out1, *out2;
      
      in1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (nBinsX_*nBinsY_));
      out1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) *(nBinsX_*nBinsY_));
      in2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (nBinsX_*nBinsY_));
      out2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) *(nBinsX_*nBinsY_));

#ifdef HAVE_FFTW3_H
      p1 = fftw_plan_dft_2d(nBinsX_, nBinsY_, in1, out1, 
                           FFTW_FORWARD, FFTW_ESTIMATE); 
      p2 = fftw_plan_dft_2d(nBinsX_, nBinsY_, in2, out2, 
                           FFTW_FORWARD, FFTW_ESTIMATE); 
#else
      p1 = fftw2d_create_plan(nBinsX_, nBinsY_, FFTW_FORWARD, FFTW_ESTIMATE);
      p2 = fftw2d_create_plan(nBinsX_, nBinsY_, FFTW_FORWARD, FFTW_ESTIMATE);
#endif

      Mat3x3d hmat = currentSnapshot_->getHmat();
      Mat3x3d invBox = currentSnapshot_->getInvHmat();
      RealType lenX_ = hmat(0,0);
      RealType lenY_ = hmat(1,1);
      RealType lenZ_ = hmat(2,2);

      Vector3d hbox = Vector3d(lenX_/2.0, lenY_/2.0, lenZ_/2.0);
     
      RealType x, y, z, dx, dy, dz;
      RealType sigma, rcut;
      int di, dj, dk, ibin, jbin, kbin;
      int igrid, jgrid, kgrid;
      Vector3d scaled;
      
      dx = lenX_ / nBinsX_;
       dy = lenY_ / nBinsY_;
      dz = lenZ_ / nBinsZ_;
            
      for (sd = seleMan_.beginSelected(ii); sd != NULL;
           sd = seleMan_.nextSelected(ii)) {

        if (sd->isAtom()) {
          Atom* atom = static_cast<Atom*>(sd);
          Vector3d pos = sd->getPos();
          LennardJonesAdapter lja = LennardJonesAdapter(atom->getAtomType());
          // For SPC/E water, this yields the Willard-Chandler
          // distance of 2.4 Angstroms:
          //sigma = lja.getSigma() * 0.758176459;
          sigma = lja.getSigma() * 0.5;
          rcut = 3.0 * sigma;

          // scaled positions relative to the box vectors
          scaled = invBox * pos;
          // wrap the vector back into the unit box by subtracting
          // integer box numbers
          for (int j = 0; j < 3; j++) {
            scaled[j] -= roundMe(scaled[j]);
            scaled[j] += 0.5;
            // Handle the special case when an object is exactly on
            // the boundary (a scaled coordinate of 1.0 is the same as
            // scaled coordinate of 0.0)
            if (scaled[j] >= 1.0) scaled[j] -= 1.0;
          }
          // find ijk-indices of voxel that atom is in.
          ibin = nBinsX_ * scaled.x();
          jbin = nBinsY_ * scaled.y();
          kbin = nBinsZ_ * scaled.z();
                   
          di = (int) (rcut / dx);
          dj = (int) (rcut / dy);
          dk = (int) (rcut / dz);
                    
          for (int i = -di; i <= di; i++) {

            igrid = ibin + i;
            while (igrid >= nBinsX_) {igrid -= nBinsX_;}
            while (igrid < 0) {igrid += nBinsX_;}
                        
            x = lenX_ * (RealType(igrid) / RealType(nBinsX_) );
            
            for (int j = -dj; j <= dj; j++) {
              jgrid = jbin + j;
              while (jgrid >= nBinsY_) {jgrid -= nBinsY_;}
              while (jgrid < 0) {jgrid += nBinsY_;}
              
              y = lenY_ * (RealType(jgrid) / RealType(nBinsY_));
              
              for (int k = -dk; k <= dk; k++) {
                kgrid = kbin + k;
                while (kgrid >= nBinsZ_) {kgrid -= nBinsZ_;}
                while (kgrid < 0) {kgrid += nBinsZ_;}

                z = lenZ_ * (RealType(kgrid) / RealType(nBinsZ_));

                
                Vector3d r = Vector3d(x, y, z) - (pos + hbox);
                
                if (usePeriodicBoundaryConditions_)
                  currentSnapshot_->wrapVector(r);
                
                RealType dist = r.length();
                dens_[igrid][jgrid][kgrid] += getDensity(dist, sigma, rcut);
              }
            }
          }
        }
      }

      RealType maxDens(0.0);
      for (unsigned int i = 0; i < nBinsX_; i++) {
        for (unsigned int j = 0; j < nBinsY_; j++) {
          for (unsigned int k = 0; k < nBinsZ_; k++) {
            //std::cout << dens_[i][j][k] << "\t";
            if (dens_[i][j][k] > maxDens) maxDens = dens_[i][j][k];
          }
          //std::cout << "\n";
        }
        //std::cout << "\n";
      }
      
      RealType threshold = maxDens / 2.0;
      RealType z0, z1, h0, h1;

      for (unsigned int i = 0; i < nBinsX_; i++) {        
        for (unsigned int j = 0; j < nBinsY_; j++) {

          // There are two cases if we are periodic in z and the
          // density is localized in z.  Either we're starting below
          // the isodensity, or above it:
          //      ______             _______        ______
          // ____/      \_____ or:          \______/
          //          
          // In either case, there are two crossings of the
          // isodensity.
          
          if (dens_[i][j][0] < threshold) {
            bool minFound = false;
            for (unsigned int k = 0; k < nBinsZ_-1; k++) {

              z0 = lenZ_ * (RealType(k) / RealType(nBinsZ_));
              z1 = lenZ_ * (RealType(k+1) / RealType(nBinsZ_));
              h0 = dens_[i][j][k];
              h1 = dens_[i][j][k+1];
              
              if (h0 < threshold && h1 > threshold && !minFound) {
                // simple linear interpolation to find the height:
                minHeight_[i][j] = z0 + (z1-z0)*(threshold-h0)/(h1-h0);
                minFound = true;
              }
              if (h0 > threshold && h1 < threshold && minFound) {
                // simple linear interpolation to find the height:
                maxHeight_[i][j] = z0 + (z1-z0)*(threshold-h0)/(h1-h0);
              }
            }
            
          } else {
            bool maxFound = false;
            for (unsigned int k = 0; k < nBinsZ_-1; k++) {

              z0 = lenZ_ * (RealType(k) / RealType(nBinsZ_));
              z1 = lenZ_ * (RealType(k+1) / RealType(nBinsZ_));
              h0 = dens_[i][j][k];
              h1 = dens_[i][j][k+1];
              
              if (h0 > threshold && h1 < threshold && !maxFound) {
                // simple linear interpolation to find the height:
                maxHeight_[i][j] = z0 + (z1-z0)*(threshold-h0)/(h1-h0);
                maxFound = true;
              }
              if (h0 < threshold && h1 > threshold && maxFound) {
                // simple linear interpolation to find the height:
                minHeight_[i][j] = z0 + (z1-z0)*(threshold-h0)/(h1-h0);
              }
            }
          }
        }
      }

      RealType minBar = 0.0;
      RealType maxBar = 0.0;
      int count = 0;
      for (unsigned int i = 0; i < nBinsX_; i++) {        
        for (unsigned int j = 0; j < nBinsY_; j++) {
          minBar += minHeight_[i][j];
          maxBar += maxHeight_[i][j];
          count++;
        }
      }           
      minBar /= count;
      maxBar /= count;

      std::cerr << "bottomSurf = " << minBar << "\ttopSurf = " << maxBar << "\n";
      int newindex;
      for (unsigned int i=0; i < nBinsX_; i++) {
	for (unsigned int j=0; j < nBinsY_; j++) {
	  newindex = i*nBinsY_ + j;
          //std::cout << minHeight_[i][j] << "\t";
	  c_re(in1[newindex]) = maxHeight_[i][j] - maxBar;
	  c_im(in1[newindex]) = 0.0;
	  c_re(in2[newindex]) = minHeight_[i][j] - minBar;
	  c_im(in2[newindex]) = 0.0;
	}
        //std::cout << "\n";
      }

#ifdef HAVE_FFTW3_H
      fftw_execute(p1);
      fftw_execute(p2);
#else
      fftwnd_one(p1, in1, out1);
      fftwnd_one(p2, in2, out2);
#endif
      
      for (unsigned int i=0; i< nBinsX_; i++) {
	for(unsigned int j=0; j< nBinsY_; j++) {
	  newindex = i*nBinsY_ + j;
	  mag1[newindex] = pow(c_re(out1[newindex]),2) +
            pow(c_im(out1[newindex]),2);
	  mag2[newindex] = pow(c_re(out2[newindex]),2) +
            pow(c_im(out2[newindex]),2);
	}
      }

#ifdef HAVE_FFTW3_H
      fftw_destroy_plan(p1);
      fftw_destroy_plan(p2);
#else
      fftwnd_destroy_plan(p1);
      fftwnd_destroy_plan(p2);
#endif      
      fftw_free(out1);
      fftw_free(in1);
      fftw_free(out2);
      fftw_free(in2);

      int index, new_i, new_j, new_index;
      for (unsigned int i=0; i< (nBinsX_/2); i++) {
	for(unsigned int j=0; j< (nBinsY_/2); j++) {
          index = i*nBinsY_ + j;
          new_i = i + (nBinsX_/2);
          new_j = j + (nBinsY_/2);
          new_index = new_i*nBinsY_ + new_j;
	  newmag1[new_index] = mag1[index];
	  newmag2[new_index] = mag2[index];
	}
      }
      
      for (unsigned int i=(nBinsX_/2); i< nBinsX_; i++) {
	for(unsigned int j=0; j< (nBinsY_/2); j++) {
	  index = i*nBinsY_ + j;
	  new_i = i - (nBinsX_/2);
	  new_j = j + (nBinsY_/2);
	  new_index = new_i*nBinsY_ + new_j;
	  newmag1[new_index] = mag1[index];
	  newmag2[new_index] = mag2[index];
	}
      }
      
      for (unsigned int i=0; i< (nBinsX_/2); i++) {
	for(unsigned int j=(nBinsY_/2); j< nBinsY_; j++) {
	  index = i*nBinsY_ + j;
	  new_i = i + (nBinsX_/2);
	  new_j = j - (nBinsY_/2);
	  new_index = new_i*nBinsY_ + new_j;
	  newmag1[new_index] = mag1[index];
	  newmag2[new_index] = mag2[index];
	}
      }
      
      for (unsigned int i=(nBinsX_/2); i< nBinsX_; i++) {
	for(unsigned int j=(nBinsY_/2); j< nBinsY_; j++) {
	  index = i*nBinsY_ + j;
	  new_i = i - (nBinsX_/2);
	  new_j = j - (nBinsY_/2);
	  new_index = new_i*nBinsY_ + new_j;
	  newmag1[new_index] = mag1[index];
	  newmag2[new_index] = mag2[index];
	}
      }
  
      RealType maxfreqx = RealType(nBinsX_) / lenX_;
      RealType maxfreqy = RealType(nBinsY_) / lenY_;
      
      RealType maxfreq = sqrt(maxfreqx*maxfreqx + maxfreqy*maxfreqy);
      dfreq_ = maxfreq/(RealType)(nBins_-1);
          
      int zero_freq_x = nBinsX_/2; 
      int zero_freq_y = nBinsY_/2; 
      
      for (unsigned int i=0; i< nBinsX_; i++) {
	for(unsigned int j=0; j< nBinsY_; j++) {
	  RealType freq_x = (RealType)(i - zero_freq_x)*maxfreqx*2 / nBinsX_;
	  RealType freq_y = (RealType)(j - zero_freq_y)*maxfreqy*2 / nBinsY_;
	  
	  RealType freq = sqrt(freq_x*freq_x + freq_y*freq_y);
	  
	  unsigned int whichbin = (unsigned int) (freq / dfreq_);
          
	  newindex = i*nBinsY_ + j;

          std::cout << newmag1[newindex] << "\t";
          
          dynamic_cast<Accumulator*>(counts_->accumulator[whichbin])->add(1);
          dynamic_cast<Accumulator*>(freq_->accumulator[whichbin])->add(freq);
          dynamic_cast<Accumulator *>(top_->accumulator[whichbin])->add(newmag1[newindex]);
          dynamic_cast<Accumulator *>(bottom_->accumulator[whichbin])->add(newmag2[newindex]);
          
	}
      }
      
    }

    writeOutput();

#else
    sprintf(painCave.errMsg, "Hxy: FFTW support was not compiled in!\n");
    painCave.isFatal = 1;
    simError();  

#endif
  }
    
   
  RealType Hxy::getDensity(RealType r, RealType sigma, RealType rcut) {
    RealType sigma2 = sigma*sigma;
    RealType dens = exp(-r*r/(sigma2*2.0)) /
      (pow(2.0*NumericConstant::PI*sigma2, 3));
    RealType dcut = exp(-rcut*rcut/(sigma2*2.0)) /
      (pow(2.0*NumericConstant::PI*sigma2, 3));
    if (r < rcut) 
      return dens - dcut;
    else
      return 0.0;
  }



}
