/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 2001, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** Fire & Heat Transfer modules developed by:                         **
**   Yaqiang Jiang (y.jiang@ed.ac.uk)                                 **
**   Asif Usmani (asif.usmani@ed.ac.uk)                               **
**                                                                    **
** ****************************************************************** */

//
// Written by Liming Jiang (liming.jiang@ed.ac.uk)
//

#include <TimberHTMaterial.h>
#include <Matrix.h>
#include <OPS_Globals.h>
#include <cmath>
#include <fstream>
#include <iomanip>

using std::ios;
using std::ifstream;

double TimberHTMaterial::epsilon = 1e-5;

TimberHTMaterial::TimberHTMaterial(int tag,int typeTag, HeatTransferDomain* theDomain, Vector matPars)
:HeatTransferMaterial(tag), trial_temp(0.0), charTime(0.0), thePars(0), HtComb(0.0),
 ini_temp(0.0), rho(0), cp(0.0), enthalpy(0.0),TypeTag(typeTag), PhaseTag(0), theHTDomain(theDomain)
{
    if ( k == 0){
		k = new Matrix(3,3);
		if (k == 0) {
			opserr << "TimberHTMaterial::CarbonSteelEN93() - out of memory\n";
			exit(-1);
			}
		}

    pht1 = 0;
    pht2 = 0;
    //phaseTag =0: Wet Wood
    // phaseTag =1: Dry Wood
    // phaseTag =2: Char 
    //PhaseTag =3: Ash
    MatPars = matPars;
}

TimberHTMaterial::TimberHTMaterial(int tag, int typeTag, HeatTransferDomain* theDomain, Matrix thepars, Vector matPars)
    :HeatTransferMaterial(tag), trial_temp(0.0), charTime(0.0), HtComb(0.0),
    ini_temp(0.0), rho(0), cp(0.0), enthalpy(0.0), TypeTag(typeTag), PhaseTag(0), theHTDomain(theDomain)
{
    if (k == 0) {
        k = new Matrix(3, 3);
        if (k == 0) {
            opserr << "TimberHTMaterial::CarbonSteelEN93() - out of memory\n";
            exit(-1);
        }
    }

    thePars = new Matrix(thepars.noRows(), thepars.noCols());
    (*thePars) = thepars;
    MatPars = matPars;
    pht1 = 0;
    pht2 = 0;
    //phaseTag =0: Wet Wood
    // phaseTag =1: Dry Wood
    // phaseTag =2: Char 
    //PhaseTag =3: Ash
    
}

TimberHTMaterial::~TimberHTMaterial()
{
    if (k != 0)
		delete k;
}

int 
TimberHTMaterial::setTrialTemperature(double temp, int par)
{
    trial_temp = temp - 273.15;
    

    double time = theHTDomain->getCurrentTime();

    this->determinePhase(trial_temp, time);

    return 0;
}


const Matrix& 
TimberHTMaterial::getConductivity(void)
{
	double materialK = 0;
	if(PhaseTag ==0){
      materialK =(*thePars)(0,2);
      //wet wood
	}
  else if(PhaseTag ==1){
        materialK = (*thePars)(1, 2);
    //dry wood
  }
  else if(PhaseTag ==2){
        materialK = (*thePars)(2, 2);
    //char
  }
  else if (PhaseTag == 3) {
        materialK = (*thePars)(3, 2);
        //ash
  }
  else
    opserr<<"TimberHTMaterial::unrecognised PhaseTag "<<PhaseTag;
  

	(*k)(0,0) = materialK;
	(*k)(1,1) = materialK;
	(*k)(2,2) = materialK;

    return *k; // change
}


double  
TimberHTMaterial::getRho(void)
{

  if (PhaseTag == 0) {
      rho = (*thePars)(0, 1);
      //wet wood
  }
  else if (PhaseTag == 1) {
      rho = (*thePars)(1, 1);
      //dry wood
  }
  else if (PhaseTag == 2) {
      rho = (*thePars)(2, 1);
      //char
  }
  else if (PhaseTag == 3) {
      rho = (*thePars)(3, 1);
      //ash
  }
  else
      opserr << "TimberHTMaterial::unrecognised PhaseTag " << PhaseTag;

  return rho;
}


