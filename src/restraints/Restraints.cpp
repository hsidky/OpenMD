#include <iostream>
#include <stdlib.h>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <math.h>

using namespace std;

#include "restraints/Restraints.hpp"
#include "brains/SimInfo.hpp"
#include "utils/simError.h"

#define PI 3.14159265359
#define TWO_PI 6.28318530718

Restraints::Restraints(double lambdaVal, double lambdaExp){
  lambdaValue = lambdaVal;
  lambdaK = lambdaExp;

  const char *jolt = " \t\n;,";

#ifdef IS_MPI
  if(worldRank == 0 ){
#endif // is_mpi

    strcpy(springName, "HarmSpringConsts.txt");
    
    ifstream springs(springName);
    
    if (!springs) { 
      sprintf(painCave.errMsg,
	      "In Restraints: Unable to open HarmSpringConsts.txt for reading.\n"
	      "\tDefault spring constants will be loaded. If you want to specify\n"
	      "\tspring constants, include a three line HarmSpringConsts.txt file\n"
	      "\tin the current directory.\n");
      painCave.severity = OOPSE_WARNING;
      painCave.isFatal = 0;
      simError();   
      
      // load default spring constants
      kDist  = 6;  // spring constant in units of kcal/(mol*ang^2)
      kTheta = 7.5;   // in units of kcal/mol
      kOmega = 13.5;   // in units of kcal/mol
    } else  {
      
      springs.getline(inLine,999,'\n');
      springs.getline(inLine,999,'\n');
      token = strtok(inLine,jolt);
      token = strtok(NULL,jolt);
      strcpy(inValue,token);
      kDist = (atof(inValue));
      springs.getline(inLine,999,'\n');
      token = strtok(inLine,jolt);
      token = strtok(NULL,jolt);
      strcpy(inValue,token);
      kTheta = (atof(inValue));
      springs.getline(inLine,999,'\n');
      token = strtok(inLine,jolt);
      token = strtok(NULL,jolt);
      strcpy(inValue,token);
      kOmega = (atof(inValue));
      springs.close();
    }
#ifdef IS_MPI
  }
  
  MPI_Bcast(&kDist, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&kTheta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&kOmega, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD); 
  
  sprintf( checkPointMsg,
           "Sucessfully opened and read spring file.\n");
  MPIcheckPoint();

#endif // is_mpi
  
  sprintf(painCave.errMsg,
	  "The spring constants for thermodynamic integration are:\n"
	  "\tkDist = %lf\n"
	  "\tkTheta = %lf\n"
	  "\tkOmega = %lf\n", kDist, kTheta, kOmega);
  painCave.severity = OOPSE_INFO;
  painCave.isFatal = 0;
  simError();   
}

Restraints::~Restraints(){
}

void Restraints::Calc_rVal(double position[3], int currentMol){
  delRx = position[0] - cofmPosX[currentMol];
  delRy = position[1] - cofmPosY[currentMol];
  delRz = position[2] - cofmPosZ[currentMol];

  return;
}

void Restraints::Calc_body_thetaVal(double matrix[3][3], int currentMol){
  ub0x = matrix[0][0]*uX0[currentMol] + matrix[0][1]*uY0[currentMol]
    + matrix[0][2]*uZ0[currentMol];
  ub0y = matrix[1][0]*uX0[currentMol] + matrix[1][1]*uY0[currentMol]
    + matrix[1][2]*uZ0[currentMol];
  ub0z = matrix[2][0]*uX0[currentMol] + matrix[2][1]*uY0[currentMol]
    + matrix[2][2]*uZ0[currentMol];

  normalize = sqrt(ub0x*ub0x + ub0y*ub0y + ub0z*ub0z);
  ub0x = ub0x/normalize;
  ub0y = ub0y/normalize;
  ub0z = ub0z/normalize;

  // Theta is the dot product of the reference and new z-axes
  theta = acos(ub0z);

  return;
}

void Restraints::Calc_body_omegaVal(double matrix[3][3], double zAngle){
  double zRotator[3][3];
  double tempOmega;
  double wholeTwoPis;
  // Use the omega accumulated from the rotation propagation
  omega = zAngle;

  // translate the omega into a range between -PI and PI
  if (omega < -PI){
    tempOmega = omega / -TWO_PI;
    wholeTwoPis = floor(tempOmega);
    tempOmega = omega + TWO_PI*wholeTwoPis;
    if (tempOmega < -PI)
      omega = tempOmega + TWO_PI;
    else
      omega = tempOmega;
  }
  if (omega > PI){
    tempOmega = omega / TWO_PI;
    wholeTwoPis = floor(tempOmega);
    tempOmega = omega - TWO_PI*wholeTwoPis;
    if (tempOmega > PI)
      omega = tempOmega - TWO_PI;   
    else
      omega = tempOmega;
  }

  vb0x = sin(omega);
  vb0y = cos(omega);
  vb0z = 0.0;

  normalize = sqrt(vb0x*vb0x + vb0y*vb0y + vb0z*vb0z);
  vb0x = vb0x/normalize;
  vb0y = vb0y/normalize;
  vb0z = vb0z/normalize;

  return;
}

