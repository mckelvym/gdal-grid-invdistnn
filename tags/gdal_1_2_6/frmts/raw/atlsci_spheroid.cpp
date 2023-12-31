/******************************************************************************
 * $Id$
 *
 * Project:  Spheroid classes
 * Purpose:  Provide spheroid lookup table base classes.
 * Author:   Gillian Walter
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.1  2003/03/03 20:10:05  gwalter
 * Updated MFF and HKV (MFF2) georeferencing support.
 *
 */

#include "atlsci_spheroid.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/**********************************************************************/
/* ================================================================== */
/*          Spheroid definitions                                      */
/* ================================================================== */
/**********************************************************************/


void SpheroidItem :: SetValuesByRadii(const char *spheroidname, double eq_radius, double p_radius)
{
    spheroid_name = CPLStrdup(spheroidname);
    equitorial_radius=eq_radius;
    polar_radius=p_radius;
    inverse_flattening=eq_radius/(eq_radius - polar_radius);
}

void SpheroidItem :: SetValuesByEqRadiusAndInvFlattening(const char *spheroidname, double eq_radius, double inverseflattening)
{
    spheroid_name = CPLStrdup(spheroidname);
    equitorial_radius=eq_radius;
    polar_radius=eq_radius*(1.0 - (1.0/inverse_flattening));
    inverse_flattening=inverseflattening;
}
SpheroidItem :: SpheroidItem()
{
  spheroid_name=NULL;
  equitorial_radius=-1.0;
  polar_radius=-1.0;
  inverse_flattening=-1.0;
}

SpheroidItem :: ~SpheroidItem()
{
  if (spheroid_name != NULL)
      CPLFree(spheroid_name);
}

SpheroidList :: SpheroidList()
{
  num_spheroids=0;
}

SpheroidList :: ~SpheroidList()
{
}

char *SpheroidList :: GetSpheroidNameByRadii( double eq_radius, double polar_radius )
{
  int index=0;
  double er=0.0;
  double pr=0.0;

  for(index=0;index<num_spheroids;index++)
  {
    er = spheroids[index].equitorial_radius;
    pr = spheroids[index].polar_radius;
    if ((fabs(er - eq_radius) < epsilonR) && (fabs(pr - polar_radius) < epsilonR))
      return CPLStrdup(spheroids[index].spheroid_name);
  }
  
  return NULL;

}

char *SpheroidList :: GetSpheroidNameByEqRadiusAndInvFlattening( double eq_radius, double inverse_flattening )
{
  int index=0;
  double er=0.0;
  double invf=0.0;

  for(index=0;index<num_spheroids;index++)
  {
    er = spheroids[index].equitorial_radius;
    invf = spheroids[index].inverse_flattening;
    if ((fabs(er - eq_radius) < epsilonR) && (fabs(invf - inverse_flattening) < epsilonI))
      return CPLStrdup(spheroids[index].spheroid_name);
  }
  
  return NULL;

}

double SpheroidList :: GetSpheroidEqRadius( const char *spheroid_name )
{
  int index=0;

  for(index=0;index<num_spheroids;index++)
  {
    if EQUAL(spheroids[index].spheroid_name,spheroid_name)
      return spheroids[index].equitorial_radius;
  }
  
  return -1.0;

}

int SpheroidList :: SpheroidInList( const char *spheroid_name )
{
  /* Return 1 if the spheroid name is recognized; 0 otherwise */
  int index=0;

  for(index=0;index<num_spheroids;index++)
  {
    if EQUAL(spheroids[index].spheroid_name,spheroid_name) 
      return 1;
  }
  
  return 0;
}

double SpheroidList :: GetSpheroidInverseFlattening( const char *spheroid_name )
{
  int index=0;

  for(index=0;index<num_spheroids;index++)
  {
    if  EQUAL(spheroids[index].spheroid_name,spheroid_name)
      return spheroids[index].inverse_flattening;
  }
  
  return -1.0;

}

double SpheroidList :: GetSpheroidPolarRadius( const char *spheroid_name )
{
  int index=0;

  for(index=0;index<num_spheroids;index++)
  {
    if (strcmp(spheroids[index].spheroid_name,spheroid_name) == 0)
      return spheroids[index].polar_radius;
  }
  
  return -1.0;

}

