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
:HeatTransferMaterial(tag), trial_temp(0.0), charTime(0.0), thePars(0), HtComb(0.0), trialphTag(0), TempTag(0),
 ini_temp(0.0), rho(0), cp(0.0), enthalpy(0.0),TypeTag(typeTag), PhaseTag(0), theHTDomain(theDomain)
{
    if ( k == 0){
		k = new Matrix(3,3);
		if (k == 0) {
			opserr << "TimberHTMaterial::CarbonSteelEN93() - out of memory\n";
			exit(-1);
			}
		}
    rho0 = 0;
    moist = 0;
    pht1 = 0;
    pht2 = 0;
    //phaseTag =0: Wet Wood
    // phaseTag =1: Dry Wood
    // phaseTag =2: Char 
    //PhaseTag =3: Ash
    MatPars = matPars;
    if (typeTag == 0) {
        rho0 = MatPars(0);
        moist = MatPars(1);
    }

    T1 = 0; T2 = 0; T3 = 0;
    dt1 = 0; dt2 = 0; dt3 = 0;

}

TimberHTMaterial::TimberHTMaterial(int tag, int typeTag, HeatTransferDomain* theDomain, Matrix thepars, Vector matPars)
    :HeatTransferMaterial(tag), trial_temp(0.0), charTime(0.0), HtComb(0.0), trialphTag(0), TempTag(0),
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
    rho0 = 0;
    moist = 0;
    //phaseTag =0: Wet Wood
    // phaseTag =1: Dry Wood
    // phaseTag =2: Char 
    //PhaseTag =3: Ash

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

    if (TypeTag!=0) {
        this->determinePhase(trial_temp, time);
    }
    

    return 0;
}


const Matrix& 
TimberHTMaterial::getConductivity(void)
{
    double materialK = 0;
    if (TypeTag == 0) {
        if (trial_temp <= 20)
            materialK = 0.12;
        else if (trial_temp <= 200)
            materialK = 0.12 + 0.03 * (trial_temp - 20) / 180;
        else if (trial_temp <= 350)
            materialK = 0.15 - 0.08 * (trial_temp - 200) / 150;
        else if (trial_temp <= 500)
            materialK = 0.07 + 0.02 * (trial_temp - 350) / 150;
        else if (trial_temp <= 800)
            materialK = 0.09 + 0.26 * (trial_temp - 500) / 300;
        else if (trial_temp <= 1200)
            materialK = 0.35 + 1.15 * (trial_temp - 800) / 400;
        else
            opserr << "TimberHTMaterial::getSpecificHeat recieves incorrect temperature: " << trial_temp << endln;
    }
    else {
        
        if (trialphTag == 0) {
            materialK = (*thePars)(0, 2);
            //wet wood
        }
        else if (trialphTag == 10)
        {
            if (trial_temp <= 95)
                materialK = (*thePars)(0, 2);
            else if(trial_temp<=125)
                materialK = (*thePars)(0, 2) + ((*thePars)(1, 2) - (*thePars)(0, 2)) * (trial_temp - 95) / 25;
            else
                materialK = (*thePars)(1, 2);
        }
        else if (trialphTag == 1) {
            materialK = (*thePars)(1, 2);
                //+ ((*thePars)(2, 2) - (*thePars)(1, 2)) * (trial_temp - 125) / 175;
      
            //dry wood
        }
        else if (trialphTag == 2) {
            if (trial_temp <= 400)
                materialK = (*thePars)(1, 2) + ((*thePars)(2, 2) - (*thePars)(1, 2)) * (trial_temp - 300) / 100;
            else if (trial_temp <= 700)
                materialK = (*thePars)(2, 2);
            else if (trial_temp <= 800)
                materialK = (*thePars)(2, 2) + ((*thePars)(3, 2) - (*thePars)(2, 2)) * (trial_temp - 700) / 100;
            else if (TempTag == 1) {
                materialK = (*thePars)(3, 2);
            }
          
            //char
        }
        else if (trialphTag == 3) {
            materialK = (*thePars)(3, 2);
            //ash
        }
        else
            opserr << "TimberHTMaterial::unrecognised trialphTag " << trialphTag;
    }
	
    if (materialK < 0)
        opserr << "incorrect conductivity" << endln;

	(*k)(0,0) = materialK;
	(*k)(1,1) = materialK;
	(*k)(2,2) = materialK;

    return *k; // change
}