double Restraints::Calc_Restraint_Forces(vector<StuntDouble*> vecParticles){
  double pos[3];
  double A[3][3];
  double tolerance;
  double tempPotent;
  double factor;
  double spaceTrq[3];
  double omegaPass;

  tolerance = 5.72957795131e-7;

  harmPotent = 0.0;  // zero out the global harmonic potential variable

  factor = 1 - pow(lambdaValue, lambdaK);

  for (i=0; i<vecParticles.size(); i++){
    if (vecParticles[i]->isDirectional()){
      vecParticles[i]->getPos(pos);
      vecParticles[i]->getA(A);
      Calc_rVal( pos, i );
      Calc_body_thetaVal( A, i );
      omegaPass = vecParticles[i]->getZangle();
      Calc_body_omegaVal( A, omegaPass );

      if (omega > PI || omega < -PI)
	cout << "oops... " << omega << "\n";

      // first we calculate the derivatives
      dVdrx = -kDist*delRx;
      dVdry = -kDist*delRy;
      dVdrz = -kDist*delRz;

      // uTx... and vTx... are the body-fixed z and y unit vectors
      uTx = 0.0;
      uTy = 0.0;
      uTz = 1.0;
      vTx = 0.0;
      vTy = 1.0;
      vTz = 0.0;

      dVdux = 0;
      dVduy = 0;
      dVduz = 0;
      dVdvx = 0;
      dVdvy = 0;
      dVdvz = 0;

      if (fabs(theta) > tolerance) {
	dVdux = -(kTheta*theta/sin(theta))*ub0x;
	dVduy = -(kTheta*theta/sin(theta))*ub0y;
	dVduz = -(kTheta*theta/sin(theta))*ub0z;
      }

      if (fabs(omega) > tolerance) {
	dVdvx = -(kOmega*omega/sin(omega))*vb0x;
	dVdvy = -(kOmega*omega/sin(omega))*vb0y;
	dVdvz = -(kOmega*omega/sin(omega))*vb0z;
      }

      // next we calculate the restraint forces and torques
      restraintFrc[0] = dVdrx;
      restraintFrc[1] = dVdry;
      restraintFrc[2] = dVdrz;
      tempPotent = 0.5*kDist*(delRx*delRx + delRy*delRy + delRz*delRz);

      restraintTrq[0] = 0.0;
      restraintTrq[1] = 0.0;
      restraintTrq[2] = 0.0;

      if (fabs(omega) > tolerance) {
	restraintTrq[0] += 0.0;
	restraintTrq[1] += 0.0;
	restraintTrq[2] += vTy*dVdvx;
	tempPotent += 0.5*(kOmega*omega*omega);
      }
      if (fabs(theta) > tolerance) {
	restraintTrq[0] += (uTz*dVduy);
	restraintTrq[1] += -(uTz*dVdux);
	restraintTrq[2] += 0.0;
	tempPotent += 0.5*(kTheta*theta*theta);
      }

      for (j = 0; j < 3; j++) {
	restraintFrc[j] *= factor;
	restraintTrq[j] *= factor;
      }

      harmPotent += tempPotent;

      // now we need to convert from body-fixed torques to space-fixed torques
      spaceTrq[0] = A[0][0]*restraintTrq[0] + A[1][0]*restraintTrq[1] 
	+ A[2][0]*restraintTrq[2];
      spaceTrq[1] = A[0][1]*restraintTrq[0] + A[1][1]*restraintTrq[1] 
	+ A[2][1]*restraintTrq[2];
      spaceTrq[2] = A[0][2]*restraintTrq[0] + A[1][2]*restraintTrq[1] 
	+ A[2][2]*restraintTrq[2];

      // now it's time to pass these temporary forces and torques
      // to the total forces and torques
      vecParticles[i]->addFrc(restraintFrc);
      vecParticles[i]->addTrq(spaceTrq);
    }
  }

  // and we can return the appropriately scaled potential energy
  tempPotent = harmPotent * factor;
  return tempPotent;
}

