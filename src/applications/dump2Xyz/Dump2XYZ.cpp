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
 
#include <iostream>
#include <fstream>
#include <string>

#include "applications/dump2Xyz/Dump2XYZCmd.h"
#include "brains/Register.hpp"
#include "brains/SimCreator.hpp"
#include "brains/SimInfo.hpp"
#include "io/DumpReader.hpp"
#include "utils/simError.h"
#include "visitors/AtomVisitor.hpp"
#include "visitors/CompositeVisitor.hpp"
#include "visitors/RigidBodyVisitor.hpp"
#include "visitors/OtherVisitor.hpp"
#include "visitors/ZconsVisitor.hpp"
#include "selection/SelectionEvaluator.hpp"
#include "selection/SelectionManager.hpp"
#include "visitors/LipidTransVisitor.hpp"
#include "visitors/AtomNameVisitor.hpp"

using namespace oopse;

int main(int argc, char* argv[]){
  
  //register force fields
  registerForceFields();
  
  gengetopt_args_info args_info;
  std::string dumpFileName;
  std::string xyzFileName;
  
  //parse the command line option
  if (cmdline_parser (argc, argv, &args_info) != 0) {
    exit(1) ;
  }
  
  //get the dumpfile name and meta-data file name
  if (args_info.input_given){
    dumpFileName = args_info.input_arg;
  } else {
    std::cerr << "Does not have input file name" << std::endl;
    exit(1);
  }
  
  if (args_info.output_given){
    xyzFileName = args_info.output_arg;
  } else {
    xyzFileName = dumpFileName;
    xyzFileName = xyzFileName.substr(0, xyzFileName.rfind(".")) + ".xyz";
  }
  
  //parse md file and set up the system
  SimCreator creator;
  SimInfo* info = creator.createSim(dumpFileName, false);
  

  //create visitor list
  CompositeVisitor* compositeVisitor = new CompositeVisitor();
    
  //create RigidBody Visitor
  if(args_info.rigidbody_flag){
    RBCOMVisitor* rbCOMVisitor = new RBCOMVisitor(info);
    compositeVisitor->addVisitor(rbCOMVisitor, 900);
  }
  
  //create SSD atom visitor
  SSDAtomVisitor* ssdVisitor = new SSDAtomVisitor(info);
  compositeVisitor->addVisitor(ssdVisitor, 800);
  
  LinearAtomVisitor* linearVisitor = new LinearAtomVisitor(info);
  compositeVisitor->addVisitor(linearVisitor, 750);
  if (args_info.gb_given) {
    linearVisitor->addGayBerneAtomType(args_info.gb_arg);
  }
  
  GBLipidAtomVisitor* gbLipidVisitor = new GBLipidAtomVisitor(info);
  compositeVisitor->addVisitor(gbLipidVisitor, 740);

  Ring5gbAtomVisitor* ring5Visitor = new Ring5gbAtomVisitor(info);
  compositeVisitor->addVisitor(ring5Visitor, 730);

  HeadAtomVisitor* headVisitor = new HeadAtomVisitor(info);
  compositeVisitor->addVisitor(headVisitor, 720);

  //create default atom visitor
  DefaultAtomVisitor* defaultAtomVisitor = new DefaultAtomVisitor(info);
  compositeVisitor->addVisitor(defaultAtomVisitor, 700);
  
  //create waterType visitor
  if(args_info.watertype_flag){
    WaterTypeVisitor* waterTypeVisitor = new WaterTypeVisitor;
    compositeVisitor->addVisitor(waterTypeVisitor, 600);
  }

  if (args_info.basetype_flag) {
      AtomNameVisitor* atomNameVisitor = new AtomNameVisitor(info);
      compositeVisitor->addVisitor(atomNameVisitor, 550);

  }
  
  //create ZconsVisitor
  if(args_info.zconstraint_flag){

    ZConsVisitor* zconsVisitor = new ZConsVisitor(info);
    
    if(zconsVisitor->haveZconsMol()) {
      compositeVisitor->addVisitor(zconsVisitor, 500);
    } else {
      delete zconsVisitor;
    }
  }
  
  //create wrapping visitor
  
  //if(args_info.periodicBox_flag){
  //  WrappingVisitor* wrappingVisitor = new WrappingVisitor(info);
  //  compositeVisitor->addVisitor(wrappingVisitor, 400);
  //}

  //create replicate visitor
  if(args_info.repeatX_given > 0 || args_info.repeatY_given > 0 ||args_info.repeatY_given > 0){
    Vector3i replicateOpt(args_info.repeatX_arg, args_info.repeatY_arg, args_info.repeatZ_arg);
    ReplicateVisitor* replicateVisitor = new ReplicateVisitor(info, replicateOpt);
    compositeVisitor->addVisitor(replicateVisitor, 300);
  }


  //create rotation visitor
  if (args_info.refsele_given&& args_info.originsele_given) {
    compositeVisitor->addVisitor(new LipidTransVisitor(info, args_info.originsele_arg, args_info.refsele_arg), 250); 
  } else if (args_info.refsele_given || args_info.originsele_given) {
    std::cerr << "Both of --refsele and --originsele should appear by pair" << std::endl;
    exit(1);
  }
    
  //create xyzVisitor
  XYZVisitor* xyzVisitor;

  if (args_info.selection_given) {
    xyzVisitor = new XYZVisitor(info, args_info.selection_arg);
  } else {
    xyzVisitor = new XYZVisitor(info);
  }

  if(args_info.printPosOnly_flag){
    bool posOnly = true;
    xyzVisitor->setPosOnly(posOnly);
  }

  compositeVisitor->addVisitor(xyzVisitor, 200);
  
  std::cout << compositeVisitor->toString();
  
  //create prepareVisitor
  PrepareVisitor* prepareVisitor = new PrepareVisitor();
  
  //open dump file
  DumpReader* dumpReader = new DumpReader(info, dumpFileName);
  int nframes = dumpReader->getNFrames();
  
  
  std::ofstream xyzStream(xyzFileName.c_str());
  
  SimInfo::MoleculeIterator miter;
  Molecule::IntegrableObjectIterator  iiter;
  Molecule::RigidBodyIterator rbIter;
  Molecule* mol;
  StuntDouble* integrableObject;
  RigidBody* rb;
  Vector3d molCom;
  Vector3d newMolCom;
  Vector3d displacement;
  Mat3x3d hmat;
  Snapshot* currentSnapshot;
       
  for (int i = 0; i < nframes; i += args_info.frame_arg){
    dumpReader->readFrame(i);

    //wrapping the molecule
    if(args_info.periodicBox_flag) {
      currentSnapshot = info->getSnapshotManager()->getCurrentSnapshot();    
      for (mol = info->beginMolecule(miter); mol != NULL; mol = info->nextMolecule(miter)) {
          molCom = mol->getCom();
          newMolCom = molCom;
          currentSnapshot->wrapVector(newMolCom);
          displacement = newMolCom - molCom;
        for (integrableObject = mol->beginIntegrableObject(iiter); integrableObject != NULL;
             integrableObject = mol->nextIntegrableObject(iiter)) {  
          integrableObject->setPos(integrableObject->getPos() + displacement);
        }
      }    
    }
    //update atoms of rigidbody
    for (mol = info->beginMolecule(miter); mol != NULL; mol = info->nextMolecule(miter)) {
      
      //change the positions of atoms which belong to the rigidbodies
      for (rb = mol->beginRigidBody(rbIter); rb != NULL; rb = mol->nextRigidBody(rbIter)) {
        rb->updateAtoms();
      }
    }
    
    //prepare visit
    for (mol = info->beginMolecule(miter); mol != NULL; mol = info->nextMolecule(miter)) {
      for (integrableObject = mol->beginIntegrableObject(iiter); integrableObject != NULL;
           integrableObject = mol->nextIntegrableObject(iiter)) {
        integrableObject->accept(prepareVisitor);
      }
    }
    
    //update visitor
    compositeVisitor->update();


    //visit stuntdouble
    for (mol = info->beginMolecule(miter); mol != NULL; mol = info->nextMolecule(miter)) {
      for (integrableObject = mol->beginIntegrableObject(iiter); integrableObject != NULL;
           integrableObject = mol->nextIntegrableObject(iiter)) {
        integrableObject->accept(compositeVisitor);
      }
    }
    
    xyzVisitor->writeFrame(xyzStream);
    xyzVisitor->clear();
    
  }//end for (int i = 0; i < nframes; i += args_info.frame_arg)
 
  xyzStream.close();
  delete prepareVisitor; 
  delete compositeVisitor;
  delete info;
}