double  
TimberHTMaterial::getRho(void)
{
    if (TypeTag == 0) {
        if (trial_temp <= 100)
            rho = rho0*(1 + moist);
        else if (trial_temp <= 200)
            rho = rho0;
        else if (trial_temp <= 250)
            rho = rho0*(1.00 - 0.07 * (trial_temp - 200) / 50);
        else if (trial_temp <= 300)
            rho = rho0 * (0.93 - 0.17 * (trial_temp - 250) / 50);
        else if (trial_temp <= 350)
            rho = rho0 * (0.76 - 0.24 * (trial_temp - 300) / 50);
        else if (trial_temp <= 400)
            rho = rho0 * (0.52 - 0.14 * (trial_temp - 350) / 50);
        else if (trial_temp <= 600)
            rho = rho0 * (0.38 - 0.1 * (trial_temp - 400) / 200);
        else if (trial_temp <= 800)
            rho = rho0 * (0.28 - 0.02 * (trial_temp - 600) / 200);
        else if (trial_temp <= 1200)
            rho = rho0 * (0.26 - 0.26 * (trial_temp - 800) / 400);
        else
            opserr << "TimberHTMaterial::getSpecificHeat recieves incorrect temperature: " << trial_temp << endln;
    }
    else {
        if (trialphTag == 0) {
            rho = (*thePars)(0, 1);
            //wet wood
        }
        else if (trialphTag == 10) {
            if (trial_temp <= 95)
                rho = (*thePars)(0, 1);
            else if(trial_temp<=125)
                rho = (*thePars)(0, 1) + ((*thePars)(1, 1) - (*thePars)(0, 1)) * (trial_temp - 95) / 30;
            else
                rho = (*thePars)(1, 1);
        }
        else if (trialphTag == 1) {
           // if (trial_temp <= 200)
             //   rho = (*thePars)(1, 1);
           // else
            rho = (*thePars)(1, 1);
           // rho = (*thePars)(1, 1) + ((*thePars)(2, 1) - (*thePars)(1, 1)) * (trial_temp - 125) / 175;

            //dry wood
        }
        else if (trialphTag == 2) {
            if (trial_temp <= 400)
                rho = (*thePars)(1, 1) + ((*thePars)(2, 1) - (*thePars)(1, 1)) * (trial_temp - 300) / 100;
            else if (trial_temp <= 700)
                rho = (*thePars)(2, 1);
            else if (trial_temp <= 800)
                rho = (*thePars)(2, 1) + ((*thePars)(3, 1) - (*thePars)(2, 1)) * (trial_temp - 700) / 100;
            else if (TempTag == 1) {
                rho = (*thePars)(3, 1);
            }

            //char
        }
        else if (trialphTag == 3) {
            rho = (*thePars)(3, 1);
            //ash
        }
        else
            opserr << "TimberHTMaterial::unrecognised trialphTag " << trialphTag;
    }

    if (rho < 0)
        opserr << "incorrect density" << endln;

  return rho;
}


