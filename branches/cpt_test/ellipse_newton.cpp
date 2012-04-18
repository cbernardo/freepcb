#include "stdafx.h"
#include <math.h>

//----------------------------------------------------------------------------
double DistancePointEllipseSpecial (double dU, double dV, double dA,
									double dB, const double dEpsilon, const int iMax, int& riIFinal,
									double& rdX, double& rdY)
{
	// initial guess
	double dT = dB*(dV - dB);

	// Newton’s method
	int i;
	for (i = 0; i < iMax; i++)
	{
		double dTpASqr = dT + dA*dA;
		double dTpBSqr = dT + dB*dB;
		double dInvTpASqr = 1.0/dTpASqr;

		double dInvTpBSqr = 1.0/dTpBSqr;
		double dXDivA = dA*dU*dInvTpASqr;
		double dYDivB = dB*dV*dInvTpBSqr;
		double dXDivASqr = dXDivA*dXDivA;
		double dYDivBSqr = dYDivB*dYDivB;
		double dF = dXDivASqr + dYDivBSqr - 1.0;
		if (dF < dEpsilon)
		{
			// F(t0) is close enough to zero, terminate the iteration
			rdX = dXDivA*dA;
			rdY = dYDivB*dB;
			riIFinal = i;
			break;
		}
		double dFDer = 2.0*(dXDivASqr*dInvTpASqr + dYDivBSqr*dInvTpBSqr);
		double dRatio = dF/dFDer;
		if (dRatio < dEpsilon)
		{
			// t1-t0 is close enough to zero, terminate the iteration
			rdX = dXDivA*dA;
			rdY = dYDivB*dB;
			riIFinal = i;
			break;
		}
		dT += dRatio;
	}
	if (i == iMax)
	{
		// method failed to converge, let caller know
		riIFinal = -1;
		return -FLT_MAX;
	}
	double dDelta0 = rdX - dU, dDelta1 = rdY - dV;
	return sqrt(dDelta0*dDelta0 + dDelta1*dDelta1);
}
//----------------------------------------------------------------------------
double DistancePointEllipse (
							 double dU, double dV, // test point (u,v)
							 double dA, double dB, // ellipse is (x/a)^2 + (y/b)^2 = 1
							 const double dEpsilon, // zero tolerance for Newton’s method
							 const int iMax, // maximum iterations in Newton’s method
							 int& riIFinal, // number of iterations used
							 double& rdX, double& rdY) // a closest point (x,y)
{
	// special case of circle
	if (fabs(dA-dB) < dEpsilon)
	{
		double dLength = sqrt(dU*dU+dV*dV);

		return fabs(dLength - dA);
	}
	// reflect U = -U if necessary, clamp to zero if necessary
	bool bXReflect;
	if (dU > dEpsilon)
	{
		bXReflect = false;
	}
	else if (dU < -dEpsilon)
	{
		bXReflect = true;
		dU = -dU;
	}
	else
	{
		bXReflect = false;
		dU = 0.0;
	}
	// reflect V = -V if necessary, clamp to zero if necessary
	bool bYReflect;
	if (dV > dEpsilon)
	{
		bYReflect = false;
	}
	else if (dV < -dEpsilon)
	{
		bYReflect = true;
		dV = -dV;
	}
	else
	{
		bYReflect = false;
		dV = 0.0;
	}
	// transpose if necessary
	double dSave;
	bool bTranspose;
	if (dA >= dB)
	{
		bTranspose = false;
	}
	else
	{
		bTranspose = true;
		dSave = dA;
		dA = dB;
		dB = dSave;
		dSave = dU;
		dU = dV;

		dV = dSave;
	}
	double dDistance;
	if (dU != 0.0)
	{
		if (dV != 0.0)
		{
			dDistance = DistancePointEllipseSpecial(dU,dV,dA,dB,dEpsilon,iMax,
				riIFinal,rdX,rdY);
		}
		else
		{
			double dBSqr = dB*dB;
			if (dU < dA - dBSqr/dA)
			{
				double dASqr = dA*dA;
				rdX = dASqr*dU/(dASqr-dBSqr);
				double dXDivA = rdX/dA;
				rdY = dB*sqrt(fabs(1.0-dXDivA*dXDivA));
				double dXDelta = rdX - dU;
				dDistance = sqrt(dXDelta*dXDelta+rdY*rdY);
				riIFinal = 0;
			}
			else
			{
				dDistance = fabs(dU - dA);
				rdX = dA;
				rdY = 0.0;
				riIFinal = 0;
			}
		}
	}
	else
	{
		dDistance = fabs(dV - dB);
		rdX = 0.0;
		rdY = dB;
		riIFinal = 0;
	}
	if (bTranspose)
	{
		dSave = rdX;
		rdX = rdY;
		rdY = dSave;
	}
	if (bYReflect)
	{
		rdY = -rdY;
	}

	if (bXReflect)
	{
		rdX = -rdX;
	}
	return dDistance;
}