void Restraints::Store_Init_Info(vector<StuntDouble*> vecParticles){
  double pos[3];
  double A[3][3];
  double RfromQ[3][3];
  double quat0, quat1, quat2, quat3;
  double dot;
//   char *token;
//   char fileName[200];
//   char angleName[200];
//   char inLine[1000];
//   char inValue[200];
  const char *delimit = " \t\n;,";

  //open the idealCrystal.in file and zAngle.ang file
  strcpy(fileName, "idealCrystal.in");
  strcpy(angleName, "zAngle.ang");
  
  ifstream crystalIn(fileName);
  ifstream angleIn(angleName);

  if (!crystalIn) { 
    sprintf(painCave.errMsg,
	    "Restraints Error: Unable to open idealCrystal.in for reading.\n"
	    "\tMake sure a reference crystal file is in the current directory.\n");
    painCave.isFatal = 1;
    simError();   
    
    return;
  }

  if (!angleIn) { 
    sprintf(painCave.errMsg,
	    "Restraints Warning: The lack of a zAngle.ang file is mildly\n"
	    "\tunsettling... This means the simulation is starting from the\n"
	    "\tidealCrystal.in reference configuration, so the omega values\n"
	    "\twill all be set to zero. If this is not the case, you should\n"
	    "\tquestion your results.\n");
    painCave.isFatal = 0;
    simError();   
  }

  // A rather specific reader for OOPSE .eor files...
  // Let's read in the perfect crystal file
  crystalIn.getline(inLine,999,'\n');
  crystalIn.getline(inLine,999,'\n');
  
  for (i=0; i<vecParticles.size(); i++) {
    crystalIn.getline(inLine,999,'\n');
    token = strtok(inLine,delimit);
    token = strtok(NULL,delimit);
    strcpy(inValue,token);
    cofmPosX.push_back(atof(inValue));
    token = strtok(NULL,delimit);
    strcpy(inValue,token);
    cofmPosY.push_back(atof(inValue));
    token = strtok(NULL,delimit);
    strcpy(inValue,token);
    cofmPosZ.push_back(atof(inValue));
    token = strtok(NULL,delimit);
    token = strtok(NULL,delimit);
    token = strtok(NULL,delimit);
    token = strtok(NULL,delimit);
    strcpy(inValue,token);
    quat0 = atof(inValue);
    token = strtok(NULL,delimit);
    strcpy(inValue,token);
    quat1 = atof(inValue);
    token = strtok(NULL,delimit);
    strcpy(inValue,token);
    quat2 = atof(inValue);
    token = strtok(NULL,delimit);
    strcpy(inValue,token);
    quat3 = atof(inValue);

    // now build the rotation matrix and find the unit vectors
    RfromQ[0][0] = quat0*quat0 + quat1*quat1 - quat2*quat2 - quat3*quat3;
    RfromQ[0][1] = 2*(quat1*quat2 + quat0*quat3);
    RfromQ[0][2] = 2*(quat1*quat3 - quat0*quat2);
    RfromQ[1][0] = 2*(quat1*quat2 - quat0*quat3);
    RfromQ[1][1] = quat0*quat0 - quat1*quat1 + quat2*quat2 - quat3*quat3;
    RfromQ[1][2] = 2*(quat2*quat3 + quat0*quat1);
    RfromQ[2][0] = 2*(quat1*quat3 + quat0*quat2);
    RfromQ[2][1] = 2*(quat2*quat3 - quat0*quat1);
    RfromQ[2][2] = quat0*quat0 - quat1*quat1 - quat2*quat2 + quat3*quat3;
    
    normalize = sqrt(RfromQ[2][0]*RfromQ[2][0] + RfromQ[2][1]*RfromQ[2][1] 
		     + RfromQ[2][2]*RfromQ[2][2]);
    uX0.push_back(RfromQ[2][0]/normalize);
    uY0.push_back(RfromQ[2][1]/normalize);
    uZ0.push_back(RfromQ[2][2]/normalize);

    normalize = sqrt(RfromQ[1][0]*RfromQ[1][0] + RfromQ[1][1]*RfromQ[1][1]
		     + RfromQ[1][2]*RfromQ[1][2]);
    vX0.push_back(RfromQ[1][0]/normalize);
    vY0.push_back(RfromQ[1][1]/normalize);
    vZ0.push_back(RfromQ[1][2]/normalize);
  }

  // now we can read in the zAngle.ang file
  if (angleIn){
    angleIn.getline(inLine,999,'\n');
    for (i=0; i<vecParticles.size(); i++) {
      angleIn.getline(inLine,999,'\n');
      token = strtok(inLine,delimit);
      strcpy(inValue,token);
      vecParticles[i]->setZangle(atof(inValue));
    }
  }

  return;
}

void Restraints::Write_zAngle_File(vector<StuntDouble*> vecParticles){

  char zOutName[200];

  strcpy(zOutName,"zAngle.ang");

  ofstream angleOut(zOutName);
  angleOut << "This file contains the omega values for the .eor file\n";
  for (i=0; i<vecParticles.size(); i++) {
    angleOut << vecParticles[i]->getZangle() << "\n";
  }
  return;
}

double Restraints::getVharm(){
  return harmPotent;
}