double 
TimberHTMaterial::getSpecificHeat(void)
{
    if (TypeTag == 0) {
        if (trial_temp <= 20)
            cp = 1530.0;
        else if (trial_temp <= 99)
            cp = 1530.0 + 240.0 * (trial_temp - 20) / 79;
        else if (trial_temp <= 109)
            cp = 1770.0 + 11830 * (trial_temp - 99) / 10;
        else if (trial_temp <= 119)
            cp = 13600.0 - 100.0 * (trial_temp - 109) / 10;
           // cp = 1770.0 + 350.0 * (trial_temp - 100) / 20;
        else if (trial_temp <= 129)
            cp = 13500.0 - 11380.0 * (trial_temp - 119) / 10;
        else if (trial_temp <= 200)
            cp = 2120.0 - 120.0 * (trial_temp - 121) / 79;
        else if (trial_temp <= 250)
            cp = 2000.0 - 380.0 * (trial_temp - 200) / 50;
        else if (trial_temp <= 300)
            cp = 1620.0 - 910.0 * (trial_temp - 250) / 50;
        else if (trial_temp <= 350)
            cp = 710.0 + 140.0 * (trial_temp - 300) / 50;
        else if (trial_temp <= 400)
            cp = 850.0 + 150.0 * (trial_temp - 350) / 50;
        else if (trial_temp <= 600)
            cp = 1000.0 + 400.0 * (trial_temp - 400) / 200;
        else if (trial_temp <= 800)
            cp = 1400.0 + 250.0 * (trial_temp - 600) / 200;
        else if (trial_temp <= 1200)
            cp = 1650.0;
        else
            opserr << "TimberHTMaterial::getSpecificHeat recieves incorrect temperature: " << trial_temp << endln;
    }
    else {
        if (trialphTag == 0) {
            cp = (*thePars)(0, 3);
        }
        else if (trialphTag == 10) {
            double maxcp = 13600;
            if (trial_temp <= 95)
                cp = (*thePars)(0, 3);
             else if (trial_temp <= 105)
                 cp = (*thePars)(0, 3) + (maxcp - (*thePars)(0, 3)) * (trial_temp - 95.0) / 10.0;
            else if (trial_temp <= 115)
                 cp = maxcp;
            else if (trial_temp <= 125)
                cp = maxcp- (maxcp - (*thePars)(1, 3)) * (trial_temp - 115.0) / 10.0;
            else
                cp = (*thePars)(1, 3);
        }
        else if (trialphTag == 1) {
            //if (trial_temp <= 200)
               // cp = (*thePars)(1, 3);
           // else
            cp = (*thePars)(1, 3);
            //cp = (*thePars)(1, 3) + ((*thePars)(2, 3) - (*thePars)(1, 3)) * (trial_temp - 125) / 175;

            //dry wood
        }
        else if (trialphTag == 2) {
            if (trial_temp <= 400)
                cp = (*thePars)(1, 3) + ((*thePars)(2, 3) - (*thePars)(1, 3)) * (trial_temp - 300) / 100;
            else if (trial_temp <= 700)
                cp = (*thePars)(2, 3);
            else if (trial_temp <= 800)
                cp = (*thePars)(2, 3) + ((*thePars)(3, 3) - (*thePars)(2, 3)) * (trial_temp - 700) / 100;
            else if (TempTag == 1) {
                cp = (*thePars)(3, 3);
            }

            //char
        }
        else if (trialphTag == 3) {
            cp = (*thePars)(3, 3);
            //ash
        }
        else
            opserr << "TimberHTMaterial::unrecognised trialphTag " << trialphTag;
    }

    if (cp < 0)
        opserr << "incorrect specific heat" << endln;

    return cp;
}