double 
TimberHTMaterial::getSpecificHeat(void)
{
  
  if(PhaseTag ==0){
      cp = (*thePars)(0, 3);
  }
  else if(PhaseTag ==1){
      cp = (*thePars)(1, 3);
  
      /*  Temperature based definition
    if (trial_temp < 100.0)
      cp = 1627+ 22.3*trial_temp;
    else if(trial_temp < 400.0)
      cp = 4446 - 5.05*trial_temp;
    else if(trial_temp < 700.0)
      cp = -1336 + 9.37*trial_temp;
    else if(trial_temp < 1200.0)
      cp = -1336 + 9.37*700;
	else 
		opserr<<"SFRM Coating ,invalid temperature"<<trial_temp;
      */

  }
  else if (PhaseTag == 2) {
      cp = (*thePars)(2, 3);
  }
  else if (PhaseTag == 3) {
      cp = (*thePars)(3, 3);
  }
  else
      opserr << "TimberHTMaterial::unrecognised PhaseTag " << PhaseTag;



    return cp;
}


double
TimberHTMaterial::getEnthalpy()
{
    	return 0;	
}


double
TimberHTMaterial::getEnthalpy(double temp)
{
    
    return 0;	
}


HeatTransferMaterial*
TimberHTMaterial::getCopy(void)
{
    TimberHTMaterial* theCopy = 0;
    if(thePars!=0) {
        theCopy = new TimberHTMaterial(this->getTag(), TypeTag, theHTDomain, (*thePars), MatPars);
    }
    else
         theCopy = new TimberHTMaterial(this->getTag(), TypeTag,theHTDomain, MatPars);
    
    theCopy->trial_temp = trial_temp;
    return theCopy;
}


void
TimberHTMaterial::update()
{
    return; 
}


int 
TimberHTMaterial::commitState(void)
{
    return 0;
}


int 
TimberHTMaterial::revertToLastCommit(void)
{
    return 0;
}


int 
TimberHTMaterial::revertToStart(void)
{
    return 0;
}


int
TimberHTMaterial::determinePhase(double temp, double time)
{
    double T1, T2, T3;
    double dt1, dt2, dt3;

    if (MatPars.Size() == 3) {
        T1 = (*thePars)(1, 0); dt1 = MatPars(0);
        T2 = (*thePars)(2, 0); dt2 = MatPars(1);
        T3 = (*thePars)(3, 0); dt3 = MatPars(2);
    }
    else if (MatPars.Size() == 4) {
        T1 = (*thePars)(1, 0); dt1 = MatPars(0);
        T2 = (*thePars)(2, 0); dt2 = MatPars(1);
        T3 = (*thePars)(3, 0); dt3 = MatPars(2);
        HtComb = MatPars(3);

    }
    else if (MatPars.Size() == 6) {
        T1 = MatPars(0); dt1 = MatPars(1);
        T2 = MatPars(2); dt2 = MatPars(3);
        T3 = MatPars(4); dt3 = MatPars(5);
    }
    else
        opserr << "Timber Material recieves incorrect material properties" << endln;



    if (temp < T1) {
        PhaseTag = 0;
        pht1 = 0;
        //Wet wood
    }
    else if (temp < T2)
    {
        if (pht1<1e-6 && PhaseTag<1) {
            pht1 = time;
        }
        else {
            pht2 = time;
        }

        if ((pht2 - pht1) > dt1) {
            PhaseTag = 1;
            pht1 = 0;
        }
        //dry wood
    }
    else if (temp < T3)
    {
        if (pht1 < 1e-6 && PhaseTag<2) {
            pht1 = time;
        }
        else {
            pht2 = time;
        }

        if ((pht2 - pht1) > dt2) {
            PhaseTag = 2;
            pht1 = 0;
            if (charTime < 1e-6)
                charTime = time;
        }

        //char
    }
    else {
        if (pht1 < 1e-6 && PhaseTag<3) {
            pht1 = time;
        }
        else {
            pht2 = time;
        }

        if ((pht2 - pht1) > dt3) {
            PhaseTag = 3;
            pht1 = 0;
        }
        //ash
    }

      
    return 0;
}


bool
TimberHTMaterial::getIfHeatGen()
{
    if (PhaseTag == 2 || PhaseTag == 3)
        return true;
    else
        return false;
}


double
TimberHTMaterial::getHeatGen()
{
    double Qgen = 0;
    if (PhaseTag == 2)
    {
        Qgen = HtComb;
    }
    return Qgen ;
}



const Vector&
TimberHTMaterial::getPars() {
    static Vector pars(2);
    pars(0) = PhaseTag;
    pars(1) = charTime;

    return pars;

}



