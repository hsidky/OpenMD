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
 
#include <cmath>
#include "constraints/ZconstraintForceManager.hpp"
#include "integrators/Integrator.hpp"
#include "utils/simError.h"
#include "utils/OOPSEConstant.hpp"
#include "utils/StringUtils.hpp"
namespace oopse {
ZconstraintForceManager::ZconstraintForceManager(SimInfo* info): ForceManager(info), infiniteTime(1e31) {
    currSnapshot_ = info_->getSnapshotManager()->getCurrentSnapshot();
    Globals* simParam = info_->getSimParams();

    if (simParam->haveDt()){
        dt_ = simParam->getDt();
    } else {
        sprintf(painCave.errMsg,
                "Integrator Error: dt is not set\n");
        painCave.isFatal = 1;
        simError();    
    }

    if (simParam->haveZconstraintTime()){
        zconsTime_ = simParam->getZconsTime();
    }
    else{
        sprintf(painCave.errMsg,
        "ZConstraint error: If you use a ZConstraint,\n"
        "\tyou must set zconsTime.\n");
        painCave.isFatal = 1;
        simError();
    }

    if (simParam->haveZconsTol()){
        zconsTol_ = simParam->getZconsTol();
    }
    else{
        zconsTol_ = 0.01;
        sprintf(painCave.errMsg,
            "ZConstraint Warning: Tolerance for z-constraint method is not specified.\n"
            "\tOOPSE will use a default value of %f.\n"
            "\tTo set the tolerance, use the zconsTol variable.\n",
            zconsTol_);
        painCave.isFatal = 0;
        simError();      
    }

    //set zcons gap
    if (simParam->haveZConsGap()){
        usingZconsGap_ = true;
        zconsGap_ = simParam->getZconsGap();
    }else {
        usingZconsGap_ = false;
        zconsGap_ = 0.0;
    }

    //set zcons fixtime
    if (simParam->haveZConsFixTime()){
        zconsFixingTime_ = simParam->getZconsFixtime();
    } else {
        zconsFixingTime_ = infiniteTime;
    }

    //set zconsUsingSMD
    if (simParam->haveZConsUsingSMD()){
        usingSMD_ = simParam->getZconsUsingSMD();
    }else {
        usingSMD_ =false;
    }
    
    zconsOutput_ = getPrefix(info_->getFinalConfigFileName()) + ".fz";

    //estimate the force constant of harmonical potential
    Mat3x3d hmat = currSnapshot_->getHmat();
    double halfOfLargestBox = std::max(hmat(0, 0), std::max(hmat(1, 1), hmat(2, 2))) /2;	
    double targetTemp;
    if (simParam->haveTargetTemp()) {
        targetTemp = simParam->getTargetTemp();
    } else {
        targetTemp = 298.0;
    }
    double zforceConstant = OOPSEConstant::kb * targetTemp / (halfOfLargestBox * halfOfLargestBox);
         
    int nZconstraints = simParam->getNzConstraints();
    ZconStamp** stamp = simParam->getZconStamp();
    //
    for (int i = 0; i < nZconstraints; i++){

        ZconstraintParam param;
        int zmolIndex = stamp[i]->getMolIndex();
        if (stamp[i]->haveZpos()) {
            param.zTargetPos = stamp[i]->getZpos();
        } else {
            param.zTargetPos = getZTargetPos(zmolIndex);
        }

        param.kz = zforceConstant * stamp[i]->getKratio();

        if (stamp[i]->haveCantVel()) {
            param.cantVel = stamp[i]->getCantVel();
        } else {
            param.cantVel = 0.0;
        }

        allZMolIndices_.insert(std::make_pair(zmolIndex, param));
    }

    //create fixedMols_, movingMols_ and unconsMols lists 
    update();
    
    //calculate masss of unconstraint molecules in the whole system (never change during the simulation)
    double totMassUnconsMols_local = 0.0;    
    std::vector<Molecule*>::iterator j;
    for ( j = unzconsMols_.begin(); j !=  unzconsMols_.end(); ++j) {
        totMassUnconsMols_local += (*j)->getMass();
    }    
#ifndef IS_MPI
    totMassUnconsMols_ = totMassUnconsMols_local;
#else
    MPI_Allreduce(&totMassUnconsMols_local, &totMassUnconsMols_, 1, MPI_DOUBLE,
        MPI_SUM, MPI_COMM_WORLD);  
#endif

    // creat zconsWriter  
    fzOut = new ZConsWriter(info_, zconsOutput_.c_str());   

    if (!fzOut){
      sprintf(painCave.errMsg, "Fail to create ZConsWriter\n");
      painCave.isFatal = 1;
      simError();
    }

}

ZconstraintForceManager::~ZconstraintForceManager(){

  if (fzOut){
    delete fzOut;
  }

}

void ZconstraintForceManager::update(){
    fixedZMols_.clear();
    movingZMols_.clear();
    unzconsMols_.clear();

    for (std::map<int, ZconstraintParam>::iterator i = allZMolIndices_.begin(); i != allZMolIndices_.end(); ++i) {
#ifdef IS_MPI
        if (info_->getMolToProc(i->first) == worldRank) {
#endif
            ZconstraintMol zmol;
            zmol.mol = info_->getMoleculeByGlobalIndex(i->first);
            assert(zmol.mol);
            zmol.param = i->second;
            zmol.cantPos = zmol.param.zTargetPos; /**@todo fixed me when zmol migrate, it is incorrect*/
            Vector3d com = zmol.mol->getCom();
            double diff = fabs(zmol.param.zTargetPos - com[whichDirection]);
            if (diff < zconsTol_) {
                fixedZMols_.push_back(zmol);
            } else {
                movingZMols_.push_back(zmol);            
            }

#ifdef IS_MPI
        }
#endif
    }

    calcTotalMassMovingZMols();

    std::set<int> zmolSet;
    for (std::list<ZconstraintMol>::iterator i = movingZMols_.begin(); i !=  movingZMols_.end(); ++i) {
        zmolSet.insert(i->mol->getGlobalIndex());
    }    

    for (std::list<ZconstraintMol>::iterator i = fixedZMols_.begin(); i !=  fixedZMols_.end(); ++i) {
        zmolSet.insert(i->mol->getGlobalIndex());
    }

    SimInfo::MoleculeIterator mi;
    Molecule* mol;
    for(mol = info_->beginMolecule(mi); mol != NULL; mol = info_->nextMolecule(mi)) {
        if (zmolSet.find(mol->getGlobalIndex()) == zmolSet.end()) {
            unzconsMols_.push_back(mol);
        }
    }

}

bool ZconstraintForceManager::isZMol(Molecule* mol){
    return allZMolIndices_.find(mol->getGlobalIndex()) == allZMolIndices_.end() ? false : true;
}

void ZconstraintForceManager::init(){

  //zero out the velocities of center of mass of unconstrained molecules 
  //and the velocities of center of mass of every single z-constrained molecueles
  zeroVelocity();

  currZconsTime_ = currSnapshot_->getTime();
}

void ZconstraintForceManager::calcForces(bool needPotential, bool needStress){
    ForceManager::calcForces(needPotential, needStress);
    
    if (usingZconsGap_){
        updateZPos();
    }

    if (checkZConsState()){
        zeroVelocity();    
        calcTotalMassMovingZMols();
    }  

    //do zconstraint force; 
    if (haveFixedZMols()){
        doZconstraintForce();
    }

    //use external force to move the molecules to the specified positions
    if (haveMovingZMols()){
        doHarmonic();
    }

    //write out forces and current positions of z-constraint molecules    
    if (currSnapshot_->getTime() >= currZconsTime_){
        std::list<ZconstraintMol>::iterator i;
        Vector3d com;
        for(i = fixedZMols_.begin(); i != fixedZMols_.end(); ++i) {
            com = i->mol->getCom();
            i->zpos = com[whichDirection];
        }
        
        fzOut->writeFZ(fixedZMols_);
        currZconsTime_ += zconsTime_;
    }
}

void ZconstraintForceManager::zeroVelocity(){

    Vector3d comVel;
    Vector3d vel;
    std::list<ZconstraintMol>::iterator i;
    Molecule* mol;
    StuntDouble* integrableObject;
    Molecule::IntegrableObjectIterator ii;

    //zero out the velocities of center of mass of fixed z-constrained molecules
    for(i = fixedZMols_.begin(); i != fixedZMols_.end(); ++i) {
        mol = i->mol;
        comVel = mol->getComVel();
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {
            vel = integrableObject->getVel();  
            vel[whichDirection] -= comVel[whichDirection];
            integrableObject->setVel(vel);
        }
    }

    // calculate the vz of center of mass of moving molecules(include unconstrained molecules 
    // and moving z-constrained molecules)  
    double pzMovingMols_local = 0.0;
    double pzMovingMols;
    
    for ( i = movingZMols_.begin(); i !=  movingZMols_.end(); ++i) {
        mol = i->mol;        
        comVel = mol->getComVel();
        pzMovingMols_local +=  mol->getMass() * comVel[whichDirection];   
    }

    std::vector<Molecule*>::iterator j;
    for ( j = unzconsMols_.begin(); j !=  unzconsMols_.end(); ++j) {
        mol =*j;
        comVel = mol->getComVel();
        pzMovingMols_local += mol->getMass() * comVel[whichDirection];
    }
    
#ifndef IS_MPI
    pzMovingMols = pzMovingMols_local;
#else
    MPI_Allreduce(&pzMovingMols_local, &pzMovingMols, 1, MPI_DOUBLE,
        MPI_SUM, MPI_COMM_WORLD);
#endif

    double vzMovingMols = pzMovingMols / (totMassMovingZMols_ + totMassUnconsMols_);

    //modify the velocities of moving z-constrained molecuels
    for ( i = movingZMols_.begin(); i !=  movingZMols_.end(); ++i) {
        mol = i->mol;
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {

            vel = integrableObject->getVel();
            vel[whichDirection] -= vzMovingMols;
            integrableObject->setVel(vel); 
        }
    }

    //modify the velocites of unconstrained molecules  
    for ( j = unzconsMols_.begin(); j !=  unzconsMols_.end(); ++j) {
        mol =*j;
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {

            vel = integrableObject->getVel();
            vel[whichDirection] -= vzMovingMols;
            integrableObject->setVel(vel); 
        }
    }
    
}


void ZconstraintForceManager::doZconstraintForce(){
  double totalFZ; 
  double totalFZ_local;
  Vector3d com;
  Vector3d force(0.0);

  //constrain the molecules which do not reach the specified positions  

  //Zero Out the force of z-contrained molecules    
  totalFZ_local = 0;


    //calculate the total z-contrained force of fixed z-contrained molecules
    std::list<ZconstraintMol>::iterator i;
    Molecule* mol;
    StuntDouble* integrableObject;
    Molecule::IntegrableObjectIterator ii;

    for ( i = fixedZMols_.begin(); i !=  fixedZMols_.end(); ++i) {
        mol = i->mol;
        i->fz = 0.0;
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {

            force = integrableObject->getFrc();    
            i->fz += force[whichDirection]; 
        }
         totalFZ_local += i->fz;
    }

  //calculate total z-constraint force
#ifdef IS_MPI
  MPI_Allreduce(&totalFZ_local, &totalFZ, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
#else
  totalFZ = totalFZ_local;
#endif


     // apply negative to fixed z-constrained molecues;
    for ( i = fixedZMols_.begin(); i !=  fixedZMols_.end(); ++i) {
        mol = i->mol;
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {

            force[whichDirection] = -getZFOfFixedZMols(mol, integrableObject, i->fz);
            integrableObject->addFrc(force);
        }
    }

  //modify the forces of moving z-constrained molecules
    for ( i = movingZMols_.begin(); i !=  movingZMols_.end(); ++i) {
        mol = i->mol;
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {

            force[whichDirection] = -getZFOfMovingMols(mol,totalFZ);
            integrableObject->addFrc(force);
        }
    }

  //modify the forces of unconstrained molecules
    std::vector<Molecule*>::iterator j;
    for ( j = unzconsMols_.begin(); j !=  unzconsMols_.end(); ++j) {
        mol =*j;
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {

            force[whichDirection] = -getZFOfMovingMols(mol, totalFZ);
            integrableObject->addFrc(force);
        }
    }

}


void ZconstraintForceManager::doHarmonic(){
    double totalFZ;
    Vector3d force(0.0);
    Vector3d com;
    double totalFZ_local = 0;
    std::list<ZconstraintMol>::iterator i;
    StuntDouble* integrableObject;
    Molecule::IntegrableObjectIterator ii;
    Molecule* mol;
    for ( i = movingZMols_.begin(); i !=  movingZMols_.end(); ++i) {
        mol = i->mol;
        com = mol->getCom();   
        double resPos = usingSMD_? i->cantPos : i->param.zTargetPos;
        double diff = com[whichDirection] - resPos; 
        double harmonicU = 0.5 * i->param.kz * diff * diff;
        currSnapshot_->statData[Stats::LONG_RANGE_POTENTIAL] += harmonicU;
        double harmonicF = -i->param.kz * diff;
        totalFZ_local += harmonicF;

        //adjust force
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {

            force[whichDirection] = getHFOfFixedZMols(mol, integrableObject, harmonicF);
            integrableObject->addFrc(force);            
        }
    }

#ifndef IS_MPI
  totalFZ = totalFZ_local;
#else
  MPI_Allreduce(&totalFZ_local, &totalFZ, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);  
#endif

    //modify the forces of unconstrained molecules
    std::vector<Molecule*>::iterator j;
    for ( j = unzconsMols_.begin(); j !=  unzconsMols_.end(); ++j) {
        mol = *j;
        for(integrableObject = mol->beginIntegrableObject(ii); integrableObject != NULL; 
            integrableObject = mol->nextIntegrableObject(ii)) {

            force[whichDirection] = getHFOfUnconsMols(mol, totalFZ);
            integrableObject->addFrc(force);            
        }
    }

}

bool ZconstraintForceManager::checkZConsState(){
    Vector3d com;
    double diff;
    int changed_local = 0;

    std::list<ZconstraintMol>::iterator i;
    std::list<ZconstraintMol>::iterator j;
    
    std::list<ZconstraintMol> newMovingZMols;
    for ( i = fixedZMols_.begin(); i !=  fixedZMols_.end();) {
        com = i->mol->getCom();
        diff = fabs(com[whichDirection] - i->param.zTargetPos);
        if (diff > zconsTol_) {
            if (usingZconsGap_) {
                i->endFixingTime = infiniteTime;
            }
            j = i++;
            newMovingZMols.push_back(*j);
            fixedZMols_.erase(j);
	    
            changed_local = 1;            
        }else {
            ++i;
        }
    }  

    std::list<ZconstraintMol> newFixedZMols;
    for ( i = movingZMols_.begin(); i !=  movingZMols_.end();) {
        com = i->mol->getCom();
        diff = fabs(com[whichDirection] - i->param.zTargetPos);
        if (diff <= zconsTol_) {
            if (usingZconsGap_) {
                i->endFixingTime = currSnapshot_->getTime() + zconsFixingTime_;
            }
            //this moving zconstraint molecule is about to fixed
	    //moved this molecule to
	    j = i++;
	    newFixedZMols.push_back(*j);
	    movingZMols_.erase(j);
            changed_local = 1;            
        }else {
            ++i;
        }
    }     

    //merge the lists
    fixedZMols_.insert(fixedZMols_.end(), newFixedZMols.begin(), newFixedZMols.end());
    movingZMols_.insert(movingZMols_.end(), newMovingZMols.begin(), newMovingZMols.end());

    int changed;
#ifndef IS_MPI
    changed = changed_local; 
#else
    MPI_Allreduce(&changed_local, &changed, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
#endif

    return (changed > 0);
}

bool ZconstraintForceManager::haveFixedZMols(){
  int havingFixed;
  int havingFixed_local = fixedZMols_.empty() ? 0 : 1;

#ifndef IS_MPI
  havingFixed = havingFixed_local;
#else
  MPI_Allreduce(&havingFixed_local, &havingFixed, 1, MPI_INT, MPI_SUM,
                MPI_COMM_WORLD);
#endif

  return havingFixed > 0;
}


bool ZconstraintForceManager::haveMovingZMols(){
  int havingMoving_local;
  int havingMoving;

  havingMoving_local = movingZMols_.empty()? 0 : 1;

#ifndef IS_MPI
  havingMoving = havingMoving_local;
#else
  MPI_Allreduce(&havingMoving_local, &havingMoving, 1, MPI_INT, MPI_SUM,
                MPI_COMM_WORLD);
#endif

  return havingMoving > 0;
}

void ZconstraintForceManager::calcTotalMassMovingZMols(){

    double totMassMovingZMols_local = 0.0;
    std::list<ZconstraintMol>::iterator i;
    for ( i = movingZMols_.begin(); i !=  movingZMols_.end(); ++i) {
        totMassMovingZMols_local += i->mol->getMass();
    }
    
#ifdef IS_MPI
    MPI_Allreduce(&totMassMovingZMols_local, &totMassMovingZMols_, 1, MPI_DOUBLE,
                MPI_SUM, MPI_COMM_WORLD);
#else
    totMassMovingZMols_ = totMassMovingZMols_local;
#endif

}

double ZconstraintForceManager::getZFOfFixedZMols(Molecule* mol, StuntDouble* sd, double totalForce){
  return totalForce * sd->getMass() / mol->getMass();
}

double ZconstraintForceManager::getZFOfMovingMols(Molecule* mol, double totalForce){
  return totalForce * mol->getMass() / (totMassUnconsMols_ + totMassMovingZMols_);
}

double ZconstraintForceManager::getHFOfFixedZMols(Molecule* mol, StuntDouble*sd, double totalForce){
  return totalForce * sd->getMass() / mol->getMass();
}

double ZconstraintForceManager::getHFOfUnconsMols(Molecule* mol, double totalForce){
  return totalForce * mol->getMass() / totMassUnconsMols_;
}

void ZconstraintForceManager::updateZPos(){
    double curTime = currSnapshot_->getTime();
    std::list<ZconstraintMol>::iterator i;
    for ( i = fixedZMols_.begin(); i !=  fixedZMols_.end(); ++i) {
        i->param.zTargetPos += zconsGap_;     
    }  
}

void ZconstraintForceManager::updateCantPos(){
    std::list<ZconstraintMol>::iterator i;
    for ( i = movingZMols_.begin(); i !=  movingZMols_.end(); ++i) {
        i->cantPos += i->param.cantVel * dt_;
    }
}

double ZconstraintForceManager::getZTargetPos(int index){
    double zTargetPos;
#ifndef IS_MPI    
    Molecule* mol = info_->getMoleculeByGlobalIndex(index);
    assert(mol);
    Vector3d com = mol->getCom();
    zTargetPos = com[whichDirection];
#else
    int whicProc = info_->getMolToProc(index);
    MPI_Bcast(&zTargetPos, 1, MPI_DOUBLE, whicProc, MPI_COMM_WORLD);
#endif
    return zTargetPos;
}

}