double
TimberHTMaterial::getEnthalpy()
{
    
    // The temperature range is expanded other than the original one [20,1200] in Eurocode.
    // The reason is, for an analysis with initial temperature at 20, the solution could be lower than
    // 20 after initial iterations. Eventhough, the slope of H-T within the expanded temperature range is 
    // kept constant, the same as the heat capacity at T = 20;
    //if ((0.0 <= nod_temp) && (nod_temp <= 100.0)) {
    /*
      double maxcp = 13600;
    if (trial_temp <= 100.0) {
        double c1 = (*thePars)(0, 3)* (*thePars)(0, 1);
        double c2 = (*thePars)(0, 3) * (*thePars)(0, 1)*25;
        enthalpy = c1 * trial_temp - c2;
    }
    else if ((100.0 < trial_temp) && (trial_temp <= 115.0)) {
        double c = (*thePars)(0, 3) * (*thePars)(0, 1) * (100.0 - 25.0);
        double c11 = maxcp* (*thePars)(0, 1);
        double c12 = c- maxcp * (*thePars)(0, 1)*100;
        enthalpy = c11 * trial_temp + c12 ;
    }
    else if ((115.0 < trial_temp) && (trial_temp <= 125.0)) {
        double c = maxcp * (*thePars)(0, 1)*115.0+ (*thePars)(0, 3) * (*thePars)(0, 1) * (100.0 - 25.0) - maxcp * (*thePars)(0, 1) * 100.0;
        
        double c11 = (*thePars)(1, 1)*((*thePars)(1, 3)-maxcp)/20.0;
        double c12 = (*thePars)(1, 1)* maxcp-115.0*((*thePars)(1, 3)-maxcp)/10.0;
        double c13 = c11 * 115.0 * 115.0 + c12 * 115.0;
        enthalpy = c11 * trial_temp * trial_temp + c12 * trial_temp-c13+c;
    }
    else if (trial_temp <= 1200.0) {
        double c = maxcp * (*thePars)(0, 1) * 115.0 + (*thePars)(0, 3) * (*thePars)(0, 1) * (100.0 - 25.0) - maxcp * (*thePars)(0, 1) * 100.0;

        double c11 = (*thePars)(1, 1) * ((*thePars)(1, 3) - maxcp) / 20.0;
        double c12 = (*thePars)(1, 1) * maxcp - 115.0 * ((*thePars)(1, 3) - maxcp) / 10.0;
        double c13 = c11 * 115.0 * 115.0 + c12 * 115.0;
        double c20 = c11 * 125.0 * 125.0 + c12 * 125.0 - c13 + c;

        double c21 = (*thePars)(1, 3) * (*thePars)(1, 1);
        double c22 = c20 - (*thePars)(1, 3) * (*thePars)(1, 1) * 125.0;
        enthalpy = c21 * trial_temp + c22;
    }


    else
    */
  
        enthalpy = 0;
    


    
    return enthalpy;	
}


double
TimberHTMaterial::getEnthalpy(double temp)
{
    double enthp;

    /*
    double nod_temp = temp - 273.15;

    // The temperature range is expanded other than the original one [20,1200] in Eurocode.
    // The reason is, for an analysis with initial temperature at 20, the solution could be lower than
    // 20 after initial iterations. Eventhough, the slope of H-T within the expanded temperature range is 
    // kept constant, the same as the heat capacity at T = 20;
    //if ((0.0 <= nod_temp) && (nod_temp <= 100.0)) {
    double maxcp = 13600;
    if (nod_temp <= 100.0) {
        double c1 = (*thePars)(0, 3) * (*thePars)(0, 1);
        double c2 = (*thePars)(0, 3) * (*thePars)(0, 1) * 25;
        enthp = c1 * nod_temp - c2;
    }
    else if (nod_temp <= 115.0) {
        double c = (*thePars)(0, 3) * (*thePars)(0, 1) * (100.0 - 25.0);
        double c11 = maxcp * (*thePars)(0, 1);
        double c12 = c - maxcp * (*thePars)(0, 1) * 100;
        enthp = c11 * nod_temp + c12;
    }
    else if (nod_temp <= 125.0) {
        double c = maxcp * (*thePars)(0, 1) * 115.0 + (*thePars)(0, 3) * (*thePars)(0, 1) * (100.0 - 25.0) - maxcp * (*thePars)(0, 1) * 100.0;

        double c11 = (*thePars)(1, 1) * ((*thePars)(1, 3) - maxcp) / 20.0;
        double c12 = (*thePars)(1, 1) * maxcp - 115.0 * ((*thePars)(1, 3) - maxcp)* (*thePars)(1, 1) / 10.0;
        double c13 = c11 * 115.0 * 115.0 + c12 * 115.0;
        enthp = c11 * nod_temp * nod_temp + c12 * nod_temp - c13 + c;
    }
   
    else if (nod_temp <= 200.0) {
        double c = maxcp * (*thePars)(0, 1) * 115.0 + (*thePars)(0, 3) * (*thePars)(0, 1) * (100.0 - 25.0) - maxcp * (*thePars)(0, 1) * 100.0;

        double c11 = (*thePars)(1, 1) * ((*thePars)(1, 3) - maxcp) / 20.0;
        double c12 = (*thePars)(1, 1) * maxcp - 115.0 * ((*thePars)(1, 3) - maxcp) * (*thePars)(1, 1) / 10.0;
        double c13 = c11 * 115.0 * 115.0 + c12 * 115.0;
        double c20 = c11 * 125.0 * 125.0 + c12 * 125.0 - c13 + c;

        double c21 = (*thePars)(1, 3) * (*thePars)(1, 1);
        double c22 = c20 - (*thePars)(1, 3) * (*thePars)(1, 1) * 125.0;
        enthp = c21 * nod_temp + c22;
    }
  
    
    else
    
    */
    
        enthp = 0;

    return enthp;
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

    PhaseTag = trialphTag;
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
    
    PhaseTag = 0;
    return 0;
}


int
TimberHTMaterial::determinePhase(double temp, double time)
{
    
    trialphTag = PhaseTag;
    
    //determine phase
    if (temp <95) {
        //<100oC
        if(trialphTag<1)
            trialphTag = 0;
       // if (trialphTag ==10)
         //   trialphTag = 0;
        pht1 = 0;
        //Wet wood
    }
    else if (temp < T2)
    {
        //<300oC
        if (trialphTag == 0 || trialphTag == 10) {

            // if (pht1 < 1e-6 && trialphTag == 0) {
             //    pht1 = time;
            // }
            // else {
            //     pht2 = time;
            // }

           //  if ((pht2 - pht1) > dt1) {
           //      trialphTag = 1;
             //    pht1 = 0;
            // } 
            if (temp < 125)
                trialphTag = 10; // evaporation
            else
                trialphTag = 1;  //dry wood
        }
        else if(trialphTag == 1)
            trialphTag = 1;
        pht1 = 0;
        //dry wood
    }
    else if (temp < T3)
    {
        //<800oC
        if (trialphTag < 2|| trialphTag ==10) {
            if (pht1 < 1e-6 ) {
                if (trialphTag == 1 || trialphTag == 10) {
                    pht1 = time;
                    pht2 = time;
                }
                    
            }

           
            trialphTag = 2;// enter char stage
           
           if (charTime < 1e-6)
                charTime = time;
           
        }
        else if (trialphTag == 2)
        {
            pht2 = time;
        }

        //char
    }
    else {
        if (trialphTag < 3) {
            if ((pht2-pht1>1e-6)&&TempTag==0) {
                pht1 = time;
                pht2 = time;
                TempTag = 1;
            }

            if (TempTag == 1)
                pht2 = time;
            

            if ((pht2 - pht1 > dt3) && TempTag==1) {
                trialphTag = 3;
                //pht1 = 0;
            }
        }
        //ash
    }

      
    return 0;
}


bool
TimberHTMaterial::getIfHeatGen()
{
    if (trialphTag == 2 )
        return true;
    else
        return false;
}


double
TimberHTMaterial::getHeatGen()
{
    double Qgen = 0;
    double alpha = 0;

    if (trialphTag == 2)
    {
        if (TempTag == 0) {
            if (pht2 - pht1 < dt2)
                alpha = (pht2 - pht1) / dt2;
            else
                alpha = 1;
        }
        else if (TempTag == 1) {
            alpha = 1- (pht2 - pht1) / dt3;
        }
        
        Qgen = alpha* HtComb;
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



